#include "xtbtask.h"
#include "exefileutilities.h"
#include "filedependency.h"
#include "xtb.h"
#include <fmt/core.h>
#include <occ/core/element.h>

XtbTask::XtbTask(QObject *parent) : ExternalProgramTask(parent) {
  setExecutable(exe::findProgramInPath("xtb"));
  qDebug() << "Executable" << executable();
}

void XtbTask::setParameters(const xtb::Parameters &params) {
  m_parameters = params;
}

const xtb::Parameters &XtbTask::getParameters() const { return m_parameters; }

QString XtbTask::jsonFilename() const {
  return QString("%1.xtbout.json").arg(baseName());
}

QString XtbTask::moldenFilename() const {
  return QString("%1.molden.input").arg(baseName());
}

QString XtbTask::propertiesFilename() const {
  return QString("%1_properties.txt").arg(baseName());
}

QString XtbTask::stdoutContents() const {
  return property("stdout").toString();
}

QString XtbTask::jsonContents() const {
  return property(getOutputFilePropertyName("xtbout.json")).toString();
}

QString XtbTask::moldenContents() const {
  return property(getOutputFilePropertyName("molden.input")).toString();
}

QString XtbTask::propertiesContents() const {
  return property(getOutputFilePropertyName("properties.txt")).toString();
}

QString XtbTask::coordFilename() const { return baseName() + inputSuffix(); }

void XtbTask::preProcess() {
  QString coord = xtbCoordString(m_parameters);
  emit progressText("Generated coord input");

  QString name = baseName();

  if (!io::writeTextFile(coordFilename(), coord)) {
    emit errorOccurred("Could not write input file");
    return;
  }
  emit progressText("Wrote input file");

  QStringList arguments{coordFilename()};
  FileDependencyList outputs{FileDependency("xtbout.json", jsonFilename()),
                             FileDependency("properties.txt", propertiesFilename())};

  if (m_parameters.write_molden) {
    arguments.append("--molden");
    outputs.append(FileDependency("molden.input", moldenFilename()));
  }
  setArguments(arguments);
  setRequirements({FileDependency(coordFilename())});
  setOutputs(outputs);
  emit progressText("Finished preprocessing XTB task");
}

void XtbTask::start() { ExternalProgramTask::start(); }

void XtbTask::postProcess() {
  qDebug() << "Begin post process" << baseName();
  emit progressText("Reading xtb outputs");

  XtbOutputs outputs;
  outputs.stdoutContents = stdoutContents();
  outputs.jsonPath = jsonFilename();
  outputs.propertiesPath = propertiesFilename();
  outputs.moldenPath = moldenFilename();

  m_result.name = baseName();
  m_result.stdoutContents = stdoutContents().toUtf8();
  m_result.jsonContents = jsonContents().toUtf8();
  m_result.propertiesContents = propertiesContents().toUtf8();
  m_result.moldenContents = moldenContents().toUtf8();

  if (deleteWorkingFiles()) {
    emit progressText("Deleting XTB working files");
    io::deleteFile(jsonFilename());
    io::deleteFile(propertiesFilename());
    io::deleteFile(moldenFilename());
  }
  emit progressText("Finished post processing");
  qDebug() << "Finish post process" << baseName();
  m_result.success = true;
}

QString XtbTask::inputSuffix() const { return inputSuffixDefault; }

const xtb::Result &XtbTask::getResult() const { return m_result; }
