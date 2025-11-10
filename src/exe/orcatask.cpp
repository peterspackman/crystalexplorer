#include "orcatask.h"
#include "exefileutilities.h"
#include "filedependency.h"
#include "orcainput.h"
#include "settings.h"
#include <fmt/core.h>
#include <occ/core/element.h>

OrcaSCFTask::OrcaSCFTask(QObject *parent) : ExternalProgramTask(parent) {
  setExecutable(
      settings::readSetting(settings::keys::ORCA_EXECUTABLE).toString());
}

void OrcaSCFTask::setParameters(const wfn::Parameters &params) {
  m_parameters = params;
}

const wfn::Parameters &OrcaSCFTask::getParameters() const {
  return m_parameters;
}

QString OrcaSCFTask::gbwFilename() const {
  return QString("%1.gbw").arg(baseName());
}

QString OrcaSCFTask::propertiesFilename() const {
  return QString("%1.property.txt").arg(baseName());
}

QString OrcaSCFTask::jsonFilename() const {
  return QString("%1.orca.json").arg(baseName());
}

QString OrcaSCFTask::moldenFilename() const {
  return QString("%1.molden").arg(baseName());
}

void OrcaSCFTask::start() {
  QString input = m_parameters.userInputContents;

  if (!m_parameters.userEditRequested) {
    input = io::orcaInputString(m_parameters);
  }

  emit progressText("Generated ORCA input");

  QString name = baseName();
  QString inputName = name + inputSuffix();
  QString outputName = jsonFilename();

  if (!io::writeTextFile(inputName, input)) {
    emit errorOccurred("Could not write input file");
    return;
  }

  emit progressText("Wrote input file");

  QStringList arguments{inputName};
  FileDependencyList outputs{FileDependency(gbwFilename())};
  setArguments(arguments);
  setRequirements({FileDependency(inputName)});
  setOutputs(outputs);

  emit progressText("Starting ORCA process");
  ExternalProgramTask::start();
  qDebug() << "Finish ORCA task start";
}

QString OrcaSCFTask::inputSuffix() const { return inputSuffixDefault; }

OrcaConvertTask::OrcaConvertTask(QObject *parent)
    : ExternalProgramTask(parent) {
  setExecutable(
      settings::readSetting(settings::keys::ORCA_2MKL_EXECUTABLE).toString());
}

QString OrcaConvertTask::moldenFilename() const {
  return io::changeSuffix(gbwFilename(), ".molden");
}

void OrcaConvertTask::start() {
  QStringList arguments{"input", "-molden"};
  FileDependencyList requirements{FileDependency(gbwFilename(), "input.gbw")};
  FileDependencyList outputs{
      FileDependency("input.molden.input", moldenFilename())};
  setArguments(arguments);
  setRequirements(requirements);
  setOutputs(outputs);

  emit progressText("Starting orca_2mkl process");
  ExternalProgramTask::start();
}

void OrcaConvertTask::setGbwFilename(const QString &f) { m_gbw = f; }

const QString &OrcaConvertTask::gbwFilename() const { return m_gbw; }

void OrcaConvertTask::setFormat(bool json) { m_json = json; }

OrcaWavefunctionTask::OrcaWavefunctionTask(QObject *parent)
    : ExternalProgramTask(parent), m_scfTask(new OrcaSCFTask(this)),
      m_convertTask(new OrcaConvertTask(this)) {

  connect(m_scfTask, &OrcaSCFTask::completed, this,
          &OrcaWavefunctionTask::scfFinished);
  connect(m_scfTask, &OrcaSCFTask::errorOccurred, this,
          &OrcaWavefunctionTask::errorOccurred);
  connect(m_scfTask, &OrcaSCFTask::progressText, this,
          &OrcaWavefunctionTask::progressText);
  connect(m_scfTask, &OrcaSCFTask::stdoutChanged, this,
          &OrcaWavefunctionTask::updateStdout);

  connect(m_convertTask, &OrcaConvertTask::completed, this,
          &OrcaWavefunctionTask::conversionFinished);
  connect(m_convertTask, &OrcaConvertTask::errorOccurred, this,
          &OrcaWavefunctionTask::errorOccurred);
  connect(m_convertTask, &OrcaConvertTask::progressText, this,
          &OrcaWavefunctionTask::progressText);
  connect(m_convertTask, &OrcaSCFTask::stdoutChanged, this,
          &OrcaWavefunctionTask::updateStdout);
}

void OrcaWavefunctionTask::updateStdout() {
  setProperty("stdout", m_scfTask->property("stdout"));
  setProperty("stderr", m_scfTask->property("stderr"));

  setProperty("convert-stdout", m_convertTask->property("stdout"));
  setProperty("convert-stderr", m_convertTask->property("stderr"));
}

void OrcaWavefunctionTask::start() {
  m_scfTask->setExecutable(executable());
  m_scfTask->setDeleteWorkingFiles(deleteWorkingFiles());
  m_scfTask->start();
}

void OrcaWavefunctionTask::scfFinished() {
  m_convertTask->setExecutable(executable() + "_2mkl");
  m_convertTask->setGbwFilename(m_scfTask->gbwFilename());
  m_convertTask->start();
}

void OrcaWavefunctionTask::conversionFinished() { emit completed(); }

void OrcaWavefunctionTask::setBackend(TaskBackend *backend) {
  // Set backend for this task
  Task::setBackend(backend);

  // Propagate backend to sub-tasks
  if (m_scfTask) {
    m_scfTask->setBackend(backend);
  }
  if (m_convertTask) {
    m_convertTask->setBackend(backend);
  }
}
