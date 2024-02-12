#include "quantumchemistryinterface.h"
#include <QMessageBox>

QuantumChemistryInterface::QuantumChemistryInterface(QWidget *parent)
    : QObject(), _process(new QProcess()) {
  _parent = parent;
  init();
}

QuantumChemistryInterface::~QuantumChemistryInterface() { delete _inputEditor; }

void QuantumChemistryInterface::init() {
  _inputEditor = new FileEditor();
  connect(_inputEditor, SIGNAL(writtenFileToDisk()), this,
          SLOT(createProcessAndRun()));
  connect(_process, SIGNAL(finished(int, QProcess::ExitStatus)), this,
          SLOT(jobFinished(int, QProcess::ExitStatus)));
  connect(_process, SIGNAL(stateChanged(QProcess::ProcessState)), this,
          SLOT(jobState(QProcess::ProcessState)));
}

void QuantumChemistryInterface::runJob(const JobParameters &jobParams,
                                       DeprecatedCrystal *crystal) {
  _jobParams = jobParams;

  prejobSetup();
  setJobName(crystal);

  QString errorMsg;

  if (isExecutableInstalled()) {
    if (writeInputfile(crystal)) {
      if (_jobParams.editInputFile) {
        editInputFile();
      } else {
        createProcessAndRun();
      }
    } else {
      errorMsg = failedWritingInputfileMsg();
    }
  } else {
    errorMsg = execMissingMsg();
  }

  if (!errorMsg.isNull()) {
    QMessageBox::warning(_parent, errorTitle(), errorMsg);
  }
}

void QuantumChemistryInterface::editInputFile() {
  _inputEditor->insertFile(fullInputFilename());
  _inputEditor->show();
}

void QuantumChemistryInterface::createProcessAndRun() {
  _cycle = 0;
  _processStoppedByUser = false;
  _process->setWorkingDirectory(_workingDirectory);
  _process->setProcessEnvironment(getEnvironment());
  if (redirectStdoutToOutputFile()) {
    qDebug() << "Redirecting stdout to file: " << outputFilename();
    _process->setStandardOutputFile(outputFilename());
  }
  _process->start(program(), commandline(_jobParams));

  // QMessageBox::warning(_parent, errorTitle(), execFailedMsg());
}

bool QuantumChemistryInterface::redirectStdoutToOutputFile() { return false; }

void QuantumChemistryInterface::jobState(QProcess::ProcessState state) {
  switch (state) {
  case QProcess::NotRunning:
    break;
  case QProcess::Starting:
    break;
  case QProcess::Running:
    emit updateStatusMessage(execRunningMsg());
    emit processRunning();
    break;
  }
}

QProcessEnvironment QuantumChemistryInterface::getEnvironment() {
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

  return env;
}

void QuantumChemistryInterface::jobFinished(int exitCode,
                                            QProcess::ExitStatus exitStatus) {
  Q_UNUSED(exitCode);

  qDebug() << "Job finished";
  if (_processStoppedByUser) {
    return;
  }

  // We want to tidy up by deleting the QProcess but it seems that if the
  // program genuinely crashed
  // then the QProcess is automatically deleted hence the else block.
  if (exitStatus == QProcess::CrashExit) {
    QMessageBox::warning(_parent, jobErrorMessage(_jobParams.jobType),
                         execCrashMsg());
    return;
  }
  bool foundError = errorInOutput();
  emit processFinished(foundError);
  if (!foundError) {
    emit wavefunctionDone();
  }
}

bool QuantumChemistryInterface::errorInOutput() {
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

void QuantumChemistryInterface::stopJob() {

  if (_process && _process->state() == QProcess::Running) {
    _process->kill();
  }

  if (_process && _process->state() == QProcess::Running) {
    QMessageBox::information(_parent, "Unable to terminate Gaussian",
                             "Gaussian uses worker processes, which are not "
                             "always terminated when the main Gaussian process "
                             "is killed.\n\nYou may need to manually kill your "
                             "Gaussian job.",
                             QMessageBox::Ok);
  } else {
    _processStoppedByUser = true;
    emit processCancelled(processCancellationMsg());
  }
}

void QuantumChemistryInterface::setWorkingDirectory(const QString filename) {
  QFileInfo fileInfo(filename);
  _workingDirectory = fileInfo.absolutePath();
}

QString QuantumChemistryInterface::workingDirectory() {
  return _workingDirectory;
}

void QuantumChemistryInterface::setJobName(DeprecatedCrystal *crystal) {
  QFileInfo fi(crystal->cifFilename());
  QString cifFilename = fi.baseName();
  QString crystalName = crystal->crystalName();

  _jobName = cifFilename + "_" + crystalName;
}

QString QuantumChemistryInterface::fullInputFilename() {
  Q_ASSERT(!workingDirectory().isEmpty());

  QString inputFile = workingDirectory() + QDir::separator() + inputFilename();
  return inputFile;
}

bool QuantumChemistryInterface::writeInputfile(DeprecatedCrystal *crystal) {
  Q_ASSERT(crystal != nullptr);

  bool success = false;

  QString filename = inputFilename();
  _jobParams.QMInputFilename = filename;

  // Remove existing input and output file names
  if (QFile::exists(filename)) {
    QFile::remove(filename);
  }

  QFile inputFile(filename);
  if (inputFile.open(QIODevice::WriteOnly)) {
    QTextStream ts(&inputFile);
    writeInputForWavefunctionCalculation(ts, _jobParams, crystal);
    inputFile.close();
    success = true;
  }
  return success;
}

QString QuantumChemistryInterface::outputFilePath() {
  Q_ASSERT(!workingDirectory().isEmpty());

  return workingDirectory() + QDir::separator() + outputFilename();
}

QString QuantumChemistryInterface::inputFilePath() {
  Q_ASSERT(!workingDirectory().isEmpty());

  return workingDirectory() + QDir::separator() + _jobParams.QMInputFilename;
}

QString QuantumChemistryInterface::errorTitle() {
  return "Error running " + programName();
}

QString QuantumChemistryInterface::failedWritingInputfileMsg() {
  return "Unable to write " + programName() + " input file.";
}

QString QuantumChemistryInterface::execMissingMsg() {
  return "Unable to find " + programName() + " executable. Check the " +
         programName() + " path is set correctly in the preferences.";
}

QString QuantumChemistryInterface::execRunningMsg() {
  return programName() + " wavefunction calculation in progress...";
}

QString QuantumChemistryInterface::execFailedMsg() {
  return programName() + " failed to run.";
}

QString QuantumChemistryInterface::execCrashMsg() {
  return programName() + " crashed unexpectedly.";
}

QString QuantumChemistryInterface::processCancellationMsg() {
  return programName() + " job terminated.";
}
