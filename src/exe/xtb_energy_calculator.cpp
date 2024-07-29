#include "xtb_energy_calculator.h"
#include "load_xtb_json.h"
#include "exefileutilities.h"
#include "xtbtask.h"
#include "settings.h"
#include <QFile>
#include <QTextStream>
#include <occ/core/element.h>

XtbEnergyCalculator::XtbEnergyCalculator(QObject *parent)
    : QObject(parent) {
  m_xtbExecutable =
      settings::readSetting(settings::keys::XTB_EXECUTABLE).toString();
  m_environment = QProcessEnvironment::systemEnvironment();
  m_environment.insert("OMP_NUM_THREADS", "1");
  m_deleteWorkingFiles = settings::readSetting(settings::keys::DELETE_WORKING_FILES).toBool();
}

void XtbEnergyCalculator::setTaskManager(TaskManager *mgr) {
  m_taskManager = mgr;
}

void XtbEnergyCalculator::start(xtb::Parameters params) {
  if (!params.structure) {
    qDebug()
        << "Found nullptr for chemical structure in XtbEnergyCalculator";
    return;
  }
  m_structure = params.structure;

  std::vector<int> idx =
      params.structure->atomIndicesWithFlags(AtomFlag::Selected);
  occ::IVec nums = params.structure->atomicNumbers()(idx);
  occ::Mat3N pos = params.structure->atomicPositions()(Eigen::all, idx);

  if(params.name == "XtbCalculation") {
    params.name = xtb::methodToString(params.method);
  }
  auto *task = new XtbTask();
  task->setParameters(params);
  task->setProperty("name", params.name);
  task->setProperty("basename", params.name);
  task->setExecutable(m_xtbExecutable);
  task->setEnvironment(m_environment);
  QString outputFilename = task->jsonFilename();

  auto taskId = m_taskManager->add(task);
  connect(task, &Task::completed,
          [&, params, outputFilename]() {
            this->handleFinishedTask(params, outputFilename, params.name);
          });
}

void XtbEnergyCalculator::handleFinishedTask(xtb::Parameters params,
                                             QString filename,
                                             QString name) {
  qDebug() << "Task" << name << "finished in XtbEnergyCalculator";
  auto result = load_xtb_json(filename);
  result.name = name;
  qDebug() << "Loaded result from" << filename << result.success
           << params.atoms.size();
  if(m_deleteWorkingFiles) {
    exe::deleteFile(filename);
  }
  emit calculationComplete(params, result);
}

xtb::Result XtbEnergyCalculator::getResult(int index) const {
  return m_results[index];
}
