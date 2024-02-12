#include "xtbinterface.h"
#include <QMessageBox>
#include "settings.h"

XTBInterface::XTBInterface(QWidget *parent)
    : QObject(parent), m_parentWidget(parent), m_process(new QProcess()) {
  m_inputEditor = new FileEditor();
  connect(m_inputEditor, &FileEditor::writtenFileToDisk, this,
          &XTBInterface::runProcess, Qt::UniqueConnection);
  connect(m_process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &XTBInterface::jobFinished, Qt::UniqueConnection);
  connect(m_process, &QProcess::stateChanged, this, &XTBInterface::jobState,
          Qt::UniqueConnection);
}

void XTBInterface::prejobSetup() {
  // currently do nothing
}

void XTBInterface::runJob(const JobParameters &jobParams,
                          DeprecatedCrystal *crystal) {
  m_currentJobParams = jobParams;
  prejobSetup();
  setCurrentJobNameFromCrystal(crystal);
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

void XTBInterface::editInputFile() {
  m_inputEditor->insertFile(fullInputFilename());
  m_inputEditor->show();
}

void XTBInterface::runProcess() {
  m_cycle = 0;
  m_processStoppedByUser = false;

  m_process->setWorkingDirectory(m_workingDirectory);
  m_process->setProcessEnvironment(getEnvironment());
  if (redirectStdoutToOutputFile()) {
    m_process->setStandardOutputFile(outputFilename());
  }
  m_process->start(program(), commandline(m_currentJobParams));
}

void XTBInterface::jobState(QProcess::ProcessState state) {
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

QProcessEnvironment XTBInterface::getEnvironment() {
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("OMP_NUM_THREADS", "1,1");
  env.insert("OMP_MAX_ACTIVE_LEVELS", "1");
  env.insert("OMP_STACKSIZE", "4G");
  return env;
}

QString XTBInterface::jobDescription(JobType jobType, int maxStep, int step) {
  QString description(jobProcessDescription(jobType));

  if (maxStep > 0) {
    description += QString(" (%1/%2)").arg(step).arg(maxStep);
  }
  return description;
}

void XTBInterface::jobFinished(int exitCode, QProcess::ExitStatus exitStatus) {
  Q_UNUSED(exitCode);
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
  if (!foundError) {
    QString jsonFile = workingDirectory() + QDir::separator() + "xtbout.json";
    QFile json(jsonFile);
    if (json.open(QFile::ReadOnly)) {
      m_lastJsonOutput = QJsonDocument::fromJson(json.readAll());
    }

    else {
      foundError = true;
    }
  }
  emit processFinished(foundError, m_currentJobParams.jobType);
}

bool XTBInterface::errorInOutput() {
  QString stderrString = m_process->readAllStandardError();
  if (stderrString.contains(normalTerminationHook(), Qt::CaseInsensitive)) {
    return false;
  }
  return true;
}

void XTBInterface::stopJob() {
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

void XTBInterface::setWorkingDirectory(const QString &filename) {
  QFileInfo fileInfo(filename);
  m_workingDirectory = fileInfo.absolutePath();
}

const QString &XTBInterface::workingDirectory() const {
  return m_workingDirectory;
}

void XTBInterface::setCurrentJobNameFromCrystal(DeprecatedCrystal *crystal) {
  QFileInfo f(crystal->cifFilename());
  QString cifFilename = f.baseName();
  QString crystalName = crystal->crystalName();
  m_currentJobName = cifFilename + "_" + crystalName;
}

QString XTBInterface::fullInputFilename() {
  Q_ASSERT(!workingDirectory().isEmpty());
  QString inputFile = workingDirectory() + QDir::separator() + inputFilename();
  return inputFile;
}

bool XTBInterface::writeInputfile(DeprecatedCrystal *crystal) {
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
    case JobType::monomerEnergy:
      writeInputForMonomerEnergyCalculation(ts, m_currentJobParams, crystal);
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

QString XTBInterface::outputFilePath() {
  Q_ASSERT(!workingDirectory().isEmpty());
  return workingDirectory() + QDir::separator() + outputFilename();
}

QString XTBInterface::inputFilePath() {
  Q_ASSERT(!workingDirectory().isEmpty());

  return workingDirectory() + QDir::separator() +
         m_currentJobParams.QMInputFilename;
}

QString XTBInterface::outputFilename() const {
  QString extension = "xtb_stdout";
  return m_currentJobName + "." + extension;
}

QString XTBInterface::errorTitle() { return "Error running " + programName(); }

QString XTBInterface::failedWritingInputfileMsg() {
  return "Unable to write " + programName() + " input file.";
}

QString XTBInterface::execMissingMsg() {
  return "Unable to find " + programName() + " executable. Check the " +
         programName() + " path is set correctly in the preferences.";
}

QString XTBInterface::execRunningMsg() {
  return programName() + " wavefunction calculation in progress...";
}

QString XTBInterface::execFailedMsg() {
  return programName() + " failed to run.";
}

QString XTBInterface::execCrashMsg() {
  return programName() + " crashed unexpectedly.";
}

QString XTBInterface::processCancellationMsg() {
  return programName() + " job terminated.";
}

bool XTBInterface::executableInstalled() { return QFile::exists(executable()); }

QString XTBInterface::executable() {
  return settings::readSetting(settings::keys::XTB_EXECUTABLE).toString();
}

QString XTBInterface::program() { return executable(); }

QString XTBInterface::programName() { return executable(); }

QStringList XTBInterface::commandline(const JobParameters &jobParams) {
  return {jobParams.QMInputFilename};
}

QString XTBInterface::inputFilename() {
  m_currentInputFilename = m_currentJobName + ".inp";
  return m_currentInputFilename;
}

QString XTBInterface::calculationName(QString cifFilename,
                                      QString crystalName) {
  Q_ASSERT(!crystalName.contains("/"));

  QFileInfo fileInfo(cifFilename);
  QString name = fileInfo.baseName().replace(" ", "_");
  return name + "_" + crystalName;
}

QString XTBInterface::wavefunctionFilename(const JobParameters &jobParams,
                                           QString crystalName) {
  QString calcName = calculationName(jobParams.inputFilename, crystalName);
  QString suffix = defaultFchkFileExtension();
  return calcName + suffix;
}

QString XTBInterface::basissetName(BasisSet basis) {
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

QString XTBInterface::methodName(const JobParameters &jobParams) {
  switch (jobParams.theory) {
  case Method::b3lyp:
    return "b3lyp";
  case Method::hartreeFock:
    return (jobParams.multiplicity == 1) ? "rhf" : "uhf";
  default:
    return "unknown";
  }
}

void XTBInterface::writeInputForPairEnergyCalculation(
    QTextStream &ts, const JobParameters &jobParams,
    DeprecatedCrystal *crystal) {
  ts << "$coord angs" << Qt::endl;
  const auto atoms = crystal->generateAtomsFromAtomIds(jobParams.atoms);
  for (const auto &atom : atoms) {
    const auto &pos = atom.pos();
    ts << pos[0] << " " << pos[1] << " " << pos[2] << " " << atom.symbol()
       << Qt::endl;
  }
  ts << "$gfn" << Qt::endl;
  int method = 0;
  if (jobParams.theory == Method::GFN1xTB)
    method = 1;
  if (jobParams.theory == Method::GFN2xTB)
    method = 2;
  ts << "method=" << method << Qt::endl;
  ts << "$chrg"
     << " " << jobParams.charge << Qt::endl;
  ts << "$spin"
     << " " << jobParams.multiplicity - 1 << Qt::endl;
  ts << "$write" << Qt::endl
     << "json=true" << Qt::endl; // write out a json file
  ts << "$end" << Qt::endl;
}

void XTBInterface::writeInputForMonomerEnergyCalculation(
    QTextStream &ts, const JobParameters &jobParams,
    DeprecatedCrystal *crystal) {
  ts << "$coord angs" << Qt::endl;
  const auto atoms = crystal->generateAtomsFromAtomIds(jobParams.atoms);
  for (const auto &atom : atoms) {
    const auto &pos = atom.pos();
    ts << pos[0] << " " << pos[1] << " " << pos[2] << " " << atom.symbol()
       << Qt::endl;
  }
  ts << "$gfn" << Qt::endl;
  int method = 0;
  if (jobParams.theory == Method::GFN1xTB)
    method = 1;
  if (jobParams.theory == Method::GFN2xTB)
    method = 2;
  ts << "method=" << method << Qt::endl;
  ts << "$chrg"
     << " " << jobParams.charge << Qt::endl;
  ts << "$spin"
     << " " << jobParams.multiplicity - 1 << Qt::endl;
  ts << "$write" << Qt::endl
     << "json=true" << Qt::endl; // write out a json file
  ts << "$end" << Qt::endl;
}
