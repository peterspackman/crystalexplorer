#include "nwcheminterface.h"
#include "settings.h"

NWChemInterface::NWChemInterface(QWidget *parent)
    : QuantumChemistryInterface(parent) {}

QString NWChemInterface::outputFilename() {
  QString extension = "nwchem_stdout";
  return _jobName + "." + extension;
}

bool NWChemInterface::executableInstalled() {
  return QFile::exists(executable());
}

QString NWChemInterface::executable() {
  return settings::readSetting(settings::keys::NWCHEM_EXECUTABLE).toString();
}

QString NWChemInterface::program() { return executable(); }

QStringList NWChemInterface::commandline(const JobParameters &jobParams) {
  return {jobParams.QMInputFilename};
}

QString NWChemInterface::inputFilename() {
  _inputFilename = _jobName + ".nw";
  return _inputFilename;
}

QString NWChemInterface::calculationName(QString cifFilename,
                                         QString crystalName) {
  Q_ASSERT(!crystalName.contains("/"));

  QFileInfo fileInfo(cifFilename);
  QString name = fileInfo.baseName().replace(" ", "_");
  return name + "_" + crystalName;
}

QString NWChemInterface::moldenFileName(const JobParameters &jobParams,
                                        QString crystalName) {
  QString calcName = calculationName(jobParams.inputFilename, crystalName);
  QString suffix = defaultMoldenFileExtension();
  return calcName + defaultMoldenFileExtension();
}

void NWChemInterface::writeChargeAndScfBlock(QTextStream &ts,
                                             const JobParameters &jobParams) {
  ts << QString("charge %1").arg(jobParams.charge) << Qt::endl;
  ts << "scf" << Qt::endl;
  ts << QString("nopen %1").arg(jobParams.multiplicity - 1) << Qt::endl;

  // Since tonto doesn't support ROHF wavefunctions, use UHF.
  if (jobParams.multiplicity > 1) {
    ts << "uhf" << Qt::endl;
  }
  ts << "end" << Qt::endl;
}

void NWChemInterface::writeMoldenBlock(QTextStream &ts) {
  ts << "property" << Qt::endl;
  ts << "moldenfile" << Qt::endl;
  ts << "molden_norm nwchem" << Qt::endl;
  ts << "end" << Qt::endl;
}

QString NWChemInterface::exchangeKeyword(ExchangePotential exchange) {
  QString keyword;
  switch (exchange) {
  case ExchangePotential::slater:
    keyword = "slater";
    break;
  case ExchangePotential::becke88:
    keyword = "becke88";
    break;
  default:
    keyword = "slater";
    break;
  }
  return keyword;
}

QString NWChemInterface::correlationKeyword(CorrelationPotential correlation) {
  QString keyword;
  switch (correlation) {
  case CorrelationPotential::vwn:
    keyword = "vwn3";
    break;
  case CorrelationPotential::lyp:
  default:
    keyword = "lyp";
    break;
  }
  return keyword;
}

void NWChemInterface::writeDftBlock(QTextStream &ts,
                                    const JobParameters &jobParams) {
  switch (jobParams.theory) {
  case Method::kohnSham: {
    ts << "dft" << Qt::endl;
    ts << "xc " << exchangeKeyword(jobParams.exchangePotential);
    ts << " " << correlationKeyword(jobParams.correlationPotential) << Qt::endl;
    ts << "end" << Qt::endl;
    break;
  }
  case Method::b3lyp: {
    ts << "dft" << Qt::endl;
    ts << "xc b3lyp" << Qt::endl;
    ts << "end" << Qt::endl;
  }
  case Method::mp2:
  case Method::hartreeFock:
  default:
    return;
  }
}

QString NWChemInterface::taskName(const JobParameters &jobParams) {
  switch (jobParams.theory) {
  case Method::mp2:
    return "mp2";
    break;
  case Method::b3lyp:
    return "dft";
    break;
  case Method::kohnSham:
    return "dft";
    break;
  default:
    return "scf";
  }
}

QString NWChemInterface::programName() { return "NWChem"; }

void NWChemInterface::writeBasisBlock(QTextStream &ts,
                                      const JobParameters &jobParams) {
  QString basisName;
  switch (jobParams.basisset) {
  case BasisSet::STO_3G:
    basisName = "sto-3g";
    break;
  case BasisSet::Pople3_21G:
    basisName = "3-21g";
    break;
  case BasisSet::Pople6_31Gd:
    basisName = "6-31g*";
    break;
  case BasisSet::Pople6_31Gdp:
    basisName = "6-31g**";
    break;
  case BasisSet::Pople6_311Gdp:
    basisName = "6-311g**";
    break;
  case BasisSet::CC_PVDZ:
    basisName = "cc-pvdz";
    break;
  case BasisSet::CC_PVTZ:
    basisName = "cc-pvtz";
    break;
  case BasisSet::CC_PVQZ:
    basisName = "cc-pvqz";
    break;
  default:
    basisName = "not distributed with nwchem, see";
    break;
  }

  ts << "basis" << Qt::endl;
  ts << "* library " << basisName << Qt::endl;
  ts << "end" << Qt::endl;
}

void NWChemInterface::writeInputForWavefunctionCalculation(
    QTextStream &ts, const JobParameters &jobParams,
    DeprecatedCrystal *crystal) {

  ts << "title \"" << _jobName << "\"" << Qt::endl;
  ts << "start " << _jobName << Qt::endl;

  writeChargeAndScfBlock(ts, jobParams);
  ts << "geometry nocenter noautosym noautoz units angstroms" << Qt::endl;

  // Write atom info.

  auto atoms = crystal->generateAtomsFromAtomIds(jobParams.atoms);
  for (const auto &atom : atoms) { // Write out atoms in format:  symbol x  y  z
    QVector3D pos = atom.pos();
    QString symbol = atom.element()->symbol();
    // should check if 'using complete cluster'
    ts << QString("%1 %2 %3 %4")
              .arg(symbol)
              .arg(pos.x(), 0, 'f', 6)
              .arg(pos.y(), 0, 'f', 6)
              .arg(pos.z(), 0, 'f', 6);
    ts << Qt::endl;
  }
  ts << "end" << Qt::endl;
  writeBasisBlock(ts, jobParams);
  writeDftBlock(ts, jobParams);
  writeMoldenBlock(ts);
  ts << "task " << taskName(jobParams) << " property";
}
