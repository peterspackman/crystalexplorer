#include "occinterface.h"
#include <QMessageBox>
#include "settings.h"
#include <QJsonDocument>

OccInterface::OccInterface(QWidget *parent)
    : QObject(parent), m_parentWidget(parent), m_process(new QProcess()) {
  m_inputEditor = new FileEditor();
  connect(m_inputEditor, &FileEditor::writtenFileToDisk, this,
          &OccInterface::runProcess, Qt::UniqueConnection);
  connect(m_process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &OccInterface::jobFinished, Qt::UniqueConnection);
  connect(m_process, &QProcess::stateChanged, this, &OccInterface::jobState,
          Qt::UniqueConnection);
}

void OccInterface::prejobSetup() {
  // do thing slike copy wavefunctions etc.
  m_wavefunctionFilenames.clear();
  int id = 0;
  for (const auto &wavefunction : m_currentWavefunctions) {
    QString filename =
        wavefunction.restoreWavefunctionFile(m_workingDirectory, id);
    if (filename.isEmpty()) {
      QMessageBox::warning(m_parentWidget, tr("Error"),
                           tr("Unable to restore wavefunction files."));
      return;
    }
    m_wavefunctionFilenames.push_back(filename);
    id++;
  }
}

void OccInterface::runJob(const JobParameters &jobParams,
                          DeprecatedCrystal *crystal,
                          const QVector<Wavefunction> &wavefunctions) {
  m_currentJobParams = jobParams;
  m_currentWavefunctions = wavefunctions;

  prejobSetup();
  setCurrentJobNameFromCrystal(crystal);
  qDebug() << "OccInterface::runJob = "
           << ((m_currentJobParams.jobType == JobType::wavefunction)
                   ? "wavefunction"
                   : "energy");
  QString errorMsg;

  if (isExecutableInstalled()) {
    if (writeInputfile(crystal)) {
      if (m_currentJobParams.editInputFile) {
        editInputFile();
      } else {
        runProcess();
      }
    } else {
      errorMsg = failedWritingInputfileMsg();
    }
  } else {
    errorMsg = execMissingMsg();
  }

  if (!errorMsg.isNull()) {
    QMessageBox::warning(m_parentWidget, errorTitle(), errorMsg);
  }
}

void OccInterface::editInputFile() {
  m_inputEditor->insertFile(fullInputFilename());
  m_inputEditor->show();
}

void OccInterface::runProcess() {
  m_cycle = 0;
  m_processStoppedByUser = false;

  m_process->setWorkingDirectory(m_workingDirectory);
  m_process->setProcessEnvironment(getEnvironment());
  if (redirectStdoutToOutputFile()) {
    m_process->setStandardOutputFile(outputFilename());
  }
  m_process->start(program(), commandline(m_currentJobParams));
}

void OccInterface::jobState(QProcess::ProcessState state) {
  switch (state) {
  case QProcess::NotRunning:
    break;
  case QProcess::Starting:
    break;
  case QProcess::Running:
    emit updateProgressBar(m_currentJobParams.step, m_currentJobParams.maxStep);
    emit updateStatusMessage(jobDescription(m_currentJobParams.jobType,
                                            m_currentJobParams.maxStep,
                                            m_currentJobParams.step));
    emit processRunning();
    break;
  }
}

QProcessEnvironment OccInterface::getEnvironment() {
  auto env = QProcessEnvironment::systemEnvironment();
  env.insert(
      "OCC_BASIS_PATH",
      settings::readSetting(settings::keys::OCC_BASIS_DIRECTORY).toString());
  env.insert("OMP_NUM_THREADS", "1");
  return env;
}

QString OccInterface::jobDescription(JobType jobType, int maxStep, int step) {
  QString description(jobProcessDescription(jobType));

  if (maxStep > 0) {
    description += QString(" (%1/%2)").arg(step).arg(maxStep);
  }
  return description;
}

void OccInterface::jobFinished(int exitCode, QProcess::ExitStatus exitStatus) {
  Q_UNUSED(exitCode);
  qDebug() << "Job finished: type = "
           << ((m_currentJobParams.jobType == JobType::wavefunction)
                   ? "wavefunction"
                   : "energy");
  if (m_processStoppedByUser) {
    return;
  }

  // We want to tidy up by deleting the QProcess but it seems that if the
  // program genuinely crashed
  // then the QProcess is automatically deleted hence the else block.
  if (exitStatus == QProcess::CrashExit) {
    QMessageBox::warning(m_parentWidget,
                         jobErrorMessage(m_currentJobParams.jobType),
                         execCrashMsg());
    return;
  }

  bool foundError = errorInOutput();
  emit processFinished(foundError, m_currentJobParams.jobType);
}

bool OccInterface::errorInOutput() {
  QFile file(outputFilePath());
  if (file.open(QIODevice::ReadOnly)) {
    QTextStream ts(&file);
    while (!ts.atEnd()) {
      QString line = ts.readLine();
      if (line.contains(normalTerminationHook(), Qt::CaseInsensitive)) {
        return false;
      }
    }
  }
  return true;
}

void OccInterface::stopJob() {
  if (m_process == nullptr)
    return;
  if (m_process->state() == QProcess::Running) {
    m_process->kill();
  }

  if (m_process->state() == QProcess::Running) {
    QMessageBox::information(
        m_parentWidget, "Unable to terminate occ process!\n",
        "You may need to manually kill your occ process.", QMessageBox::Ok);
  } else {
    m_processStoppedByUser = true;
    emit processCancelled(processCancellationMsg());
  }
}

void OccInterface::setWorkingDirectory(const QString &filename) {
  QFileInfo fileInfo(filename);
  m_workingDirectory = fileInfo.absolutePath();
}

const QString &OccInterface::workingDirectory() const {
  return m_workingDirectory;
}

void OccInterface::setCurrentJobNameFromCrystal(DeprecatedCrystal *crystal) {
  QFileInfo f(crystal->cifFilename());
  QString cifFilename = f.baseName();
  QString crystalName = crystal->crystalName();
  m_currentJobName = cifFilename + "_" + crystalName;
}

QString OccInterface::fullInputFilename() {
  Q_ASSERT(!workingDirectory().isEmpty());
  QString inputFile = workingDirectory() + QDir::separator() + inputFilename();
  return inputFile;
}

bool OccInterface::writeInputfile(DeprecatedCrystal *crystal) {
  Q_ASSERT(crystal != nullptr);

  bool success = false;

  QString filename = inputFilename();
  m_currentJobParams.QMInputFilename = filename;

  // Remove existing input and output file names
  if (QFile::exists(filename)) {
    QFile::remove(filename);
  }

  QFile inputFile(filename);
  if (inputFile.open(QIODevice::WriteOnly)) {
    QTextStream ts(&inputFile);
    switch (m_currentJobParams.jobType) {
    case JobType::wavefunction:
      writeInputForWavefunctionCalculation(ts, m_currentJobParams, crystal);
      break;
    case JobType::pairEnergy:
      writeInputForPairEnergyCalculation(ts, m_currentJobParams);
      break;
    default:
      break; // should never happen
    }
    inputFile.close();
    success = true;
  }
  return success;
}

QString OccInterface::outputFilePath() {
  Q_ASSERT(!workingDirectory().isEmpty());
  return workingDirectory() + QDir::separator() + outputFilename();
}

QString OccInterface::inputFilePath() {
  Q_ASSERT(!workingDirectory().isEmpty());

  return workingDirectory() + QDir::separator() +
         m_currentJobParams.QMInputFilename;
}

QString OccInterface::outputFilename() {
  QString extension = "occ_stdout";
  return m_currentJobName + "." + extension;
}

QString OccInterface::errorTitle() { return "Error running " + programName(); }

QString OccInterface::failedWritingInputfileMsg() {
  return "Unable to write " + programName() + " input file.";
}

QString OccInterface::execMissingMsg() {
  return "Unable to find " + programName() + " executable. Check the " +
         programName() + " path is set correctly in the preferences.";
}

QString OccInterface::execRunningMsg() {
  return programName() + " wavefunction calculation in progress...";
}

QString OccInterface::execFailedMsg() {
  return programName() + " failed to run.";
}

QString OccInterface::execCrashMsg() {
  return programName() + " crashed unexpectedly.";
}

QString OccInterface::processCancellationMsg() {
  return programName() + " job terminated.";
}

bool OccInterface::executableInstalled() { return QFile::exists(executable()); }

QString OccInterface::executable() {
  return settings::readSetting(settings::keys::OCC_EXECUTABLE).toString();
}

QString OccInterface::program() { return executable(); }

QString OccInterface::programName() { return executable(); }

QStringList OccInterface::commandline(const JobParameters &jobParams) {
    QString subcommand = "scf";
    switch(jobParams.jobType) {
    case JobType::wavefunction:
        break;
    case JobType::pairEnergy:
        subcommand = "pair";
        break;
    default:
        break;
    }
    return {subcommand, jobParams.QMInputFilename};
}

QString OccInterface::inputFilename() {
  m_currentInputFilename = m_currentJobName + ".json";
  return m_currentInputFilename;
}

QString OccInterface::calculationName(QString cifFilename,
                                      QString crystalName) {
  Q_ASSERT(!crystalName.contains("/"));

  QFileInfo fileInfo(cifFilename);
  QString name = fileInfo.baseName().replace(" ", "_");
  return name + "_" + crystalName;
}

QString OccInterface::wavefunctionFilename(const JobParameters &jobParams,
                                           QString crystalName) {
  QString calcName = calculationName(jobParams.inputFilename, crystalName);
  QString suffix = defaultFchkFileExtension();
  return calcName + suffix;
}

QString OccInterface::basissetName(BasisSet basis) {
  switch (basis) {
  case BasisSet::STO_3G:
    return "STO-3G";
  case BasisSet::Pople3_21G:
    return "3-21G";
  case BasisSet::Pople6_31Gd:
    return "6-31G*";
  case BasisSet::Pople6_31Gdp:
    return "6-31G**";
  case BasisSet::Pople6_311Gdp:
    return "6-311G**";
  case BasisSet::CC_PVDZ:
    return "cc-pvdz";
  case BasisSet::CC_PVTZ:
    return "cc-pvtz";
  case BasisSet::CC_PVQZ:
    return "cc-pvqz";
  default:
    return "";
  }
}

QString OccInterface::methodName(const JobParameters &jobParams) {
  switch (jobParams.theory) {
  case Method::b3lyp:
    return "b3lyp";
  case Method::hartreeFock:
    return (jobParams.multiplicity == 1) ? "rhf" : "uhf";
  default:
    return "unknown";
  }
}

/*
 * Example pair input
    {
        "driver": "pair_energy",
        "name": "formamide",
        "monomers": [
            {
                "source": "formamide.fchk",
                "rotation": [
                    [1.0, 0.0, 0.0],
                    [0.0, 1.0, 0.0],
                    [0.0, 0.0, 1.0]
                ],
                "translation": [
                    0.0, 0.0, 0.0
                ]
            },
            {
                "source": "formamide.fchk",
                "rotation": [
                    [-1.0,  0.0,  0.0],
                    [ 0.0, -1.0,  0.0],
                    [ 0.0,  0.0, -1.0]
                ],
                "translation": [
                    3.604, 0.0, 0.0
                ]
            }
        ]
    }
 */
void OccInterface::writeInputForPairEnergyCalculation(
    QTextStream &ts, const JobParameters &jobParams) {
  Q_ASSERT(m_wavefunctionFilenames.size() > 1);
  QJsonObject json;
  json["name"] = m_currentJobName;
  json["driver"] = "pair_energy";
  json["threads"] = settings::readSetting(settings::keys::OCC_NTHREADS).toInt();
  QJsonArray monomers;
  for (size_t i = 0; i < 2; i++) {
    const auto &transform = jobParams.wavefunctionTransforms[i];
    QJsonArray rotation;
    for (auto x : transform.first.rowwise()) {
      rotation.push_back(QJsonArray({x(0), x(1), x(2)}));
    }
    QJsonArray translation(
        {transform.second(0), transform.second(1), transform.second(2)});
    monomers.append(QJsonObject({{"source", m_wavefunctionFilenames[i]},
                                 {"rotation", rotation},
                                 {"translation", translation}}));
  }
  json["monomers"] = monomers;
  QJsonDocument jsonDocument(json);
  ts << jsonDocument.toJson();
}

void OccInterface::writeInputForWavefunctionCalculation(
    QTextStream &ts, const JobParameters &jobParams,
    DeprecatedCrystal *crystal) {
  QJsonObject json;
  json["name"] = m_currentJobName;
  json["driver"] = "energy";
  json["threads"] = settings::readSetting(settings::keys::OCC_NTHREADS).toInt();
  QJsonObject molecule;
  QJsonArray symbols, geometry;
  auto atoms = crystal->generateAtomsFromAtomIds(jobParams.atoms);
  for (const auto &atom : atoms) {
    symbols.push_back(atom.element()->capitalizedSymbol());
    const QVector3D pos = atom.pos();
    geometry.push_back(pos.x());
    geometry.push_back(pos.y());
    geometry.push_back(pos.z());
  }
  molecule["symbols"] = symbols;
  molecule["geometry"] = geometry;
  molecule["molecular_multiplicity"] = jobParams.multiplicity;
  json["molecule"] = molecule;

  QJsonObject model;
  model["method"] = methodName(jobParams);
  model["basis"] = basissetName(jobParams.basisset);
  json["model"] = model;

  QJsonDocument jsonDocument(json);
  ts << jsonDocument.toJson();
}
