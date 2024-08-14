#include "wavefunction_calculator.h"
#include "exefileutilities.h"
#include "load_wavefunction.h"
#include "molecular_wavefunction.h"
#include "occwavefunctiontask.h"
#include "orcatask.h"
#include "settings.h"
#include <QFile>
#include <QTextStream>
#include <occ/core/element.h>

inline QString generateWavefunctionName(const wfn::Parameters &params) {
  QString methodString = params.method + "/" + params.basis;
  if (params.isXtbMethod()) {
    methodString = params.method;
  }
  QString atomsString;
  occ::Vec3 meanPos(0.0, 0.0, 0.0);
  if (params.structure) {
    atomsString = params.structure->formulaSumForAtoms(params.atoms, false).remove(' ');
    meanPos = params.structure->atomicPositionsForIndices(params.atoms).rowwise().mean();
  }

  return QString("%1 %2 @ [%3, %4, %5]").arg(methodString).arg(atomsString).arg(meanPos(0)).arg(meanPos(1)).arg(meanPos(2));
}

inline xtb::Parameters wfn2xtb(const wfn::Parameters &params) {
  xtb::Parameters result;
  result.charge = params.charge;
  result.multiplicity = params.multiplicity;
  result.method = xtb::stringToMethod(params.method);
  result.structure = params.structure;
  result.atoms = params.atoms;
  result.accepted = params.accepted;
  result.write_molden = true;
  return result;
}

inline wfn::Parameters xtb2wfn(const xtb::Parameters &params) {
  wfn::Parameters result;
  result.charge = params.charge;
  result.multiplicity = params.multiplicity;
  result.method = xtb::methodToString(params.method);
  result.basis = "";
  result.structure = params.structure;
  result.atoms = params.atoms;
  result.accepted = params.accepted;
  return result;
}

WavefunctionCalculator::WavefunctionCalculator(QObject *parent)
    : QObject(parent) {
  m_orcaExecutable =
      settings::readSetting(settings::keys::ORCA_EXECUTABLE).toString();
  m_occExecutable =
      settings::readSetting(settings::keys::OCC_EXECUTABLE).toString();

  m_environment = QProcessEnvironment::systemEnvironment();
  QString dataDir =
      settings::readSetting(settings::keys::OCC_DATA_DIRECTORY).toString();
  m_deleteWorkingFiles =
      settings::readSetting(settings::keys::DELETE_WORKING_FILES).toBool();
  m_environment.insert("OCC_DATA_PATH", dataDir);
  m_environment.insert("OCC_BASIS_PATH", dataDir);
  m_xtb = new XtbEnergyCalculator(this);
  connect(m_xtb, &XtbEnergyCalculator::calculationComplete, this,
          &WavefunctionCalculator::handleXtbTaskComplete);
}

void WavefunctionCalculator::setTaskManager(TaskManager *mgr) {
  m_taskManager = mgr;
  m_xtb->setTaskManager(mgr);
}

Task *WavefunctionCalculator::makeOccTask(wfn::Parameters params) {
  QString filename;

  auto idx = params.structure->atomsWithFlags(AtomFlag::Selected);
  occ::IVec nums = params.structure->atomicNumbersForIndices(idx);
  occ::Mat3N pos = params.structure->atomicPositionsForIndices(idx);

  QString wavefunctionName = generateWavefunctionName(params);
  auto *task = new OccWavefunctionTask();
  task->setParameters(params);
  task->setProperty("name", wavefunctionName);
  task->setExecutable(m_occExecutable);
  task->setEnvironment(m_environment);
  task->setDeleteWorkingFiles(m_deleteWorkingFiles);
  QString wavefunctionFilename = task->wavefunctionFilename();

  connect(task, &Task::completed,
          [&, params, wavefunctionName, wavefunctionFilename]() {
            this->handleTaskComplete(params, wavefunctionFilename,
                                     wavefunctionName);
          });

  return task;
}

Task *WavefunctionCalculator::makeOrcaTask(wfn::Parameters params) {
  QString wavefunctionName = generateWavefunctionName(params);
  auto *task = new OrcaWavefunctionTask();
  task->setParameters(params);
  task->setProperty("name", wavefunctionName);
  task->setExecutable(m_orcaExecutable);
  task->setEnvironment(m_environment);
  task->setDeleteWorkingFiles(m_deleteWorkingFiles);
  QString wavefunctionFilename = task->moldenFilename();
  qDebug() << "Molden filename" << wavefunctionFilename;

  connect(task, &Task::completed,
          [&, params, wavefunctionName, wavefunctionFilename]() {
            this->handleTaskComplete(params, wavefunctionFilename,
                                     wavefunctionName);
          });

  return task;
}

void WavefunctionCalculator::start(wfn::Parameters params) {
  if (!params.structure) {
    qDebug()
        << "Found nullptr for chemical structure in WavefunctionCalculator";
    return;
  }
  m_structure = params.structure;
  qDebug() << "Is xtb method?" << params.isXtbMethod() << params.method;

  m_complete = false;
  m_completedTaskCount = 0;
  m_totalTasks = 1;

  if (params.isXtbMethod()) {
    xtb::Parameters xtb_params = wfn2xtb(params);
    xtb_params.name = generateWavefunctionName(params);
    start(xtb_params);
    return;
  }

  Task *task = nullptr;
  // TODO check which ones are needed
  if (!m_orcaExecutable.isEmpty()) {
    task = makeOrcaTask(params);
  } else {
    task = makeOccTask(params);
  }
  auto taskId = m_taskManager->add(task);
  qDebug() << "Single task started with id:" << taskId;
}

void WavefunctionCalculator::start(xtb::Parameters params) {
  if (!params.structure) {
    qDebug()
        << "Found nullptr for chemical structure in WavefunctionCalculator";
    return;
  }
  m_structure = params.structure;
  m_xtb->start(params);
}

void WavefunctionCalculator::start_batch(
    const std::vector<wfn::Parameters> &wfn) {
  QList<Task *> tasks;
  m_completedTaskCount = 0;
  m_totalTasks = wfn.size();

  int idx{0};

  for (const auto &params : wfn) {
    if (!params.structure) {
      qDebug()
          << "Found nullptr for chemical structure in WavefunctionCalculator";
      continue;
    }
    // assume they're all for the same structure.
    m_structure = params.structure;
    qDebug() << "Is xtb method?" << params.isXtbMethod() << params.method;
    if (params.isXtbMethod()) {
      xtb::Parameters xtb_params = wfn2xtb(params);
      // TODO Name generation based on fragment
      xtb_params.name = generateWavefunctionName(params);
      start(xtb_params);
      continue;
    }

    Task *task = nullptr;
    // TODO check which ones are needed
    if (!m_orcaExecutable.isEmpty()) {
      task = makeOrcaTask(params);
    } else {
      task = makeOccTask(params);
    }

    tasks.append(task);
  }

  for (auto *task : tasks) {
    // submit all the tasks at once
    auto taskId = m_taskManager->add(task);
  }
}

void WavefunctionCalculator::handleTaskComplete(wfn::Parameters params,
                                                QString filename,
                                                QString name) {
  qDebug() << "Task" << name << "finished in WavefunctionCalculator";
  auto wfn = io::loadWavefunction(filename);
  qDebug() << "Loaded wavefunction from" << filename << wfn
           << params.atoms.size();
  m_wavefunction = wfn;
  m_completedTaskCount++;

  if (m_completedTaskCount == m_totalTasks) {
    m_complete = true;
  }

  if (wfn) {
    wfn->setParameters(params);
    wfn->setObjectName(name);
    wfn->setParent(m_structure);
    if (m_complete) {
      emit calculationComplete();
    }
  }

  if (m_deleteWorkingFiles) {
    exe::deleteFile(filename);
  }
}

void WavefunctionCalculator::handleXtbTaskComplete(xtb::Parameters params,
                                                   xtb::Result result) {
  qDebug() << "Task" << result.name << "finished in WavefunctionCalculator";
  auto *wfn = new MolecularWavefunction();
  bool success =
      io::populateWavefunctionFromJsonContents(wfn, result.jsonContents);
  success =
      io::populateWavefunctionFromMoldenContents(wfn, result.moldenContents);
  wfn->setRawContents(result.moldenContents);
  wfn->setParameters(xtb2wfn(params));
  wfn->setFileFormat(wfn::FileFormat::Molden);

  qDebug() << m_completedTaskCount << m_totalTasks;
  m_completedTaskCount++;

  if (m_completedTaskCount == m_totalTasks) {
    qDebug() << "Setting complete to true";
    m_complete = true;
  }

  if (result.success) {
    auto kv = result.energy.find("total");
    if (kv != result.energy.end()) {
      wfn->setTotalEnergy(kv->second);
    }
    for (const auto &[k, v] : result.energy) {
      qDebug() << k << v;
    }
  }
  wfn->setObjectName(result.name);
  wfn->setParent(m_structure);
  if (m_complete) {
    emit calculationComplete();
  }
}

MolecularWavefunction *WavefunctionCalculator::getWavefunction() const {
  return m_wavefunction;
}
