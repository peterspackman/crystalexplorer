#include "occwavefunctiontask.h"
#include "exefileutilities.h"
#include "filedependency.h"
#include "json.h"
#include <fmt/core.h>
#include <occ/core/element.h>

QString toJson(const wfn::Parameters &params) {
  nlohmann::json root;
  nlohmann::json topology;

  auto nums = params.structure->atomicNumbersForIndices(params.atoms);
  auto pos = params.structure->atomicPositionsForIndices(params.atoms);

  std::vector<std::string> symbols;
  std::vector<double> positions;

  // should be in angstroms
  for (int i = 0; i < nums.rows(); i++) {
    symbols.push_back(occ::core::Element(nums(i)).symbol());
    positions.push_back(pos(0, i));
    positions.push_back(pos(1, i));
    positions.push_back(pos(2, i));
  }
  root["schema_name"] = "qcschema_input";
  root["scema_version"] = 1;
  root["return_output"] = true;

  root["molecule"]["geometry"] = positions;
  root["molecule"]["symbols"] = symbols;
  root["driver"] = "energy";

  root["model"]["method"] = params.method;
  root["model"]["basis"] = params.basis;

  return QString::fromStdString(root.dump(2));
}

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

void OccWavefunctionTask::start() {
  QString json = toJson(m_parameters);
  emit progressText("Generated JSON input");

  QString name = baseName();
  QString inputName = name + inputSuffix();
  QString outputName = wavefunctionFilename();

  if (!io::writeTextFile(inputName, json)) {
    emit errorOccurred("Could not write input file");
    return;
  }
  emit progressText("Wrote input file");

  setArguments({"scf", inputName});
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
