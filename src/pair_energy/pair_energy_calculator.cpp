#include "pair_energy_calculator.h"
#include "occpairtask.h"
#include <occ/core/element.h>
#include <QFile>
#include <QTextStream>

PairEnergyCalculator::PairEnergyCalculator(QObject * parent) : QObject(parent) {}

void PairEnergyCalculator::setTaskManager(TaskManager *mgr) {
    m_taskManager = mgr;
}

void PairEnergyCalculator::start(pair_energy::Parameters params) {
  if(!(params.wfnA && params.wfnB)) {
    qDebug() << "Found nullptr for wfn in PairEnergyCalculator";
    return;
  }
  m_structure = qobject_cast<ChemicalStructure *>(params.wfnA->parent());

  auto * task = new OccPairTask();
  task->setParameters(params);
  QString name = "pair";
  task->setProperty("name", name);

  auto taskId = m_taskManager->add(task);
  connect(task, &Task::completed, [&, params, name]() {
	this->pairEnergyComplete(params, name);
  });

}

void PairEnergyCalculator::pairEnergyComplete(pair_energy::Parameters params, QString name) {
    qDebug() << "Task" << name << "finished in PairEnergyCalculator";
}
