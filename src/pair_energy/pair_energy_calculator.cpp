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

void PairEnergyCalculator::start_batch(const std::vector<pair_energy::Parameters> &energies) {
    QList<OccPairTask*> tasks;
    completedTaskCount = 0;

    int idx = 0;
    for (const auto &params : energies) {
        if (!params.structure) {
            qDebug() << "Found nullptr for chemical structure in WavefunctionCalculator";
            continue;
        }
	// assume they're all for the same structure.
	m_structure = params.structure;

        auto *task = new OccPairTask();
        task->setParameters(params);
	QString name = QString("pair_%1").arg(idx++);
        task->setProperty("name", name);

        auto taskId = m_taskManager->add(task);
        tasks.append(task);
        connect(task, &Task::completed, this, [this, params, name, tasks]() {
            completedTaskCount++;

            if (completedTaskCount == tasks.size()) {
	        m_complete = true;
            }

            this->pairEnergyComplete(params, name);
        });
    }
}

void PairEnergyCalculator::pairEnergyComplete(pair_energy::Parameters params, QString name) {
    qDebug() << "Task" << name << "finished in PairEnergyCalculator";
    if(params.structure) {
	auto * interactions = params.structure->interactions();
	PairInteractionResult * result = new PairInteractionResult(params.model);
	result->addComponent("test", 0.0);
	interactions->addPairInteractionResult(result);
    }
    if(m_complete) {
	emit calculationComplete();
    }
}
