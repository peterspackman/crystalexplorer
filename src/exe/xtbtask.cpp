#include "xtbtask.h"
#include "exefileutilities.h"
#include "filedependency.h"
#include <QJsonDocument>
#include <fmt/core.h>
#include <occ/core/element.h>
#include "xtb.h"

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

QString XtbTask::jsonFilename() const {
  return QString("%1.xtbout.json").arg(baseName());
}

QString XtbTask::moldenFilename() const {
  return QString("%1.molden.input").arg(baseName());
}

void XtbTask::start() {
  QString coord = xtbCoordString(m_parameters);
  emit progressText("Generated coord input");

  QString name = baseName();
  QString inputName = name + inputSuffix();
  QString outputName = jsonFilename();

  if (!exe::writeTextFile(inputName, coord)) {
    emit errorOccurred("Could not write input file");
    return;
  }
  emit progressText("Wrote input file");


  QStringList arguments{inputName};
  FileDependencyList outputs{FileDependency("xtbout.json", outputName)};
  if(m_parameters.write_molden) {
    arguments.append("--molden");
    outputs.append(FileDependency("molden.input", moldenFilename()));
  }
  setArguments(arguments);
  setRequirements({FileDependency(inputName)});
  setOutputs(outputs);

  emit progressText("Starting XTB process");
  ExternalProgramTask::start();
  qDebug() << "Finish occ task start";
}

QString XtbTask::inputSuffix() const { return inputSuffixDefault; }
