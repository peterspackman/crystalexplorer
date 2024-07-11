#include "wavefunction_calculator.h"
#include "load_wavefunction.h"
#include "molecular_wavefunction.h"
#include "occwavefunctiontask.h"
#include "settings.h"
#include <QFile>
#include <QTextStream>
#include <occ/core/element.h>

WavefunctionCalculator::WavefunctionCalculator(QObject *parent)
    : QObject(parent) {
  m_occExecutable =
      settings::readSetting(settings::keys::OCC_EXECUTABLE).toString();
  m_environment = QProcessEnvironment::systemEnvironment();
  QString dataDir =
      settings::readSetting(settings::keys::OCC_DATA_DIRECTORY).toString();
  m_environment.insert("OCC_DATA_PATH", dataDir);
  m_environment.insert("OCC_BASIS_PATH", dataDir);
}

void WavefunctionCalculator::setTaskManager(TaskManager *mgr) {
  m_taskManager = mgr;
}

void WavefunctionCalculator::start(wfn::Parameters params) {
  if (!params.structure) {
    qDebug()
        << "Found nullptr for chemical structure in WavefunctionCalculator";
    return;
  }
  m_structure = params.structure;

  QString filename, filename_outside;

  std::vector<int> idx =
      params.structure->atomIndicesWithFlags(AtomFlag::Selected);
  occ::IVec nums = params.structure->atomicNumbers()(idx);
  occ::Mat3N pos = params.structure->atomicPositions()(Eigen::all, idx);

  QString wavefunctionName =
      QString("%1/%2").arg(params.method).arg(params.basis);
  auto *task = new OccWavefunctionTask();
  task->setParameters(params);
  task->setProperty("name", wavefunctionName);
  task->setExecutable(m_occExecutable);
  task->setEnvironment(m_environment);
  QString wavefunctionFilename = task->wavefunctionFilename();

  auto taskId = m_taskManager->add(task);
  connect(task, &Task::completed,
          [&, params, wavefunctionName, wavefunctionFilename]() {
            m_complete = true;
            this->wavefunctionComplete(params, wavefunctionFilename,
                                       wavefunctionName);
          });
}

void WavefunctionCalculator::start_batch(
    const std::vector<wfn::Parameters> &wfn) {
  QList<OccWavefunctionTask *> tasks;
  completedTaskCount = 0;

  for (const auto &params : wfn) {
    if (!params.structure) {
      qDebug()
          << "Found nullptr for chemical structure in WavefunctionCalculator";
      continue;
    }
    // assume they're all for the same structure.
    m_structure = params.structure;

    QString wavefunctionName =
        QString("%1/%2").arg(params.method).arg(params.basis);
    auto *task = new OccWavefunctionTask();
    task->setParameters(params);
    task->setProperty("name", wavefunctionName);
    task->setExecutable(m_occExecutable);
    task->setEnvironment(m_environment);
    QString wavefunctionFilename = task->wavefunctionFilename();

    auto taskId = m_taskManager->add(task);
    tasks.append(task);

    connect(task, &Task::completed, this,
            [this, params, wavefunctionName, wavefunctionFilename, tasks]() {
              completedTaskCount++;

              if (completedTaskCount == tasks.size()) {
                m_complete = true;
              }

              this->wavefunctionComplete(params, wavefunctionFilename,
                                         wavefunctionName);
            });
  }
}

void WavefunctionCalculator::wavefunctionComplete(wfn::Parameters params,
                                                  QString filename,
                                                  QString name) {
  qDebug() << "Task" << name << "finished in WavefunctionCalculator";
  auto wfn = io::loadWavefunction(filename);
  qDebug() << "Loaded wavefunction from" << filename << wfn
           << params.atoms.size();
  m_wavefunction = wfn;
  if (wfn) {
    wfn->setParameters(params);
    wfn->setObjectName(name);
    wfn->setParent(m_structure);
    if (m_complete) {
      emit calculationComplete();
    }
  }
}

MolecularWavefunction *WavefunctionCalculator::getWavefunction() const {
  return m_wavefunction;
}
