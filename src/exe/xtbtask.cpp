#include "xtbtask.h"
#include "exefileutilities.h"
#include "filedependency.h"
#include "settings.h"
#include "xtb.h"
#include <fmt/core.h>
#include <occ/core/element.h>

XtbTask::XtbTask(QObject *parent) : ExternalProgramTask(parent) {
  setExecutable(
      settings::readSetting(settings::keys::XTB_EXECUTABLE).toString());
  qDebug() << "Executable" << executable();
}

void XtbTask::setParameters(const xtb::Parameters &params) {
  m_parameters = params;
}

const xtb::Parameters &XtbTask::getParameters() const { return m_parameters; }

QString XtbTask::jsonFilename() const {
  return QString("%1.xtbout.json").arg(hashedBaseName());
}

QString XtbTask::moldenFilename() const {
  return QString("%1.molden.input").arg(hashedBaseName());
}

QString XtbTask::propertiesFilename() const {
  return QString("%1_properties.txt").arg(hashedBaseName());
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

QString XtbTask::coordFilename() const {
  return hashedBaseName() + inputSuffix();
}

void XtbTask::preProcess() {

  QString name = hashedBaseName();

  QString coord = m_parameters.userInputContents;
  if (!m_parameters.userEditRequested) {
    coord = xtbCoordString(m_parameters);
  }
  emit progressText("Generated coord input");

  if (!io::writeTextFile(coordFilename(), coord)) {
    emit errorOccurred("Failed to write input file");
    return;
  }

  emit progressText("Wrote input file");

  QStringList arguments{coordFilename()};
  FileDependencyList outputs{
      FileDependency("xtbout.json", jsonFilename()),
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

  // Store result data in properties (thread-safe)
  setProperty("result_name", baseName());
  setProperty("result_success", true);

  if (deleteWorkingFiles()) {
    emit progressText("Deleting XTB working files");
    io::deleteFile(jsonFilename());
    io::deleteFile(propertiesFilename());
    io::deleteFile(moldenFilename());
  }
  emit progressText("Finished post processing");
  qDebug() << "Finish post process" << baseName();
}

QString XtbTask::inputSuffix() const { return inputSuffixDefault; }

xtb::Result XtbTask::getResult() const {
  xtb::Result result;

  // Build result from properties (thread-safe)
  result.name = property("result_name").toString();
  result.success = property("result_success").toBool();
  result.stdoutContents = stdoutContents().toUtf8();
  result.jsonContents = jsonContents().toUtf8();
  result.propertiesContents = propertiesContents().toUtf8();
  result.moldenContents = moldenContents().toUtf8();

  return result;
}
