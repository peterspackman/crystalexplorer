#include "pair_energy_calculator.h"
#include "load_pair_energy_json.h"
#include "load_wavefunction.h"
#include "io_utilities.h"
#include "occpairtask.h"
#include "settings.h"
#include <QFile>
#include <QTextStream>
#include <occ/core/element.h>

inline xtb::Parameters pair2xtb(const pair_energy::Parameters &params) {
  xtb::Parameters result;
  result.charge = params.charge();
  result.multiplicity = params.multiplicity();
  result.method = xtb::stringToMethod(params.model);
  result.structure = params.structure;
  qDebug() << params.wfnA << params.wfnB << params.model
           << xtb::methodToString(result.method);
  if (params.wfnA && params.wfnB) {
    result.reference_energy =
        params.wfnA->totalEnergy() + params.wfnB->totalEnergy();
  }
  result.atoms = params.atomsA;
  result.atoms.insert(result.atoms.end(), params.atomsB.begin(),
                      params.atomsB.end());
  result.name = params.deriveName();
  return result;
}

PairEnergyCalculator::PairEnergyCalculator(QObject *parent) : QObject(parent) {
  m_occExecutable =
      settings::readSetting(settings::keys::OCC_EXECUTABLE).toString();
  m_environment = QProcessEnvironment::systemEnvironment();
  QString dataDir =
      settings::readSetting(settings::keys::OCC_DATA_DIRECTORY).toString();
  m_environment.insert("OCC_DATA_PATH", dataDir);
  m_environment.insert("OCC_BASIS_PATH", dataDir);

  m_xtb = new XtbEnergyCalculator(this);
  connect(m_xtb, &XtbEnergyCalculator::calculationComplete, this,
          &PairEnergyCalculator::handleXtbTaskComplete);
}

void PairEnergyCalculator::setTaskManager(TaskManager *mgr) {
  m_taskManager = mgr;
  m_xtb->setTaskManager(mgr);
}

void PairEnergyCalculator::start(pair_energy::Parameters params) {
  if (!(params.wfnA && params.wfnB)) {
    qDebug() << "Found nullptr for wfn in PairEnergyCalculator";
    return;
  }
  m_structure = qobject_cast<ChemicalStructure *>(params.wfnA->parent());

  if (params.isXtbModel()) {
    xtb::Parameters xtb_params = pair2xtb(params);
    m_parameters[xtb_params.name] = params;
    m_xtb->start(xtb_params);
    return;
  } else {
    auto *task = new OccPairTask();
    task->setParameters(params);
    QString name = params.deriveName();
    task->setProperty("basename", name);
    task->setProperty("name", name);
    task->setExecutable(m_occExecutable);
    task->setEnvironment(m_environment);
    QString jsonFilename = task->jsonFilename();

    auto taskId = m_taskManager->add(task);
    connect(task, &Task::completed, [&, task, params, jsonFilename, name]() {
      this->pairEnergyComplete(params, task);
    });
  }
}

void PairEnergyCalculator::start_batch(
    const std::vector<pair_energy::Parameters> &energies) {
  QList<OccPairTask *> tasks;
  m_completedTaskCount = 0;

  int idx = 0;
  m_totalTasks = energies.size();
  for (const auto &params : energies) {
    if (!params.structure) {
      qDebug()
          << "Found nullptr for chemical structure in WavefunctionCalculator";
      continue;
    }
    // assume they're all for the same structure.
    m_structure = params.structure;

    if (params.isXtbModel()) {
      xtb::Parameters xtb_params = pair2xtb(params);
      m_parameters[xtb_params.name] = params;
      m_xtb->start(xtb_params);
      continue;
      ;
    }

    auto *task = new OccPairTask();
    QString name = params.deriveName();
    task->setParameters(params);
    task->setExecutable(m_occExecutable);
    task->setEnvironment(m_environment);
    task->setProperty("name", name);
    task->setProperty("basename", name);

    tasks.append(task);
    connect(task, &Task::completed, this,
            [this, task, params]() { this->pairEnergyComplete(params, task); });
  }

  for (auto *task : tasks) {
    auto taskId = m_taskManager->add(task);
  }
}

void PairEnergyCalculator::pairEnergyComplete(pair_energy::Parameters params,
                                              OccPairTask *task) {
  qDebug() << "Task" << task->baseName() << "finished in PairEnergyCalculator";
  if (params.structure) {
    auto *interactions = params.structure->pairInteractions();
    auto *result = load_pair_energy_json(task->jsonFilename());
    result->setParameters(params);
    qDebug() << "Loaded interaction energies from" << task->jsonFilename()
             << result;
    interactions->add(result);
    m_completedTaskCount++;
    if (m_completedTaskCount == m_totalTasks) {
      m_complete = true;
    }
  }
  if (m_complete) {
    qDebug() << "Calculation complete";
    emit calculationComplete();
  }
}

void PairEnergyCalculator::handleXtbTaskComplete(xtb::Parameters params,
                                                 xtb::Result result) {
  qDebug() << "Xtb task complete" << result.name;
  if (params.structure) {
    auto *interactions = params.structure->pairInteractions();
    auto *wfn = new MolecularWavefunction();
    bool success =
        io::populateWavefunctionFromJsonContents(wfn, result.jsonContents);
    success = success & io::populateWavefunctionFromXtbStdoutContents(
                            wfn, result.stdoutContents);
    auto pair = new PairInteraction(xtb::methodToString(params.method));
    qDebug() << success << "wfn->totalEnergy" << wfn->totalEnergy() << "ref"
             << params.reference_energy;
    if (!success) {
      qWarning() << "Invalid result from xtb task!";
    }
    double e = wfn->totalEnergy() - params.reference_energy;
    pair->addComponent("Total", e * 2625.5);
    pair->setParameters(m_parameters[result.name]);
    interactions->add(pair);

    delete wfn;
    m_completedTaskCount++;
    if (m_completedTaskCount == m_totalTasks) {
      m_complete = true;
    }
  }
  if (m_complete) {
    qDebug() << "Calculation complete";
    emit calculationComplete();
  }
}
