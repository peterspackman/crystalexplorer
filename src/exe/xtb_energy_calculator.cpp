#include "xtb_energy_calculator.h"
#include "exefileutilities.h"
#include "io_utilities.h"
#include "settings.h"
#include "xtb.h"
#include "xtbtask.h"
#include <QFile>
#include <QTextStream>
#include <occ/core/element.h>

XtbEnergyCalculator::XtbEnergyCalculator(QObject *parent) : QObject(parent) {
  m_xtbExecutable =
      settings::readSetting(settings::keys::XTB_EXECUTABLE).toString();
  m_environment = QProcessEnvironment::systemEnvironment();
  m_environment.insert("OMP_NUM_THREADS", "1");
  m_deleteWorkingFiles =
      settings::readSetting(settings::keys::DELETE_WORKING_FILES).toBool();
}

XtbTask* XtbEnergyCalculator::createTask(xtb::Parameters params) {
  if (!params.structure) {
    qDebug() << "Found nullptr for chemical structure in XtbEnergyCalculator";
    return nullptr;
  }
  auto idx = params.structure->atomsWithFlags(AtomFlag::Selected);
  occ::IVec nums = params.structure->atomicNumbersForIndices(idx);
  occ::Mat3N pos = params.structure->atomicPositionsForIndices(idx);

  if (params.name == "XtbCalculation") {
    params.name = xtb::methodToString(params.method);
  }
  if(params.userEditRequested) {
    params.userInputContents = io::requestUserTextEdit("XTB input", xtbCoordString(params));
    if(params.userInputContents.isEmpty()) {
      qInfo() << "XTB calculation canceled by user";
      return nullptr;
    }
  }
  auto *task = new XtbTask();
  task->setParameters(params);
  task->setProperty("name", params.name);
  task->setProperty("basename", params.name);
  task->setExecutable(m_xtbExecutable);
  task->setEnvironment(m_environment);
  task->setDeleteWorkingFiles(m_deleteWorkingFiles);

  // Store params in task properties for caller's convenience
  task->setProperty("xtb_params", QVariant::fromValue(params));

  // Return configured task - caller will connect then add to TaskManager
  return task;
}
