#include "pair_energy_calculator.h"
#include "interaction_energy_calculator.h"
#include "io_utilities.h"
#include "load_pair_energy_json.h"
#include "load_wavefunction.h"
#include "molecular_wavefunction_provider.h"
#include "occpairtask.h"
#include "xtbtask.h"
#include "settings.h"
#include "simple_energy_provider.h"
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
  // No longer connect to shared signal - we'll connect to individual tasks
}

void PairEnergyCalculator::setTaskManager(TaskManager *mgr) {
  m_taskManager = mgr;
}

void PairEnergyCalculator::start(pair_energy::Parameters params) {
  if (!(params.wfnA && params.wfnB)) {
    qDebug() << "Found nullptr for wfn in PairEnergyCalculator";
    return;
  }
  m_structure = qobject_cast<ChemicalStructure *>(params.wfnA->parent());

  if (params.isXtbModel()) {
    xtb::Parameters xtb_params = pair2xtb(params);

    // Create task, connect to it, then submit to TaskManager
    XtbTask *task = m_xtb->createTask(xtb_params);
    if (task) {
      // Store original pair_energy params in task for retrieval in slot
      task->setProperty("pair_params", QVariant::fromValue(params));
      connect(task, &Task::completed, this, [this]() {
        onXtbTaskComplete();
      });
      m_taskManager->add(task);  // TaskManager starts it
    }
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

    // Store context in task properties for safe retrieval in slot
    task->setProperty("pair_params", QVariant::fromValue(params));

    auto taskId = m_taskManager->add(task);
    connect(task, &Task::completed, this, [this]() {
      onPairEnergyTaskComplete();
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

      // Create task, connect to it, then submit to TaskManager
      XtbTask *task = m_xtb->createTask(xtb_params);
      if (task) {
        // Store original pair_energy params in task for retrieval in slot
        task->setProperty("pair_params", QVariant::fromValue(params));
        connect(task, &Task::completed, this, [this]() {
        onXtbTaskComplete();
      });
        m_taskManager->add(task);  // TaskManager starts it
      }
      continue;
    }

    auto *task = new OccPairTask();
    QString name = params.deriveName();
    task->setParameters(params);
    task->setExecutable(m_occExecutable);
    task->setEnvironment(m_environment);
    task->setProperty("name", name);
    task->setProperty("basename", name);

    // Store context in task properties for safe retrieval in slot
    task->setProperty("pair_params", QVariant::fromValue(params));

    tasks.append(task);
    connect(task, &Task::completed, this, [this]() {
      onPairEnergyTaskComplete();
    });
  }

  for (auto *task : tasks) {
    auto taskId = m_taskManager->add(task);
  }
}

void PairEnergyCalculator::onPairEnergyTaskComplete() {
  Task *taskBase = qobject_cast<Task*>(sender());
  if (!taskBase) {
    qWarning() << "onPairEnergyTaskComplete called with invalid sender";
    return;
  }

  OccPairTask *task = dynamic_cast<OccPairTask*>(taskBase);
  if (!task) {
    qWarning() << "onPairEnergyTaskComplete: sender is not an OccPairTask";
    return;
  }

  // Retrieve context from task properties
  pair_energy::Parameters params = taskBase->property("pair_params").value<pair_energy::Parameters>();

  QString jsonFile = task->jsonFilename();
  qDebug() << "[SLOT START]" << task->baseName() << "loading" << jsonFile;
  if (params.structure) {
    auto *interactions = params.structure->pairInteractions();
    auto *result = load_pair_energy_json(jsonFile);
    result->setParameters(params);
    qDebug() << "[SLOT DONE]" << task->baseName() << "model:" << result->objectName();
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

void PairEnergyCalculator::onXtbTaskComplete() {
  Task *taskBase = qobject_cast<Task*>(sender());
  if (!taskBase) {
    qWarning() << "onXtbTaskComplete called with invalid sender";
    return;
  }

  XtbTask *task = dynamic_cast<XtbTask*>(taskBase);
  if (!task) {
    qWarning() << "onXtbTaskComplete: sender is not an XtbTask";
    return;
  }

  // Retrieve original pair_energy params from task properties
  pair_energy::Parameters originalParams = taskBase->property("pair_params").value<pair_energy::Parameters>();
  xtb::Parameters xtbParams = taskBase->property("xtb_params").value<xtb::Parameters>();

  // Debug: check task state before getting result
  QString taskName = taskBase->property("name").toString();
  QString taskBaseName = taskBase->property("basename").toString();
  qDebug() << "onXtbTaskComplete: task name=" << taskName << "basename=" << taskBaseName;

  xtb::Result result = task->getResult();

  qDebug() << "Xtb task complete result.name=" << result.name << "jsonSize=" << result.jsonContents.size();

  // Skip tasks with invalid results
  if (result.name.isEmpty()) {
    qWarning() << "Skipping XTB task with empty name (task basename was" << taskBaseName << ")";
    return;
  }

  if (result.jsonContents.isEmpty()) {
    qWarning() << "Skipping XTB task" << result.name << "with empty JSON";
    return;
  }
  if (xtbParams.structure) {
    auto *interactions = xtbParams.structure->pairInteractions();
    auto *wfn = new MolecularWavefunction();
    bool success =
        io::populateWavefunctionFromJsonContents(wfn, result.jsonContents);
    success = success & io::populateWavefunctionFromXtbStdoutContents(
                            wfn, result.stdoutContents);
    auto pair = new PairInteraction(xtb::methodToString(xtbParams.method));
    qDebug() << success << "wfn->totalEnergy" << wfn->totalEnergy() << "ref"
             << xtbParams.reference_energy;
    if (!success) {
      qWarning() << "Invalid result from xtb task!";
    }

    // Use provider system for interaction energy calculation
    SimpleEnergyProvider combinedProvider(wfn->totalEnergy(),
                                          xtb::methodToString(xtbParams.method));

    // Get original parameters to access monomer wavefunctions
    if (originalParams.wfnA && originalParams.wfnB) {
      MolecularWavefunctionProvider providerA(originalParams.wfnA);
      MolecularWavefunctionProvider providerB(originalParams.wfnB);

      auto calcResult = InteractionEnergyCalculator::calculateInteraction(
          &combinedProvider, &providerA, &providerB);

      if (calcResult.success) {
        double e = calcResult.interactionEnergy;
        pair->addComponent("Total", e * 2625.5);
        qDebug() << "Provider-based calculation:" << calcResult.description
                 << "Energy:" << e << "kJ/mol:" << (e * 2625.5);
      } else {
        qWarning() << "Provider-based calculation failed:"
                   << calcResult.description;
        // Fallback to original calculation
        double e = wfn->totalEnergy() - xtbParams.reference_energy;
        pair->addComponent("Total", e * 2625.5);
      }
    } else {
      qWarning() << "No monomer wavefunctions available, using fallback";
      double e = wfn->totalEnergy() - xtbParams.reference_energy;
      pair->addComponent("Total", e * 2625.5);
    }
    pair->setParameters(originalParams);
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
