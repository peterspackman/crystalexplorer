#include "wavefunction_calculator.h"
#include "exefileutilities.h"
#include "load_wavefunction.h"
#include "molecular_wavefunction.h"
#include "occinput.h"
#include "occwavefunctiontask.h"
#include "orcainput.h"
#include "orcatask.h"
#include "xtbtask.h"
#include "settings.h"
#include <QFile>
#include <QTextStream>
#include <occ/core/element.h>

inline QString generateWavefunctionName(const wfn::Parameters &params) {
  QString methodString = params.method + " " + params.basis;
  if (params.isXtbMethod()) {
    methodString = params.method;
  }
  QString atomsString;
  occ::Vec3 meanPos(0.0, 0.0, 0.0);
  if (params.structure) {
    atomsString =
        params.structure->formulaSumForAtoms(params.atoms, false).remove(' ');
    meanPos = params.structure->atomicPositionsForIndices(params.atoms)
                  .rowwise()
                  .mean();
  }

  return QString("%1 %2 @ [%3, %4, %5]")
      .arg(methodString)
      .arg(atomsString)
      .arg(meanPos(0))
      .arg(meanPos(1))
      .arg(meanPos(2));
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
  result.userEditRequested = params.userEditRequested;
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
  result.userEditRequested = params.userEditRequested;
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
  // No longer connect to shared signal - we'll connect to individual tasks
}

void WavefunctionCalculator::setTaskManager(TaskManager *mgr) {
  m_taskManager = mgr;
}

Task *WavefunctionCalculator::makeOccTask(wfn::Parameters params) {
  QString filename;

  auto idx = params.structure->atomsWithFlags(AtomFlag::Selected);
  occ::IVec nums = params.structure->atomicNumbersForIndices(idx);
  occ::Mat3N pos = params.structure->atomicPositionsForIndices(idx);

  QString wavefunctionName = generateWavefunctionName(params);

  if (params.userEditRequested) {
    params.userInputContents = io::requestUserTextEdit(
        "OCC input", io::getOccWavefunctionJson(params));
    if (params.userInputContents.isEmpty()) {
      qInfo() << "Wavefunction calculation canceled by user";
      return nullptr;
    }
  }
  auto *task = new OccWavefunctionTask();
  task->setParameters(params);
  task->setProperty("name", wavefunctionName);
  task->setProperty("basename", wavefunctionName);
  task->setExecutable(m_occExecutable);
  task->setEnvironment(m_environment);
  task->setDeleteWorkingFiles(m_deleteWorkingFiles);
  QString wavefunctionFilename = task->wavefunctionFilename();

  // Store context in task properties for safe retrieval in slot
  task->setProperty("wfn_params", QVariant::fromValue(params));
  task->setProperty("wfn_filename", wavefunctionFilename);
  task->setProperty("wfn_name", wavefunctionName);

  connect(task, &Task::completed, this, [this]() {
    onWavefunctionTaskComplete();
  });

  return task;
}

Task *WavefunctionCalculator::makeOrcaTask(wfn::Parameters params) {
  QString wavefunctionName = generateWavefunctionName(params);

  if (params.userEditRequested) {
    params.userInputContents =
        io::requestUserTextEdit("OCC input", io::orcaInputString(params));
    if (params.userInputContents.isEmpty()) {
      qInfo() << "Wavefunction calculation canceled by user";
      return nullptr;
    }
  }

  auto *task = new OrcaWavefunctionTask();
  task->setParameters(params);
  task->setProperty("name", wavefunctionName);
  task->setExecutable(m_orcaExecutable);
  task->setEnvironment(m_environment);
  task->setDeleteWorkingFiles(m_deleteWorkingFiles);
  QString wavefunctionFilename = task->moldenFilename();
  qDebug() << "Molden filename" << wavefunctionFilename;

  // Store context in task properties for safe retrieval in slot
  task->setProperty("wfn_params", QVariant::fromValue(params));
  task->setProperty("wfn_filename", wavefunctionFilename);
  task->setProperty("wfn_name", wavefunctionName);

  connect(task, &Task::completed, this, [this]() {
    onWavefunctionTaskComplete();
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
  switch (params.program) {
  case wfn::Program::Occ:
    task = makeOccTask(params);
    break;
  case wfn::Program::Orca:
    task = makeOrcaTask(params);
    break;
  default:
    qWarning() << "Unsupported program" << wfn::programName(params.program);
    break;
  }
  if (task) {
    auto taskId = m_taskManager->add(task);
    qDebug() << "Single task started with id:" << taskId;
  }
}

void WavefunctionCalculator::start(xtb::Parameters params) {
  if (!params.structure) {
    qDebug()
        << "Found nullptr for chemical structure in WavefunctionCalculator";
    return;
  }
  m_structure = params.structure;

  // Create task, connect to it, then submit to TaskManager
  XtbTask *task = m_xtb->createTask(params);
  if (task) {
    connect(task, &Task::completed, this, [this]() {
      onXtbTaskComplete();
    });
    m_taskManager->add(task);  // TaskManager starts it
  }
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

    switch (params.program) {
    case wfn::Program::Occ:
      task = makeOccTask(params);
      break;
    case wfn::Program::Orca:
      task = makeOrcaTask(params);
      break;
    default:
      qWarning() << "Unsupported program" << wfn::programName(params.program);
      break;
    }
    if (task) {
      tasks.append(task);
    }
  }

  for (auto *task : tasks) {
    // submit all the tasks at once
    auto taskId = m_taskManager->add(task);
  }
}

void WavefunctionCalculator::onWavefunctionTaskComplete() {
  Task *task = qobject_cast<Task*>(sender());
  if (!task) {
    qWarning() << "onWavefunctionTaskComplete called with invalid sender";
    return;
  }

  // Retrieve context from task properties
  wfn::Parameters params = task->property("wfn_params").value<wfn::Parameters>();
  QString filename = task->property("wfn_filename").toString();
  QString name = task->property("wfn_name").toString();

  // Process the completed task
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
    io::deleteFile(filename);
  }
}

void WavefunctionCalculator::onXtbTaskComplete() {
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

  // Retrieve params and result from task
  xtb::Parameters params = taskBase->property("xtb_params").value<xtb::Parameters>();

  // Debug: check task state
  QString taskName = taskBase->property("name").toString();
  qDebug() << "onXtbTaskComplete (WavefunctionCalculator): task name=" << taskName;

  xtb::Result result = task->getResult();

  qDebug() << "Task result.name=" << result.name << "jsonSize=" << result.jsonContents.size()
           << "finished in WavefunctionCalculator";

  // Validate result
  if (result.name.isEmpty()) {
    qWarning() << "Skipping XTB task with empty name (task name was" << taskName << ")";
    return;
  }

  if (result.jsonContents.isEmpty() && result.moldenContents.isEmpty()) {
    qWarning() << "Skipping XTB task" << result.name << "with no output data";
    return;
  }
  auto *wfn = new MolecularWavefunction();
  bool success =
      io::populateWavefunctionFromJsonContents(wfn, result.jsonContents);
  success =
      io::populateWavefunctionFromMoldenContents(wfn, result.moldenContents);
  wfn->setRawContents(result.moldenContents);
  wfn->setParameters(xtb2wfn(params));
  wfn->setFileFormat(wfn::FileFormat::Molden);
  m_wavefunction = wfn;

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

#ifdef Q_OS_WASM
// WASM stubs - these should never be called as external programs aren't supported
// but we need the symbols for linking
#warning "WavefunctionCalculator will not work in WASM - external programs not supported"
#endif
