#include "xtb_energy_calculator.h"
#include "xtb.h"
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
  auto idx = params.structure->atomsWithFlags(AtomFlag::Selected);
  occ::IVec nums = params.structure->atomicNumbersForIndices(idx);
  occ::Mat3N pos = params.structure->atomicPositionsForIndices(idx);

  if(params.name == "XtbCalculation") {
    params.name = xtb::methodToString(params.method);
  }
  auto *task = new XtbTask();
  task->setParameters(params);
  task->setProperty("name", params.name);
  task->setProperty("basename", params.name);
  task->setExecutable(m_xtbExecutable);
  task->setEnvironment(m_environment);
  QString jsonFilename = task->jsonFilename();
  QString moldenFilename = task->moldenFilename();

  auto taskId = m_taskManager->add(task);
  connect(task, &Task::completed,
          [&, params, jsonFilename, moldenFilename]() {
            this->handleFinishedTask(params, params.name, jsonFilename, moldenFilename);
          });
}

void XtbEnergyCalculator::handleFinishedTask(xtb::Parameters params,
                                             QString name,
                                             QString jsonFilename,
                                             QString moldenFilename) {
  qDebug() << "Task" << name << "finished in XtbEnergyCalculator";
  auto result = loadXtbResult(params, jsonFilename, moldenFilename);
  result.name = name;

  qDebug() << "Loaded result from" << jsonFilename << moldenFilename << result.success
           << params.atoms.size();
  if(m_deleteWorkingFiles) {
    exe::deleteFile(jsonFilename);
    if(params.write_molden) {
      exe::deleteFile(moldenFilename);
    }
  }
  emit calculationComplete(params, result);
}
