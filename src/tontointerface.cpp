#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTextStream>
#include <QMessageBox>

#include "fmt/core.h"
#include "gaussianinterface.h" // just for getGaussianVersion(), defaultFChkFilename()
#include "nwcheminterface.h"
#include "psi4interface.h"
#include "settings.h"
#include "surfacedescription.h"
#include "tontointerface.h"

#include <QDebug>

TontoInterface::TontoInterface(QWidget *parent)
    : QObject(), _process(new QProcess()) {
  _parent = parent;
  init();
}

/// Brief Initialise: set up the runningman dialog
void TontoInterface::init() {
  _tontoInputEditor = new FileEditor();
  connect(_tontoInputEditor, SIGNAL(writtenFileToDisk()), this,
          SLOT(createProcessAndRunTonto()));
  connect(_process, SIGNAL(finished(int, QProcess::ExitStatus)), this,
          SLOT(jobFinished(int, QProcess::ExitStatus)));
  connect(_process, SIGNAL(stateChanged(QProcess::ProcessState)), this,
          SLOT(jobState(QProcess::ProcessState)));
}

TontoInterface::~TontoInterface() { delete _tontoInputEditor; }

QString TontoInterface::calculationName(const JobParameters &jobParams,
                                        const QString &crystalName) {
  Q_ASSERT(!crystalName.contains("/"));

  QFileInfo fileInfo(jobParams.inputFilename);
  QString name = fileInfo.baseName().replace(" ", "_");
  QString additionalInfo{""};
  if (jobParams.maxStep > 0) {
    additionalInfo += QString::fromStdString(
        fmt::format("_step_{}_{}", jobParams.step, jobParams.maxStep));
  }
  return name + "_" + crystalName + additionalInfo;
}

QString TontoInterface::tontoWavefunctionFileSuffix() { return QString("sbf"); }

// Returns the name in which Tonto stores the molecular orbitals
QString TontoInterface::tontoSBFName(const JobParameters &jobParams,
                                     const QString &crystalName) {
  QString calcName = calculationName(jobParams, crystalName);
  QString suffix = tontoWavefunctionFileSuffix();
  return calcName + "." + suffix;
}

QString TontoInterface::fchkFilename(const JobParameters &jobParams,
                                     const QString &crystalName) {
  QString calcName = calculationName(jobParams, crystalName);
  switch (jobParams.program) {
  case ExternalProgram::Psi4:
    return calcName + "." + Psi4Interface::defaultFchkFileExtension();
  default:
    return calcName + "." + GaussianInterface::defaultFChkFileExtension();
  }
}

QString TontoInterface::moldenFilename(const JobParameters &jobParams,
                                       const QString &crystalName) {
  QString calcName = calculationName(jobParams, crystalName);
  QString ext = NWChemInterface::defaultMoldenFileExtension();
  return calcName + ext;
}

QString TontoInterface::wavefunctionFilename(int index) {
  return _wavefunctionFilenames[index];
}

/// Brief Write the input file and run the job
void TontoInterface::runJob(const JobParameters &jobParams,
                            DeprecatedCrystal *crystal,
                            const QVector<Wavefunction> &wavefunctions) {
  // Set the internal job parameters
  _jobParams = jobParams;

  // Ensure the wavefunction files are written into the working directory
  int id = 0;
  _wavefunctionFilenames.clear();
  if (wavefunctions.size() == 2) {
    if (wavefunctions[0].jobParameters().atoms.size() !=
        wavefunctions[1].jobParameters().atoms.size()) {
      qDebug() << (wavefunctions[0].wavefunctionFile() ==
                   wavefunctions[1].wavefunctionFile());
      qDebug() << "should be false";
    }
  }
  for (const auto &wavefunction : wavefunctions) {
    QString filename =
        wavefunction.restoreWavefunctionFile(_workingDirectory, id);
    if (filename.isEmpty()) {
      QMessageBox::warning(_parent, tr("Error"),
                           tr("Unable to restore wavefunction files."));
      return;
    }
    _wavefunctionFilenames.append(filename);
    id++;
  }

  if (writeTontoInputfile(_jobParams, crystal)) {
    if (_jobParams.editInputFile) {
      editTontoInput();
    } else {
      createProcessAndRunTonto();
    }
  } else {
    QMessageBox::warning(_parent, tr("Error"),
                         tr("Unable to write Tonto input file."));
  }
}

void TontoInterface::editTontoInput() {
  _tontoInputEditor->insertFile(getTontoInputFile());
  _tontoInputEditor->show();
}

/// Brief Start the tonto job in the background and show the
/// runningman dialog. Emit the tontoRunning signal
void TontoInterface::createProcessAndRunTonto() {
  // We can't procede if Tonto not installed ...
  if (!tontoInstalled()) {
    return;
  }

  _tontoStoppedByUser = false;

  _process->setWorkingDirectory(_workingDirectory);

  // The empty QStringList is not an accident. It forces the first argument i.e.
  // the path to tonto
  // to be treated whole and not broken up at any spaces that may be present in
  // the path.
  _process->start(tontoExecutable(), QStringList());
}

void TontoInterface::jobState(QProcess::ProcessState state) {
  switch (state) {
  case QProcess::NotRunning:
    break;
  case QProcess::Starting:
    break;
  case QProcess::Running:
    emit updateProgressBar(_jobParams.step, _jobParams.maxStep);
    emit updateStatusMessage(jobDescription(
        _jobParams.jobType, _jobParams.maxStep, _jobParams.step));
    emit tontoRunning();
    break;
  }
}

QString TontoInterface::jobDescription(JobType jobType, int maxStep, int step) {
  QString description(jobProcessDescription(jobType));

  if (maxStep > 0) {
    description += QString(" (%1/%2)").arg(step).arg(maxStep);
  }
  return description;
}

void TontoInterface::stopJob() {
  _tontoStoppedByUser = true;

  if (_process && _process->state() == QProcess::Running) {
    _process->kill();
  }
  // emit tontoCancelled("Tonto job terminated.");
}

void TontoInterface::jobFinished(int exitCode,
                                 QProcess::ExitStatus exitStatus) {
  Q_UNUSED(exitCode);

  if (_tontoStoppedByUser) {
    emit tontoFinished(Stopped, _jobParams.jobType);
    return;
  }

  // We want to tidy up by deleting the QProcess but it seems that if the
  // program genuinely crashed
  // then the QProcess is automatically deleted hence the else block.
  if (exitStatus == QProcess::CrashExit) {
    emit tontoFinished(CrashExit, _jobParams.jobType);
    return;
  }

  if (!QFile::exists(TontoInterface::getTontoOutputFile())) {
    emit tontoFinished(NoOutput, _jobParams.jobType);
    return;
  }

  if (errorInTontoOutput()) {

    if (_jobParams.jobType == JobType::surfaceGeneration &&
        noIsosurfacePoints()) {
      emit tontoFinished(NoIsosurfacePoints, _jobParams.jobType);
    } else {
      emit tontoFinished(ErrorInOutput, _jobParams.jobType);
    }
    return;
  }

  emit tontoFinished(NormalExit, _jobParams.jobType);
}

/// Brief Return if the tonto executable has been defined and is really
/// executable
bool TontoInterface::tontoInstalled() {
  QString executable = tontoExecutable();

  if (executable.isEmpty()) {
    QMessageBox::critical(
        _parent, "Error",
        "CrystalExplorer does not know where the Tonto executable is.\n"
        "Please reinstall CrystalExplorer, and if this does not fix the "
        "problem, contact CrystalExplorer Support.\n");
    return false;
  }
  if (!QFile::exists(executable)) {
    QMessageBox::critical(
        _parent, "Error",
        "CrystalExplorer cannot find the Tonto executable.\n\n"
        "Please reinstall CrystalExplorer, and if this does not fix the "
        "problem, contact CrystalExplorer Support.\n");
    return false;
  }
  return true;
}

/// Brief return the full pathname of the tonto executable
QString TontoInterface::tontoExecutable() {
  // Put the appropriate delimiters around the executable path
  // under Windows to account for possible spaces in the path
  QString tontoDefault =
      settings::readSetting(settings::keys::TONTO_EXECUTABLE).toString();
  settings::writeSettingIfEmpty(settings::keys::TONTO_USER_EXECUTABLE,
                                tontoDefault);
  return settings::readSetting(settings::keys::TONTO_USER_EXECUTABLE)
      .toString();
  ;
}

/// Brief Set the working directory for the tonto job
void TontoInterface::setWorkingDirectory(const QString filename) {
  QFileInfo fileInfo(filename);
  _workingDirectory = fileInfo.absolutePath();
}

/// Brief return the working directory for the tonto job
QString TontoInterface::workingDirectory() { return _workingDirectory; }

/// Brief Return the tonto input file name
QString TontoInterface::getTontoInputFile() {
  Q_ASSERT(!workingDirectory().isEmpty());

  QString tontoInputFile =
      workingDirectory() + QDir::separator() + TONTO_INPUT_FILENAME;
  return tontoInputFile;
}

/// Brief Return the tonto output file name as a QString
QString TontoInterface::getTontoOutputFile() {
  Q_ASSERT(!workingDirectory().isEmpty());

  QString tontoOutputFile =
      workingDirectory() + QDir::separator() + TONTO_OUTPUT_FILENAME;
  return tontoOutputFile;
}

/// Brief Return TRUE if the search string is found in the Tonto output file
bool TontoInterface::foundStringInTontoOutput(const QString &searchString,
                                              bool atBeginningOfLine) {
  QFile file(TontoInterface::getTontoOutputFile());
  if (file.open(QIODevice::ReadOnly)) {
    QTextStream ts(&file);
    while (!ts.atEnd()) {
      QString line = ts.readLine();
      if (atBeginningOfLine &&
          line.startsWith(searchString, Qt::CaseInsensitive)) {
        return true;
      }
      if (!atBeginningOfLine &&
          line.contains(searchString, Qt::CaseInsensitive)) {
        return true;
      }
    }
  }
  return false;
}

/// Brief Return TRUE if the tonto out has an error
bool TontoInterface::errorInTontoOutput() {
  return foundStringInTontoOutput(ERROR_HOOK);
}

/// Brief Return TRUE if the tonto stdout report no isosurface points
/// This indicates the isovalue used to generate the surface was too low
bool TontoInterface::noIsosurfacePoints() {
  return foundStringInTontoOutput(NO_ISOSURFACE_POINTS_HOOK);
}

/// Brief Write a tonto input file
bool TontoInterface::writeTontoInputfile(const JobParameters &jobParams,
                                         DeprecatedCrystal *crystal) {

  bool success = false;

  // Get the input and output file names
  QString tontoInputFile = TontoInterface::getTontoInputFile();
  QString tontoOutputFile = TontoInterface::getTontoOutputFile();

  // Remove existing input and output file names
  if (QFile::exists(tontoInputFile)) {
    QFile::remove(tontoInputFile);
  }
  if (QFile::exists(tontoOutputFile)) {
    QFile::remove(tontoOutputFile);
  }
  if (QFile::exists(jobParams.outputFilename)) {
    QFile::remove(jobParams.outputFilename);
  }

  QFile inputFile(tontoInputFile);
  if (inputFile.open(QIODevice::WriteOnly)) {
    QTextStream ts(&inputFile);

    switch (jobParams.jobType) {
    case JobType::cifProcessing:
      writeInputForCifProcessing(ts, jobParams);
      break;
    case JobType::surfaceGeneration:
      if (jobParams.atomGroups.size() > 0) {
        writeInputForSurfaceWithProductProperty(ts, jobParams, crystal);
      } else {
        writeInputForSurfaceGeneration(ts, jobParams, crystal);
      }
      break;
    case JobType::wavefunction:
      writeInputForWavefunctionCalculation(ts, jobParams, crystal);
      break;
    case JobType::pairEnergy:
      writeInputForEnergyCalculation(ts, jobParams, crystal);
      break;
    default:
      success = false;
      break;
    }
    inputFile.close();
    success = true;
  }

  return success;
}

void TontoInterface::writeInputForCifProcessing(
    QTextStream &ts, const JobParameters &jobParams) {
  writeHeader(ts, "Tonto input file for CIF Processing.");
  writeBasisset(ts);
  writeCifData(ts, jobParams.inputFilename, jobParams.overrideBondLengths);
  writeProcessCifForCX(ts, jobParams.outputFilename);
  writeFooter(ts);
}

void TontoInterface::writeInputForSurfaceGeneration(
    QTextStream &ts, const JobParameters &jobParams,
    DeprecatedCrystal *crystal) {
  Q_ASSERT(crystal != nullptr);

  writeHeader(ts, "Tonto input file for Surface Generation.");
  writeChargeMultiplicity(ts, jobParams.charge, jobParams.multiplicity);
  writeVerbosityInfo(ts);
  writeBasisset(ts, jobParams.slaterBasisName);
  writeCifData(ts, jobParams.inputFilename, jobParams.overrideBondLengths,
               crystal->crystalName());
  writeProcessCifForSurface(ts);
  writeNameReset(ts, jobParams.inputFilename, crystal->crystalName());
  writeFragmentInfo(ts, jobParams.atoms, crystal);
  writeWavefunctionMatrix(ts, jobParams);
  writeClusterInfo(ts, jobParams);
  writeWavefunctionInfo(ts, jobParams);
  writeCreateClusterInfo(ts);
  writeSurfaceGenerationInfo(ts, jobParams);
  writeSurfaceCreationInfo(ts, jobParams);
  writeSurfacePlottingInfo(ts, jobParams);
  writeCXInfo(ts, jobParams.outputFilename);
  writeFooter(ts);
}

void TontoInterface::writeVerbosityInfo(QTextStream &ts) {
  ts << Qt::endl;
  ts << "    low_verbosity_on";
  ts << Qt::endl;
}

void TontoInterface::writeInputForWavefunctionCalculation(
    QTextStream &ts, const JobParameters &jobParams,
    DeprecatedCrystal *crystal) {
  Q_ASSERT(crystal != nullptr);

  writeHeader(ts, "Tonto input file for Wavefunction Calculation.");
  writeNameReset(ts, jobParams.inputFilename, crystal->crystalName());
  writeChargeMultiplicity(ts, jobParams.charge, jobParams.multiplicity);
  writeBasissetForWavefunction(ts, basissetDirectory(),
                               basissetName(jobParams.basisset));
  writeFragmentInfo(ts, jobParams.atoms, crystal);
  writeSCFInfo(ts, jobParams);
  writeSCFCommands(ts);
  writeSerializeMolecule(ts, jobParams, crystal->crystalName());
  writeFooter(ts);
}

void TontoInterface::writeInputForEnergyCalculation(
    QTextStream &ts, const JobParameters &jobParams,
    DeprecatedCrystal *crystal) {
  Q_ASSERT(crystal != nullptr);

  writeHeader(ts, "Tonto input file for Interaction Energy Calculation.");
  ts << "    basis_directory= \"" << basissetDirectory() << "\"" << Qt::endl;
  writeNameReset(ts, jobParams.inputFilename, crystal->crystalName());
  // writeFragmentInfo(ts, jobParams.atoms, crystal, false);
  writeFragmentGroups(ts, jobParams.atomGroups,
                      jobParams.wavefunctionTransforms);
  // writeSleazySCFCriteria(ts);
  writeEnergyCalculationInfo(ts);
  writeFooter(ts);
}

void TontoInterface::writeInputForSurfaceWithProductProperty(
    QTextStream &ts, const JobParameters &jobParams,
    DeprecatedCrystal *crystal) {
  Q_ASSERT(crystal != nullptr);

  Q_ASSERT(crystal != nullptr);

  writeHeader(ts, "Tonto input file for Surface Generation.");
  writeNameReset(ts, jobParams.inputFilename, crystal->crystalName());
  writeFragmentGroups(ts, jobParams.atomGroups,
                      jobParams.wavefunctionTransforms);
  writeChargeMultiplicity(ts, jobParams.charge, jobParams.multiplicity);
  writeVerbosityInfo(ts);
  writeBasisset(ts, jobParams.slaterBasisName);
  writeCifData(ts, jobParams.inputFilename, jobParams.overrideBondLengths,
               crystal->crystalName());
  writeProcessCifForSurface(ts);
  writeNameReset(ts, jobParams.inputFilename, crystal->crystalName());

  auto atoms = jobParams.atoms.mid(0, jobParams.atomGroups[0]);

  writeFragmentInfo(ts, atoms, crystal);
  writeWavefunctionMatrix(ts, jobParams);
  writeClusterInfo(ts, jobParams);
  writeWavefunctionInfo(ts, jobParams);
  writeCreateClusterInfo(ts);
  writeSurfaceGenerationInfo(ts, jobParams);
  writeSurfaceCreationInfo(ts, jobParams);
  writeSurfacePlottingInfo(ts, jobParams);
  writeCXInfo(ts, jobParams.outputFilename);
  writeFooter(ts);
}

void TontoInterface::writeHeader(QTextStream &ts, const QString title) {
  ts << "{" << Qt::endl;
  ts << "    ! " << title << Qt::endl;
}

QString TontoInterface::basissetDirectory() {
  return settings::readSetting(settings::keys::TONTO_BASIS_DIRECTORY)
      .toString();
}

QString TontoInterface::basissetName(BasisSet basisset) {
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
    name = "DZP";
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
    name = "INVALID BASIS SET FOR TONTO";
    break;
  }
  return name;
}

QString TontoInterface::exchangePotentialKeyword(ExchangePotential exchange) {
  QString keyword;
  switch (exchange) {
  case ExchangePotential::slater:
    keyword = "Slater";
    break;
  case ExchangePotential::x_alpha:
    keyword = "x-alpha";
    break;
  case ExchangePotential::becke88:
    keyword = "Becke88";
    break;
  case ExchangePotential::b3lyp_x:
    keyword = "b3lypgx";
    break;
  }
  return keyword;
}

QString
TontoInterface::correlationPotentialKeyword(CorrelationPotential correlation) {
  QString keyword;
  switch (correlation) {
  case CorrelationPotential::vwn:
    keyword = "VWN";
    break;
  case CorrelationPotential::lyp:
    keyword = "LYP";
    break;
  case CorrelationPotential::b3lyp_c:
    keyword = "b3lypgc";
    break;
  }
  return keyword;
}

void TontoInterface::writeBasisset(QTextStream &ts,
                                   const QString &slaterBasisName) {
  ts << Qt::endl;
  ts << "    basis_directory= \"" << basissetDirectory() << "\"" << Qt::endl;
  if (!slaterBasisName.isEmpty()) {
    ts << "    slaterbasis_name= \"" << slaterBasisName << "\"" << Qt::endl;
  }
}

void TontoInterface::writeCifData(QTextStream &ts, const QString &cifFilename,
                                  bool overrideBondLengths,
                                  const QString &dataBlockName) {
  ts << Qt::endl;
  ts << "    ! Read the CIF and data block ..." << Qt::endl;
  ts << " " << Qt::endl;
  ts << "    CIF= {" << Qt::endl;
  ts << "       file_name= \"" << cifFilename << "\"" << Qt::endl;
  if (!dataBlockName.isEmpty()) {
    ts << "       data_block_name= \"" << dataBlockName << "\"" << Qt::endl;
  }

  if (overrideBondLengths) {
    ts << "       CH_bond_length= "
       << settings::readSetting(settings::keys::CH_BOND_LENGTH).toFloat()
       << " angstrom" << Qt::endl;
    ts << "       NH_bond_length= "
       << settings::readSetting(settings::keys::NH_BOND_LENGTH).toFloat()
       << " angstrom" << Qt::endl;
    ts << "       OH_bond_length= "
       << settings::readSetting(settings::keys::OH_BOND_LENGTH).toFloat()
       << " angstrom" << Qt::endl;
    ts << "       BH_bond_length= "
       << settings::readSetting(settings::keys::BH_BOND_LENGTH).toFloat()
       << " angstrom" << Qt::endl;
  }
  ts << "    }" << Qt::endl;
}

void TontoInterface::writeProcessCifForCX(QTextStream &ts,
                                          const QString &outputFilename) {
  QString useAngstroms = TONTO_USE_ANGSTROMS ? "TRUE" : "FALSE";
  ts << "    cx_uses_angstrom= " << useAngstroms << Qt::endl;
  ts << "    CX_file_name= \"" << outputFilename << "\"" << Qt::endl;
  ts << "    process_CIF_for_CX" << Qt::endl;
}

void TontoInterface::writeProcessCifForSurface(QTextStream &ts) {
  ts << "    process_CIF" << Qt::endl;
}

void TontoInterface::writeChargeMultiplicity(QTextStream &ts, int charge,
                                             int multiplicity) {
  ts << Qt::endl;
  ts << "    charge= " << charge << Qt::endl;
  ts << "    multiplicity= " << multiplicity << Qt::endl;
}

// Reset the name in case the data block has a slash
void TontoInterface::writeNameReset(QTextStream &ts, const QString &cifFilename,
                                    const QString &crystalName) {
  ts << "    name= " << TontoInterface::calculationName(_jobParams, crystalName)
     << Qt::endl;
}

void TontoInterface::writeBasissetForWavefunction(
    QTextStream &ts, const QString &basissetDirectory,
    const QString &basisset) {
  ts << " " << Qt::endl;
  ts << "    basis_directory= \"" << basissetDirectory << "\"" << Qt::endl;
  ts << "    basis_name= " << basisset << Qt::endl;
}

void TontoInterface::writeClusterInfo(QTextStream &ts,
                                      const JobParameters &jobParams) {
  ts << Qt::endl;
  ts << "    ! We have the asymmetric unit, now make the cluster information"
     << Qt::endl;
  ts << Qt::endl;
  ts << "    cluster= {" << Qt::endl;
  if (jobParams.atomsToSuppress.size() > 0) {
    ts << "        unit_cell_atoms_to_suppress= {" << Qt::endl;
    foreach (int i, jobParams.atomsToSuppress) {
      ts << QString("           %1").arg(i + 1)
         << Qt::endl; // +1 becomes fortran arrays start at 1
    }
    ts << "        }" << Qt::endl;
    ts << "        reset_site_occupancies" << Qt::endl;
  }

  if (jobParams.surfaceType == IsosurfaceDetails::Type::Hirshfeld) {
    ts << "        generation_method= for_hirshfeld_surface" << Qt::endl;
    ts << "        atom_density_cutoff= 1.0e-8"
       << Qt::endl; // This parameter needs to be gotten from the GUI
    ts << "        defragment= FALSE" << Qt::endl;
  } else {
    // This 'generate with radius section' is needed by surfaces that want to
    // have fingeprint properties
    // i.e. di, de , dnorm mapped on them. Hence we test
    // wantFingerprintProperties.
    // For all other surfaces this generation does no harm other than slowing
    // down surface generation.
    // This is especially noticeable for void surfaces.
    if (wantFingerprintProperties(jobParams.surfaceType)) {
      ts << "        radius= 6.0 Angstrom" << Qt::endl;
      ts << "        generation_method= within_radius" << Qt::endl;
    }
    ts << "        defragment= FALSE" << Qt::endl;
  }

  ts << Qt::endl;
  ts << "        make_info" << Qt::endl;
  ts << "    }" << Qt::endl;
}

void TontoInterface::writeFragmentInfo(QTextStream &ts,
                                       const QVector<AtomId> atomIds,
                                       DeprecatedCrystal *crystal,
                                       bool isNewData) {
  ts << Qt::endl;
  ts << "    atoms= {" << Qt::endl;
  ts << "        keys= { label= { units= angstrom } pos= site_disorder_group= "
        "site_occupancy= }"
     << Qt::endl;
  if (isNewData) {
    ts << "        new_data= {" << Qt::endl;
  } else {
    ts << "        data= {" << Qt::endl;
  }

  const QVector<Atom> atoms = crystal->generateAtomsFromAtomIds(atomIds);
  for (const Atom &atom : atoms) { // Write out atoms in format:  symbol x  y  z
    QVector3D pos = atom.pos();
    QString name = atom.label();
    ts << QString("            %1    %2    %3   %4   %5   %6")
              .arg(name)
              .arg(pos.x(), 0, 'f', 6)
              .arg(pos.y(), 0, 'f', 6)
              .arg(pos.z(), 0, 'f', 6)
              .arg(atom.disorderGroup())
              .arg(atom.occupancy(), 0, 'f', 4);
    ts << Qt::endl;
  }
  ts << "        }" << Qt::endl;
  ts << "    }" << Qt::endl;
}

QString TontoInterface::getFchkFileForGroup(int i) {
  return _wavefunctionFilenames[i];
}

void TontoInterface::writeFragmentGroups(
    QTextStream &ts, const QVector<int> groups,
    const QVector<WavefunctionTransform> wavefunctionTransforms) {
  ts << Qt::endl;
  ts << "    atom_groups= {" << Qt::endl;
  ts << "        keys= { name= atom_indices= fchk_file= rotation= shift= }"
     << Qt::endl;
  ts << "        data= {" << Qt::endl;

  int offset = 0;
  for (int g = 0; g < groups.size(); ++g) {
    int groupSize = groups[g];

    // Write name= atom_indices= charge=
    ts << QString("            Group%1    { %2 ... %3 }")
              .arg(g + 1)
              .arg(offset + 1)
              .arg(offset + groupSize);
    ts << Qt::endl;

    // Write fchk_file=
    ts << QString("            \"%5\"").arg(getFchkFileForGroup(g));
    ts << Qt::endl;

    // Write rotation= shift=
    Matrix3q m = wavefunctionTransforms[g].first;
    ts << QString("           ");
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        ts << " " << m(i, j);
      }
    }
    ts << Qt::endl;

    // Write shift=
    Vector3q t = wavefunctionTransforms[g].second;
    ts << QString("            ") << t(0) << " " << t(1) << " " << t(2)
       << Qt::endl;
    ts << Qt::endl;

    offset += groupSize;
  }
  ts << "        }" << Qt::endl;
  ts << "    }" << Qt::endl;
}

void TontoInterface::writeCreateClusterInfo(QTextStream &ts) {
  ts << Qt::endl;
  ts << "    ! Now create the cluster from the stored information ..."
     << Qt::endl;
  ts << Qt::endl;
  ts << "    create_cluster" << Qt::endl;
  ts << Qt::endl;
  ts << "    put" << Qt::endl;
}

// TODO move to IsosurfaceDetails
bool TontoInterface::wantFingerprintProperties(
    IsosurfaceDetails::Type surfaceType) {
  return surfaceType == IsosurfaceDetails::Type::Hirshfeld ||
         surfaceType == IsosurfaceDetails::Type::PromoleculeDensity ||
         surfaceType == IsosurfaceDetails::Type::ElectronDensity;
}

bool TontoInterface::wantShapeProperties(IsosurfaceDetails::Type surfaceType) {
  return surfaceType == IsosurfaceDetails::Type::Hirshfeld ||
         surfaceType == IsosurfaceDetails::Type::PromoleculeDensity ||
         surfaceType == IsosurfaceDetails::Type::ElectronDensity;
}

void TontoInterface::writeSurfaceGenerationInfo(
    QTextStream &ts, const JobParameters &jobParams) {
  IsosurfaceDetails::Type surfaceType = jobParams.surfaceType;

  ts << Qt::endl;
  writeSurfaceGenerationInterpolationSettings(ts);
  ts << Qt::endl;

  ts << "    ! Generate the isosurface ..." << Qt::endl;
  ts << Qt::endl;
  ts << "    CX_surface= {" << Qt::endl;
  ts << Qt::endl;

  ts << "        kind= \""
     << IsosurfaceDetails::getAttributes(surfaceType).tontoLabel << "\""
     << Qt::endl;

  if (surfaceType == IsosurfaceDetails::Type::CrystalVoid) {
    ts << "        triangulation_method= marching_cube" << Qt::endl;
  } else {
    ts << "        triangulation_method= recursive_marching_cube" << Qt::endl;
  }
  ts << Qt::endl;

  if (surfaceType == IsosurfaceDetails::Type::Orbital ||
      surfaceType == IsosurfaceDetails::Type::DeformationDensity ||
      surfaceType == IsosurfaceDetails::Type::ElectricPotential ||
      surfaceType == IsosurfaceDetails::Type::SpinDensity) {
    ts << "        iso_values= { " << jobParams.isovalue << " "
       << -jobParams.isovalue << " }" << Qt::endl;
  } else {
    ts << "        iso_value= " << jobParams.isovalue << Qt::endl;
  }

  ts << "        surface_property= \""
     << IsosurfacePropertyDetails::getAttributes(
            jobParams.requestedPropertyType)
            .tontoName
     << "\"" << Qt::endl;
  ts << "        minimum_scan_division= " << GLOBAL_MINIMUM_SCAN_DIVISION
     << Qt::endl;
  ts << "        voxel_proximity_factor= " << GLOBAL_VOXEL_PROXIMITY_FACTOR
     << Qt::endl;
  ts << Qt::endl;

  if (wantFingerprintProperties(surfaceType)) {
    ts << "        CX_output_distance_properties= TRUE" << Qt::endl;
    ts << Qt::endl;
  }

  if (wantShapeProperties(surfaceType)) {
    ts << "        CX_output_shape_properties= TRUE" << Qt::endl;
    ts << Qt::endl;
  }

  ts << "        plot_grid= {" << Qt::endl;

  if (jobParams.surfaceType == IsosurfaceDetails::Type::CrystalVoid) {
    ts << "            use_unit_cell_as_bbox" << Qt::endl;
    ts << "            box_scale_factor= " << GLOBAL_BOUNDING_BOX_SCALE_FACTOR
       << Qt::endl;
  } else {
    ts << "            use_bounding_cube_and_axes" << Qt::endl;
    ts << "            cube_scale_factor= " << GLOBAL_CUBE_SCALE_FACTOR
       << Qt::endl;
  }

  bool useFineGrainedDesiredSeparation = false;
  if (useFineGrainedDesiredSeparation) {
    ts << "            desired_separation= " << GLOBAL_DESIRED_SEPARATION
       << Qt::endl;
  } else {
    ts << "            desired_separation= "
       << ResolutionDetails::value(jobParams.resolution) << Qt::endl;
  }

  if (jobParams.surfaceType == IsosurfaceDetails::Type::Orbital ||
      jobParams.requestedPropertyType ==
          IsosurfacePropertyDetails::Type::Orbital) {
    if (jobParams.molecularOrbitalType == HOMO) {
      ts << "            HOMO_orbital_plus= -"
         << jobParams.molecularOrbitalLevel << Qt::endl;
    } else {
      ts << "            LUMO_orbital_plus=  "
         << jobParams.molecularOrbitalLevel << Qt::endl;
    }
  }

  if (jobParams.surfaceType == IsosurfaceDetails::Type::ADP) {
    ts << "            center_atom= 1" << Qt::endl;
  }

  ts << "            put" << Qt::endl;
  ts << "        }" << Qt::endl;

  if (jobParams.surfaceType == IsosurfaceDetails::Type::CrystalVoid) {
    ts << "        cap_ends = -1" << Qt::endl;
  }
  ts << "    }" << Qt::endl;
}

void TontoInterface::writeSurfaceGenerationInterpolationSettings(
    QTextStream &ts) {
  ts << "    interpolator= {" << Qt::endl;
  ts << "        interpolation_method= " << GLOBAL_INTERPOLATION_METHOD
     << Qt::endl;
  ts << "        domain_mapping= " << GLOBAL_DOMAIN_MAPPING << Qt::endl;
  ts << "        table_eps= "
     << "1.0d" << GLOBAL_TABLE_CUTOFF << Qt::endl;
  ts << "        table_spacing= " << GLOBAL_TABLE_SPACING << Qt::endl;
  ts << "    }" << Qt::endl;
}

void TontoInterface::writeSurfaceCreationInfo(QTextStream &ts,
                                              const JobParameters &jobParams) {
  ts << Qt::endl;

  switch (jobParams.surfaceType) {
  case IsosurfaceDetails::Type::Hirshfeld:
    ts << "    ! Do the Stockholder isosurface on with the current cluster ..."
       << Qt::endl;
    ts << Qt::endl;
    ts << "    slaterbasis_name= " << jobParams.slaterBasisName << Qt::endl;
    ts << "    isosurface_plot" << Qt::endl;
    break;
  case IsosurfaceDetails::Type::PromoleculeDensity:
  case IsosurfaceDetails::Type::CrystalVoid:
    ts << Qt::endl;
    ts << "    saved_slaterbasis_name= " << jobParams.slaterBasisName
       << Qt::endl;
    ts << "    saved_isosurface_plot " << Qt::endl;
    break;
  case IsosurfaceDetails::Type::ADP:
    ts << Qt::endl;
    ts << "    isosurface_plot " << Qt::endl;
    break;
  default:
    ts << "    ! Do the Quantum Isosurface for the saved user selected "
          "fragment ..."
       << Qt::endl;
    ts << Qt::endl;

    if (jobParams.program == ExternalProgram::Tonto) {
      ts << "    saved_basis_name= " << basissetName(jobParams.basisset)
         << Qt::endl;
    }
    ts << "    saved_isosurface_plot " << Qt::endl;
  }
}

void TontoInterface::writeSurfacePlottingInfo(QTextStream &ts,
                                              const JobParameters &jobParams) {
  ts << Qt::endl;

  switch (jobParams.requestedPropertyType) {
  case IsosurfacePropertyDetails::Type::None: // do nothing for none.
    break;
  // case stockholder_weight:
  //	ts << "    ! Plot the Stockholder function on the previously calculated
  // isosurface ..." << Qt::endl;
  //	ts << "    slaterbasis_name= " << IP.hirshfeld_basis_set << Qt::endl;
  //	ts << "    plot_on_isosurface" << Qt::endl;
  //	break;
  case IsosurfacePropertyDetails::Type::PromoleculeDensity:
    // case crystalVoids: // <- surface type NOT property
    ts << Qt::endl;
    // ts << "    saved_slaterbasis_nameplotgrid= " << IP.hirshfeld_basis_set <<
    // Qt::endl;
    ts << "    saved_plot_on_isosurface" << Qt::endl;
    break;
  default:
    ts << "    ! Plot the surface property for the saved user selected "
          "fragment on the"
       << Qt::endl;
    ts << "    ! previously calculated isosurface ..." << Qt::endl;
    ts << Qt::endl;
    if (jobParams.program == ExternalProgram::Tonto) {
      ts << "    saved_basis_name= " << basissetName(jobParams.basisset)
         << Qt::endl;
    }
    ts << "    saved_plot_on_isosurface" << Qt::endl;
    break;
  }
}

void TontoInterface::writeCXInfo(QTextStream &ts,
                                 const QString &outputFilename) {
  bool use_sbf =
      settings::readSetting(settings::keys::USE_SBF_INTERFACE).toBool();
  ts << Qt::endl;
  ts << "    ! Write out the results for the GUI" << Qt::endl;
  ts << Qt::endl;
  ts << "    cx_uses_angstrom = TRUE" << Qt::endl;
  ts << "    CX_file_name= \"" << outputFilename << "\"" << Qt::endl;
  if (use_sbf)
    ts << "    serialize_isosurface " << Qt::endl;
  else
    ts << "    put_CX_data " << Qt::endl;
}

QString TontoInterface::shellKindForHartreeFock(int multiplicity) {
  Q_ASSERT(multiplicity > 0);

  return (multiplicity == 1) ? "rhf" : "uhf";
}

QString TontoInterface::shellKindForKohnSham(int multiplicity) {
  Q_ASSERT(multiplicity > 0);

  return (multiplicity == 1) ? "rks" : "uks";
}

void TontoInterface::writeSCFInfo(QTextStream &ts,
                                  const JobParameters &jobParams) {
  ts << Qt::endl;
  ts << "    scfdata= {" << Qt::endl;
  ts << "        initial_density= promolecule" << Qt::endl;

  switch (jobParams.theory) {
  case Method::hartreeFock:
  case Method::mp2:
    ts << "        kind= " << shellKindForHartreeFock(jobParams.multiplicity)
       << Qt::endl;
    break;
  case Method::b3lyp:
    ts << "        kind= " << shellKindForKohnSham(jobParams.multiplicity)
       << Qt::endl;
    ts << "        dft_exchange_functional= b3lypgx" << Qt::endl;
    ts << "        dft_correlation_functional= b3lypgc" << Qt::endl;
    break;
  case Method::kohnSham:
    ts << "        kind= " << shellKindForKohnSham(jobParams.multiplicity)
       << Qt::endl;
    ts << "        dft_exchange_functional= "
       << exchangePotentialKeyword(jobParams.exchangePotential) << Qt::endl;
    ts << "        dft_correlation_functional= "
       << correlationPotentialKeyword(jobParams.correlationPotential)
       << Qt::endl;
    break;
  default:
    ts << "ERROR Unknown level of theory for TONTO" << Qt::endl;
    break;
  }

  ts << "        direct= on" << Qt::endl;
  ts << "    }" << Qt::endl;
}

void TontoInterface::writeSCFCommands(QTextStream &ts) {
  ts << Qt::endl;
  ts << "    delete_scf_integrals" << Qt::endl;
  ts << "    delete_scf_archives" << Qt::endl;
  ts << Qt::endl;
  ts << "    scf" << Qt::endl;
  ts << Qt::endl;
  ts << "    cleanup_scf" << Qt::endl;
  ts << "    delete_scf_integrals" << Qt::endl;
}

void TontoInterface::writeWavefunctionMatrix(QTextStream &ts,
                                             const JobParameters &jobParams) {
  if (jobParams.program == ExternalProgram::None) {
    return;
  }

  Q_ASSERT(jobParams.wavefunctionTransforms.size() == 1);

  Matrix3q m = jobParams.wavefunctionTransforms[0].first;
  Vector3q t = jobParams.wavefunctionTransforms[0].second;

  if (m.isIdentity() && t.isZero()) {
    return;
  }

  ts << "    rotate_with_matrix";
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      ts << " " << m(i, j);
    }
  }
  ts << Qt::endl;
  ts << "    move_origin " << t(0) << " " << t(1) << " " << t(2) << " angstrom"
     << Qt::endl;
}

void TontoInterface::writeWavefunctionInfo(QTextStream &ts,
                                           const JobParameters &jobParams) {
  switch (jobParams.program) {
  case ExternalProgram::None:
    break;
  case ExternalProgram::Tonto:
    writeTontoWavefunction(ts, jobParams);
    break;
  case ExternalProgram::Gaussian:
    writeGaussianWavefunction(ts, TontoInterface::wavefunctionFilename());
    break;
  case ExternalProgram::Psi4:
    writeGaussianWavefunction(ts, TontoInterface::wavefunctionFilename());
    break;
  case ExternalProgram::Occ:
    writeGaussianWavefunction(ts, TontoInterface::wavefunctionFilename());
    break;
  case ExternalProgram::NWChem:
    writeMoldenWavefunction(ts, TontoInterface::wavefunctionFilename());
    break;
  default:
    break;
  }

  ts << "    slaterbasis_name= " << jobParams.slaterBasisName << Qt::endl;
}

void TontoInterface::writeGaussianWavefunction(
    QTextStream &ts, const QString &wavefunctionFilename) {
  ts << getReadFchkCommand() << wavefunctionFilename << Qt::endl;
}

void TontoInterface::writeMoldenWavefunction(
    QTextStream &ts, const QString &wavefunctionFilename) {
  ts << getReadMoldenCommand() << wavefunctionFilename << Qt::endl;
}

QString TontoInterface::getReadFchkCommand() {
  QString retval;
  switch (GaussianInterface::getGaussianVersion()) {
  case g98:
    retval = "    read_g98_fchk_file ";
    break;
  case g03:
    retval = "    read_g03_fchk_file ";
    break;
  case g09:
    retval = "    read_g09_fchk_file ";
    break;
  default:
    retval = "    read_g09_fchk_file ";
    break;
  }
  return retval;
}

QString TontoInterface::getReadMoldenCommand() {
  return "    read_molden_mos ";
}

void TontoInterface::writeTontoWavefunction(QTextStream &ts,
                                            const JobParameters &jobParams) {
  ts << Qt::endl;
  ts << "    ! Read in the previously calculated MO's. Assign the NO's."
     << Qt::endl;
  ts << "    ! Calculate the AO density matrix for electrostatic plots."
     << Qt::endl;
  ts << Qt::endl;
  ts << "    basis_name= " << basissetName(jobParams.basisset) << Qt::endl;
  ts << Qt::endl;

  if (jobParams.multiplicity == 1) { // restricted for singlet states
    ts << "    read_archive molecular_orbitals restricted" << Qt::endl;
    ts << "    assign_NOs_to_MOs" << Qt::endl;
    ts << "    scfdata = { kind = rhf }" << Qt::endl;
  } else { // if multiplicity > 1 then treat as unrestricted
    ts << "    read_archive molecular_orbitals unrestricted" << Qt::endl;
    ts << "    assign_NOs_to_MOs" << Qt::endl;
    ts << "    scfdata = { kind = uhf }" << Qt::endl;
  }
  ts << "    make_scf_density_matrix" << Qt::endl;
  ts << "    make_ao_density_matrix" << Qt::endl;
}

void TontoInterface::writeSerializeMolecule(QTextStream &ts,
                                            const JobParameters &jobParams,
                                            const QString &crystalName) {
  ts << Qt::endl;
  ts << "    serialize= " << tontoSBFName(jobParams, crystalName);
}

void TontoInterface::writeSleazySCFCriteria(QTextStream &ts) {
  ts << "    scfdata= {" << Qt::endl;
  ts << "        eri_schwarz_cutoff= 1.0e-6" << Qt::endl;
  ts << "        diis= {" << Qt::endl;
  ts << "           convergence_tolerance= 0.005";
  ts << "        }" << Qt::endl;
  ts << "    }" << Qt::endl;
}

void TontoInterface::writeEnergyCalculationInfo(QTextStream &ts) {
  ts << Qt::endl;
  ts << "    put_group_12_polarization_energy" << Qt::endl;
  ts << "    put_group_12_energies" << Qt::endl;
  ts << "    put_group_12_grimme2006_energy" << Qt::endl;
}

void TontoInterface::writeFooter(QTextStream &ts) {
  ts << " " << Qt::endl;
  ts << "}" << Qt::endl;
}
