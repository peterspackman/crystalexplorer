#include "psi4interface.h"
#include "settings.h"

Psi4Interface::Psi4Interface(QWidget *parent)
    : QuantumChemistryInterface(parent) {}

QString Psi4Interface::outputFilename() { return _jobName + ".out"; }

bool Psi4Interface::executableInstalled() {
  return QFile::exists(executable());
}

QString Psi4Interface::executable() {
  return settings::readSetting(settings::keys::PSI4_EXECUTABLE).toString();
}

QString Psi4Interface::program() { return executable(); }

QStringList Psi4Interface::commandline(const JobParameters &jobParams) {
  return {jobParams.QMInputFilename};
}

QString Psi4Interface::inputFilename() {
  _inputFilename = _jobName + ".dat";
  return _inputFilename;
}

QString Psi4Interface::calculationName(QString cifFilename,
                                       QString crystalName) {
  Q_ASSERT(!crystalName.contains("/"));

  QFileInfo fileInfo(cifFilename);
  QString name = fileInfo.baseName().replace(" ", "_");
  return name + "_" + crystalName;
}

QString Psi4Interface::fchkFilename(const JobParameters &jobParams,
                                    QString crystalName) {
  QString calcName = calculationName(jobParams.inputFilename, crystalName);
  QString suffix = defaultFchkFileExtension();
  return calcName + "." + suffix;
}

QString Psi4Interface::exchangeKeyword(ExchangePotential exchange) {
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

QString Psi4Interface::correlationKeyword(CorrelationPotential correlation) {
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

QString Psi4Interface::methodName(const JobParameters &jobParams) {
  switch (jobParams.theory) {
  case Method::kohnSham:
    return exchangeKeyword(jobParams.exchangePotential) +
           correlationKeyword(jobParams.correlationPotential);
  case Method::b3lyp:
    return "b3lyp";
  case Method::mp2:
    return "mp2";
  case Method::hartreeFock:
    return "scf";
  default:
    return "scf";
  }
}

QString Psi4Interface::taskName(const JobParameters &jobParams) {
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

QString Psi4Interface::programName() { return "Psi4"; }

QString Psi4Interface::basisName(const JobParameters &jobParams) {
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
  return basisName;
}

void Psi4Interface::writeWavefunctionCalculationBlock(
    QTextStream &ts, const JobParameters &jobParams,
    const QString &crystalName) {
  QString basis = basisName(jobParams);
  QString method = methodName(jobParams);
  ts << "set scf_type direct" << Qt::endl;
  ts << "e, wfn = energy('" << method << "/" << basis << "', return_wfn=True)"
     << Qt::endl;
  ts << "fchk(wfn, '" << fchkFilename(jobParams, crystalName) << "')"
     << Qt::endl;
}

void Psi4Interface::writeInputForWavefunctionCalculation(
    QTextStream &ts, const JobParameters &jobParams,
    DeprecatedCrystal *crystal) {

  QString crystalName = crystal->crystalName();
  // Write atom info.
  ts << "set puream False" << Qt::endl;
  ts << "molecule m {" << Qt::endl;
  ts << QString("    %1 %2").arg(jobParams.charge).arg(jobParams.multiplicity)
     << Qt::endl;
  auto atoms = crystal->generateAtomsFromAtomIds(jobParams.atoms);
  for (const auto &atom :
       std::as_const(atoms)) { // Write out atoms in format:  symbol x  y  z
    QVector3D pos = atom.pos();
    QString symbol = atom.element()->symbol();
    // should check if 'using complete cluster'
    ts << QString("    %1 %2 %3 %4")
              .arg(symbol)
              .arg(pos.x(), 0, 'f', 6)
              .arg(pos.y(), 0, 'f', 6)
              .arg(pos.z(), 0, 'f', 6);
    ts << Qt::endl;
  }
  ts << "    no_reorient" << Qt::endl << "    no_com" << Qt::endl;
  ts << "}" << Qt::endl;
  writeWavefunctionCalculationBlock(ts, jobParams, crystalName);
}
