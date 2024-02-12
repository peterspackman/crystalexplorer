#include "orcainterface.h"
#include <QMessageBox>
#include "settings.h"

OrcaInterface::OrcaInterface(QWidget *parent)
    : QObject(parent), m_parentWidget(parent), m_process(new QProcess()) {
  m_inputEditor = new FileEditor();
  connect(m_inputEditor, &FileEditor::writtenFileToDisk, this,
          &OrcaInterface::runProcess, Qt::UniqueConnection);
  connect(m_process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &OrcaInterface::jobFinished, Qt::UniqueConnection);
  connect(m_process, &QProcess::stateChanged, this, &OrcaInterface::jobState,
          Qt::UniqueConnection);
}

void OrcaInterface::prejobSetup() {
  // currently do nothing
}

void OrcaInterface::runJob(const JobParameters &jobParams,
                           DeprecatedCrystal *crystal,
                           const QVector<Wavefunction> &wavefunctions) {
  m_currentJobParams = jobParams;
  m_currentWavefunctions = wavefunctions;

  prejobSetup();
  setCurrentJobNameFromCrystal(crystal);
  qDebug() << "OrcaInterface::runJob = "
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

void OrcaInterface::editInputFile() {
  m_inputEditor->insertFile(fullInputFilename());
  m_inputEditor->show();
}

void OrcaInterface::runProcess() {
  m_cycle = 0;
  m_processStoppedByUser = false;

  m_process->setWorkingDirectory(m_workingDirectory);
  m_process->setProcessEnvironment(getEnvironment());
  if (redirectStdoutToOutputFile()) {
    m_process->setStandardOutputFile(outputFilename());
  }
  m_process->start(program(), commandline(m_currentJobParams));
}

void OrcaInterface::jobState(QProcess::ProcessState state) {
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

QProcessEnvironment OrcaInterface::getEnvironment() {
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  QString exe = executable();
  QString directory = QFileInfo(exe).dir().absolutePath();
  env.insert("LD_LIBRARY_PATH", directory);
  return env;
}

QString OrcaInterface::jobDescription(JobType jobType, int maxStep, int step) {
  QString description(jobProcessDescription(jobType));

  if (maxStep > 0) {
    description += QString(" (%1/%2)").arg(step).arg(maxStep);
  }
  return description;
}

void OrcaInterface::jobFinished(int exitCode, QProcess::ExitStatus exitStatus) {
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

bool OrcaInterface::errorInOutput() {
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

void OrcaInterface::stopJob() {
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

void OrcaInterface::setWorkingDirectory(const QString &filename) {
  QFileInfo fileInfo(filename);
  m_workingDirectory = fileInfo.absolutePath();
}

const QString &OrcaInterface::workingDirectory() const {
  return m_workingDirectory;
}

void OrcaInterface::setCurrentJobNameFromCrystal(DeprecatedCrystal *crystal) {
  QFileInfo f(crystal->cifFilename());
  QString cifFilename = f.baseName();
  QString crystalName = crystal->crystalName();
  m_currentJobName = cifFilename + "_" + crystalName;
}

QString OrcaInterface::fullInputFilename() {
  Q_ASSERT(!workingDirectory().isEmpty());
  QString inputFile = workingDirectory() + QDir::separator() + inputFilename();
  return inputFile;
}

bool OrcaInterface::writeInputfile(DeprecatedCrystal *crystal) {
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
      writeInputForPairEnergyCalculation(ts, m_currentJobParams, crystal);
      break;
    default:
      break; // should never happen
    }
    inputFile.close();
    success = true;
  }
  return success;
}

QString OrcaInterface::outputFilePath() {
  Q_ASSERT(!workingDirectory().isEmpty());
  return workingDirectory() + QDir::separator() + outputFilename();
}

QString OrcaInterface::inputFilePath() {
  Q_ASSERT(!workingDirectory().isEmpty());

  return workingDirectory() + QDir::separator() +
         m_currentJobParams.QMInputFilename;
}

QString OrcaInterface::outputFilename() {
  QString extension = "orca_stdout";
  return m_currentJobName + "." + extension;
}

QString OrcaInterface::errorTitle() { return "Error running " + programName(); }

QString OrcaInterface::failedWritingInputfileMsg() {
  return "Unable to write " + programName() + " input file.";
}

QString OrcaInterface::execMissingMsg() {
  return "Unable to find " + programName() + " executable. Check the " +
         programName() + " path is set correctly in the preferences.";
}

QString OrcaInterface::execRunningMsg() {
  return programName() + " wavefunction calculation in progress...";
}

QString OrcaInterface::execFailedMsg() {
  return programName() + " failed to run.";
}

QString OrcaInterface::execCrashMsg() {
  return programName() + " crashed unexpectedly.";
}

QString OrcaInterface::processCancellationMsg() {
  return programName() + " job terminated.";
}

bool OrcaInterface::executableInstalled() {
  return QFile::exists(executable());
}

QString OrcaInterface::executable() {
  return settings::readSetting(settings::keys::ORCA_EXECUTABLE).toString();
}

QString OrcaInterface::program() { return executable(); }

QString OrcaInterface::programName() { return executable(); }

QStringList OrcaInterface::commandline(const JobParameters &jobParams) {
  return {jobParams.QMInputFilename};
}

QString OrcaInterface::inputFilename() {
  m_currentInputFilename = m_currentJobName + ".inp";
  return m_currentInputFilename;
}

QString OrcaInterface::calculationName(QString cifFilename,
                                       QString crystalName) {
  Q_ASSERT(!crystalName.contains("/"));

  QFileInfo fileInfo(cifFilename);
  QString name = fileInfo.baseName().replace(" ", "_");
  return name + "_" + crystalName;
}

QString OrcaInterface::wavefunctionFilename(const JobParameters &jobParams,
                                            QString crystalName) {
  QString calcName = calculationName(jobParams.inputFilename, crystalName);
  QString suffix = defaultFchkFileExtension();
  return calcName + suffix;
}

QString OrcaInterface::basissetName(BasisSet basis) {
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

QString OrcaInterface::methodName(const JobParameters &jobParams) {
  switch (jobParams.theory) {
  case Method::b3lyp:
    return "b3lyp";
  case Method::hartreeFock:
    return (jobParams.multiplicity == 1) ? "rhf" : "uhf";
  default:
    return "unknown";
  }
}

void OrcaInterface::writeInputForPairEnergyCalculation(
    QTextStream &ts, const JobParameters &jobParams,
    DeprecatedCrystal *crystal) {
  ts << "! dlpno-ccsd(t) def2-TZVP def2-TZVP/C def2/j rijcosx verytightscf "
        "TightPNO LED"
     << Qt::endl;
  ts << "* xyz " << jobParams.charge << " " << jobParams.multiplicity
     << Qt::endl;
  auto atoms = crystal->generateAtomsFromAtomIds(jobParams.atoms);
  int numA = jobParams.atomGroups[0];
  int idx = 0;
  for (const auto &atom : std::as_const(atoms)) {
    const auto &pos = atom.pos();
    ts << atom.element()->symbol() << "(" << ((idx >= numA) ? 2 : 1) << ") "
       << pos[0] << " " << pos[1] << " " << pos[2] << Qt::endl;
    idx++;
  }
  ts << "*" << Qt::endl;
}

void OrcaInterface::writeInputForWavefunctionCalculation(
    QTextStream &ts, const JobParameters &jobParams,
    DeprecatedCrystal *crystal) {
  Q_ASSERT(false);
}
