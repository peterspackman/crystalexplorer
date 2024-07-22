#include "pair_energy_calculator.h"
#include "load_pair_energy_json.h"
#include "occpairtask.h"
#include <occ/core/element.h>
#include <QFile>
#include <QTextStream>
#include "settings.h"

PairEnergyCalculator::PairEnergyCalculator(QObject * parent) : QObject(parent) {
    m_occExecutable =
            settings::readSetting(settings::keys::OCC_EXECUTABLE).toString();
    m_environment = QProcessEnvironment::systemEnvironment();
    QString dataDir =
            settings::readSetting(settings::keys::OCC_DATA_DIRECTORY).toString();
    m_environment.insert("OCC_DATA_PATH", dataDir);
    m_environment.insert("OCC_BASIS_PATH", dataDir);
}

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
    task->setExecutable(m_occExecutable);
    task->setEnvironment(m_environment);
    QString jsonFilename = task->jsonFilename();

    auto taskId = m_taskManager->add(task);
    connect(task, &Task::completed, [&, task, params, jsonFilename, name]() {
        this->pairEnergyComplete(params, task);
    });

}

void PairEnergyCalculator::start_batch(const std::vector<pair_energy::Parameters> &energies) {
    QList<OccPairTask*> tasks;
    m_completedTaskCount = 0;

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
        task->setExecutable(m_occExecutable);
        task->setEnvironment(m_environment);
        QString name = QString("pair_%1").arg(idx++);
        task->setProperty("name", name);
        task->setProperty("basename", name);
        task->setJsonFilename(QString("%1_energies.json").arg(name));


        tasks.append(task);
        connect(task, &Task::completed, this, [this, task, params]() {
            this->pairEnergyComplete(params, task);
        });
    }
    m_totalTasks = tasks.size();

    for(auto * task: tasks) {
        auto taskId = m_taskManager->add(task);
    }
}

void PairEnergyCalculator::pairEnergyComplete(pair_energy::Parameters params, OccPairTask * task) {
    qDebug() << "Task" << task->baseName() << "finished in PairEnergyCalculator";
    if(params.structure) {
        auto * interactions = params.structure->pairInteractions();
        auto * result = load_pair_energy_json(task->jsonFilename());
        result->setParameters(params);
        qDebug() << "Loaded interaction energies from" << task->jsonFilename() << result;
        if(result) {
            qDebug() << result->components();
        }
        interactions->add(result);
        m_completedTaskCount++;
        if (m_completedTaskCount == m_totalTasks) {
            m_complete = true;
        }

    }
    if(m_complete) {
        qDebug() << "Calculation complete";
        emit calculationComplete();
    }
}
