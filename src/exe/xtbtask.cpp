#include "xtbtask.h"
#include "exefileutilities.h"
#include "filedependency.h"
#include <QJsonDocument>
#include <fmt/core.h>
#include <occ/core/element.h>
#include "xtbcoord.h"

XtbTask::XtbTask(QObject *parent)
    : ExternalProgramTask(parent) {
  setExecutable(exe::findProgramInPath("xtb"));
  qDebug() << "Executable" << executable();
}

void XtbTask::setParameters(const xtb::Parameters &params) {
  m_parameters = params;
}

const xtb::Parameters &XtbTask::getParameters() const {
  return m_parameters;
}

void XtbTask::start() {
  QString coord = xtbCoordString(m_parameters);
  emit progressText("Generated coord input");

  QString name = baseName();
  QString inputName = name + inputSuffix();
  QString outputName = "xtbout.json";

  if (!exe::writeTextFile(inputName, coord)) {
    emit errorOccurred("Could not write input file");
    return;
  }
  emit progressText("Wrote input file");

  setArguments({inputName});
  setRequirements({FileDependency(inputName)});
  setOutputs({FileDependency(outputName)});

  emit progressText("Starting XTB process");
  ExternalProgramTask::start();
  qDebug() << "Finish occ task start";
}

QString XtbTask::inputSuffix() const { return inputSuffixDefault; }
