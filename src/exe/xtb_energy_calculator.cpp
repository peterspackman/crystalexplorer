#include "xtb_energy_calculator.h"
#include "load_xtb_json.h"
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

  QString methodName =
      QString("XTB/%2").arg(xtb::methodToString(params.method));
  auto *task = new XtbTask();
  task->setParameters(params);
  task->setProperty("name", methodName);
  task->setExecutable(m_xtbExecutable);
  task->setEnvironment(m_environment);
  QString outputFilename = "xtbout.json";

  auto taskId = m_taskManager->add(task);
  connect(task, &Task::completed,
          [&, params, methodName, outputFilename]() {
            m_complete = true;
            this->calculationComplete(params, outputFilename,
                                       methodName);
          });
}

void XtbEnergyCalculator::calculationComplete(xtb::Parameters params,
                                              QString filename,
                                              QString name) {
  qDebug() << "Task" << name << "finished in XtbEnergyCalculator";
  auto result = load_xtb_json(filename);
  qDebug() << "Loaded result from" << filename << result.success
           << params.atoms.size();
  m_results.push_back(result);
  if (result.success) {
      emit calculationComplete();
  }
}

xtb::Result XtbEnergyCalculator::getResult(int index) const {
  return m_results[index];
}
