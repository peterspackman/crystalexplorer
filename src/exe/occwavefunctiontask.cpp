#include "occwavefunctiontask.h"
#include "exefileutilities.h"
#include "filedependency.h"
#include "occinput.h"
#include <fmt/core.h>
#include <occ/core/element.h>

OccWavefunctionTask::OccWavefunctionTask(QObject *parent)
    : ExternalProgramTask(parent) {
  setExecutable(exe::findProgramInPath("occ"));
  qDebug() << "Executable" << executable();
}

void OccWavefunctionTask::setParameters(const wfn::Parameters &params) {
  m_parameters = params;
}

const wfn::Parameters &OccWavefunctionTask::getParameters() const {
  return m_parameters;
}

int OccWavefunctionTask::threads() const {
  return properties().value("threads", 6).toInt();
}
void OccWavefunctionTask::start() {
  QString json = m_parameters.userInputContents;
  if (!m_parameters.userEditRequested) {
    json = io::getOccWavefunctionJson(m_parameters);
  }

  emit progressText("Generated JSON input");

  QString name = baseName();
  QString inputName = name + inputSuffix();
  QString outputName = wavefunctionFilename();
  QStringList args{"scf", inputName};
  if (!io::writeTextFile(inputName, json)) {
    emit errorOccurred("Could not write input file: " + inputName);
    return;
  }
  emit progressText("Wrote input file");

  args << QString("--threads=%1").arg(threads());
  setArguments(args);
  setRequirements({FileDependency(inputName)});
  setOutputs({FileDependency(outputName, outputName)});

  emit progressText("Starting OCC process");
  ExternalProgramTask::start();
  qDebug() << "Finish occ task start";
}

QString OccWavefunctionTask::inputSuffix() const { return inputSuffixDefault; }

QString OccWavefunctionTask::wavefunctionSuffix() const {
  return wavefunctionSuffixDefault;
}

QString OccWavefunctionTask::wavefunctionFilename() const {
  return baseName() + wavefunctionSuffix();
}
