#include "gaussianinterface.h"
#include "settings.h"

GaussianInterface::GaussianInterface(QWidget *parent)
    : QuantumChemistryInterface(parent) {}

void GaussianInterface::prejobSetup() {
  _gaussianVersion = getGaussianVersion();
}

QString GaussianInterface::outputFilename() {
  QString extension;
  switch (_gaussianVersion) {
  case g98:
    extension = GAUSSIAN_OUTPUT_EXTENSION[(int)g98];
    break;
  case g03:
    extension = GAUSSIAN_OUTPUT_EXTENSION[(int)g03];
    break;
  case g09:
    extension = GAUSSIAN_OUTPUT_EXTENSION[(int)g09];
    break;
  }

  return _jobName + "." + extension;
}

bool GaussianInterface::isExecutableInstalled() {
  return QFile::exists(executable());
}

bool GaussianInterface::executableInstalled() {
  return QFile::exists(executable());
}

QString GaussianInterface::executable() {
  return settings::readSetting(settings::keys::GAUSSIAN_EXECUTABLE).toString();
}

QString GaussianInterface::program() { return executable(); }

QStringList GaussianInterface::commandline(const JobParameters &jobParams) {
  return {jobParams.QMInputFilename};
}

QString GaussianInterface::inputFilename() {

  _inputFilename =
      _jobName + "." + GAUSSIAN_INPUT_EXTENSION[(int)_gaussianVersion];

  return _inputFilename;
}

QString GaussianInterface::normalTerminationHook() {
  return "Normal termination";
}

QString GaussianInterface::exchangeKeyword(ExchangePotential exchange) {
  QString keyword;
  switch (exchange) {
  case ExchangePotential::slater:
    keyword = "S";
    break;
  case ExchangePotential::x_alpha:
    keyword = "XA";
    break;
  case ExchangePotential::becke88:
  default:
    keyword = "B";
    break;
  }
  return keyword;
}

QString
GaussianInterface::correlationKeyword(CorrelationPotential correlation) {
  QString keyword;
  switch (correlation) {
  case CorrelationPotential::vwn:
    keyword = "VWN";
    break;
  case CorrelationPotential::lyp:
  default:
    keyword = "LYP";
    break;
  }
  return keyword;
}

QString GaussianInterface::methodName(const JobParameters &jobParams,
                                      const bool unrestricted) {
  QString name;
  switch (jobParams.theory) {
  case Method::hartreeFock:
    name = "HF";
    break;
  case Method::mp2:
    name = "MP2";
    break;
  case Method::b3lyp:
    name = "B3LYP";
    break;
  case Method::kohnSham:
    name = exchangeKeyword(jobParams.exchangePotential) +
           correlationKeyword(jobParams.correlationPotential);
    break;
  default:
    name = "Unknown";
    break;
  }
  if (unrestricted)
    name = "U" + name;
  return name;
}

QString GaussianInterface::basissetName(BasisSet basisset) {
  QString name;
  switch (basisset) {
  case BasisSet::STO_3G:
    name = "STO-3G";
    break;
  case BasisSet::Pople3_21G:
    name = "3-21G";
    break;
  case BasisSet::Pople6_31Gd:
    name = "6-31G(d)";
    break;
  case BasisSet::Pople6_31Gdp:
    name = "6-31G(d,p)";
    break;
  case BasisSet::Pople6_311Gdp:
    name = "6-311G(d,p)";
    break;
  case BasisSet::D95V:
    name = "D95V";
    break;
  case BasisSet::DGDZVP:
    name = "DGDZVP";
    break;
  case BasisSet::CC_PVDZ:
    name = "cc-pVDZ";
    break;
  case BasisSet::CC_PVTZ:
    name = "cc-pVTZ";
    break;
  case BasisSet::CC_PVQZ:
    name = "cc-pVQZ";
    break;
  default:
    name = "Unknown";
    break;
  }
  return name;
}

QString GaussianInterface::keywords(const JobParameters &jobParams) {
  const QString DEFAULT_OUTPUT_LEVEL = "#P ";
  const QString DEFAULT_KEYWORDS = " 6d 10f NoSymm FChk";

  QString extra_keywords =
      (jobParams.theory == Method::mp2) ? " density=mp2" : "";

  return DEFAULT_OUTPUT_LEVEL +
         methodName(jobParams, shouldUseUnrestricted(jobParams.multiplicity)) +
         "/" + basissetName(jobParams.basisset) + DEFAULT_KEYWORDS +
         extra_keywords;
}

void GaussianInterface::writeInputForWavefunctionCalculation(
    QTextStream &ts, const JobParameters &jobParams,
    DeprecatedCrystal *crystal) {
  ts << keywords(jobParams) << Qt::endl;
  ts << " " << Qt::endl;
  ts << "CrystalExplorer Job" << Qt::endl;
  ts << " " << Qt::endl;
  ts << QString("%1 %2").arg(jobParams.charge).arg(jobParams.multiplicity)
     << Qt::endl;

  // Write atom info.

  auto atoms = crystal->generateAtomsFromAtomIds(jobParams.atoms);
  for (const auto &atom :
       (atoms)) { // Write out atoms in format:  symbol x  y  z
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
  ts << " " << Qt::endl; // Must finish with a blank line
}

// Adds a unique suffix to a file name so no existing file has the same file
// name. Can be used to avoid overwriting existing files. Works for both
// files/directories, and both relative/absolute paths. The suffix is in the
// form - "path/to/file.tar.gz", "path/to/file (1).tar.gz",
// "path/to/file (2).tar.gz", etc.
QString addUniqueSuffix(const QString &fileName) {
  // If the file doesn't exist return the same name.
  if (!QFile::exists(fileName)) {
    return fileName;
  }

  QFileInfo fileInfo(fileName);
  QString ret;

  // Split the file into 2 parts - dot+extension, and everything else. For
  // example, "path/file.tar.gz" becomes "path/file"+".tar.gz", while
  // "path/file" (note lack of extension) becomes "path/file"+"".
  QString secondPart = fileInfo.completeSuffix();
  QString firstPart;
  if (!secondPart.isEmpty()) {
    secondPart = "." + secondPart;
    firstPart = fileName.left(fileName.size() - secondPart.size());
  } else {
    firstPart = fileName;
  }

  // Try with an ever-increasing number suffix, until we've reached a file
  // that does not yet exist.
  for (int ii = 1;; ii++) {
    // Construct the new file name by adding the unique number between the
    // first and second part.
    ret = QString("%1_%2%3").arg(firstPart).arg(ii).arg(secondPart);
    // If no file exists with the new name, return it.
    if (!QFile::exists(ret)) {
      return ret;
    }
  }
}

// Inputfile for D2 disperison, BSSE corrected interaction energy from
// B3LYP/6-31g(d,p) calculation
// Used as a benchmark for the interactions energies obtained from Gaussian.
bool GaussianInterface::writeCounterpoiseInputFile(
    QString filename, DeprecatedCrystal *crystal,
    const JobParameters &jobParams) {
  Q_ASSERT(crystal != nullptr);

  bool success = false;
  auto fragAtomsA = jobParams.atoms.mid(0, jobParams.atomGroups[0]);
  auto fragAtomsB = jobParams.atoms.mid(jobParams.atomGroups[0]);
  auto identifier =
      crystal->calculateFragmentPairIdentifier(fragAtomsA, fragAtomsB);

  // Remove existing input and output file names
  filename = addUniqueSuffix(filename);

  QFile inputFile(filename);
  if (inputFile.open(QIODevice::WriteOnly)) {
    QTextStream ts(&inputFile);
    writeInputForCounterpoiseCalculation(ts, crystal, jobParams, identifier);
    inputFile.close();
    success = true;
  }
  return success;
}

/*
 *
 * This really needs a decent rewrite involving the multiplicity for each
 * fragment + the total multiplicity
 */
void GaussianInterface::writeInputForCounterpoiseCalculation(
    QTextStream &ts, DeprecatedCrystal *crystal, const JobParameters &jobParams,
    QString comments) {

  auto atomIdsForFragmentA = jobParams.atoms.mid(0, jobParams.atomGroups[0]);
  auto atomIdsForFragmentB = jobParams.atoms.mid(jobParams.atomGroups[0]);
  auto cmA = crystal->chargeMultiplicityForFragment(atomIdsForFragmentA);
  auto cmB = crystal->chargeMultiplicityForFragment(atomIdsForFragmentB);
  int chargeA = cmA.charge;
  int chargeB = cmB.charge;
  int total_multiplicity = cmA.multiplicity + cmB.multiplicity - 1;

  ts << "#P B3LYP/6-31G(d,p) 6d 10f NoSymm Counterpoise=2 "
        "EmpiricalDispersion=GD2"
     << Qt::endl;
  ts << " " << Qt::endl;
  ts << "CrystalExplorer Job " << comments << Qt::endl;
  ts << " " << Qt::endl;
  ts << QString("%1,%2 %3,%4 %5,%6")
            .arg(chargeA + chargeB)
            .arg(total_multiplicity)
            .arg(chargeA)
            .arg(cmA.multiplicity)
            .arg(chargeB)
            .arg(cmB.multiplicity)
     << Qt::endl;

  // Atoms for fragment A
  auto atoms = crystal->generateAtomsFromAtomIds(atomIdsForFragmentA);
  for (const auto &atom :
       (atoms)) { // Write out atoms in format:  symbol x  y  z
    QVector3D pos = atom.pos();
    QString symbol = atom.element()->symbol();
    // should check if 'using complete cluster'
    ts << QString("%1 %2 %3 %4 1")
              .arg(symbol)
              .arg(pos.x(), 0, 'f', 6)
              .arg(pos.y(), 0, 'f', 6)
              .arg(pos.z(), 0, 'f', 6);
    ts << Qt::endl;
  }

  // Atoms for fragment B
  atoms = crystal->generateAtomsFromAtomIds(atomIdsForFragmentB);
  for (const auto &atom :
       (atoms)) { // Write out atoms in format:  symbol x  y  z
    QVector3D pos = atom.pos();
    QString symbol = atom.element()->symbol();
    ts << QString("%1 %2 %3 %4 2")
              .arg(symbol)
              .arg(pos.x(), 0, 'f', 6)
              .arg(pos.y(), 0, 'f', 6)
              .arg(pos.z(), 0, 'f', 6);
    ts << Qt::endl;
  }

  ts << " " << Qt::endl; // Must finish with a blank line
}

QProcessEnvironment GaussianInterface::getEnvironment() {
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

  QString executableName = executable();
  QString exeDirectory = QFileInfo(executableName).dir().path();

  switch (_gaussianVersion) {
  case g98:
    // env.insert("g98root", rootDirectory);
    env.insert("GAUSS_EXEDIR", exeDirectory);
    break;
  case g03:
    // env.insert("g03root", rootDirectory);
    env.insert("GAUSS_EXEDIR", exeDirectory);
    break;
  case g09:
    // env.insert("g09root", rootDirectory);
    env.insert("GAUSS_EXEDIR", exeDirectory);
    break;
  default: // Shouldn't get here
    Q_ASSERT(false);
  }

  env.insert("GAUSS_SCRDIR", QDir::temp().path());

  return env;
}

QString GaussianInterface::programName() { return "Gaussian"; }

GaussianVersion GaussianInterface::getGaussianVersion() {
  GaussianVersion version;

  QFileInfo fi(executable());
  QString basename = fi.baseName().toLower();

  int index = GAUSSIAN_BASENAME.indexOf(basename);
  if (index != -1) {
    version = (GaussianVersion)index;
  } else {
    version = g09; // default to the latest version
  }

  return version;
}

bool GaussianInterface::shouldUseUnrestricted(int multiplicity) {
  return multiplicity > 1;
}
