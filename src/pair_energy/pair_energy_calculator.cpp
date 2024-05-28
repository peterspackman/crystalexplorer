#include "pair_energy_calculator.h"
#include "load_pair_energy_json.h"
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
  task->setProperty("basename", name);
  task->setJsonFilename(QString("%1_energies.json").arg(name));
  QString jsonFilename = task->jsonFilename();

  auto taskId = m_taskManager->add(task);
  connect(task, &Task::completed, [&, task, params, jsonFilename, name]() {
	this->pairEnergyComplete(params, task);
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
        task->setProperty("basename", name);
	task->setJsonFilename(QString("%1_energies.json").arg(name));


        tasks.append(task);
        connect(task, &Task::completed, this, [this, task, params, tasks]() {
            completedTaskCount++;

            if (completedTaskCount == tasks.size()) {
	        m_complete = true;
            }

            this->pairEnergyComplete(params, task);
        });
        auto taskId = m_taskManager->add(task);
    }
}

void PairEnergyCalculator::pairEnergyComplete(pair_energy::Parameters params, OccPairTask * task) {
    qDebug() << "Task" << task->baseName() << "finished in PairEnergyCalculator";
    if(params.structure) {
	auto * interactions = params.structure->interactions();
	PairInteractionResult * result = load_pair_energy_json(task->jsonFilename());
	result->setParameters(params);
	qDebug() << "Loaded interaction energies from" << task->jsonFilename() << result;
	if(result) {
	    qDebug() << result->components();
	}
	interactions->addPairInteractionResult(result);
    }
    if(m_complete) {
	emit calculationComplete();
    }
}
