#include "wavefunction_calculator.h"
#include "molecular_wavefunction.h"
#include "occwavefunctiontask.h"
#include <occ/core/element.h>
#include <QFile>
#include <QTextStream>

WavefunctionCalculator::WavefunctionCalculator(QObject * parent) : QObject(parent) {}

void WavefunctionCalculator::setTaskManager(TaskManager *mgr) {
    m_taskManager = mgr;
}

void WavefunctionCalculator::start(wfn::Parameters params) {
  if(!params.structure) {
    qDebug() << "Found nullptr for chemical structure in WavefunctionCalculator";
    return;
  }
  m_structure = params.structure;

  QString filename, filename_outside;

  std::vector<int> idx = params.structure->atomIndicesWithFlags(AtomFlag::Selected);
  occ::IVec nums = params.structure->atomicNumbers()(idx);
  occ::Mat3N pos = params.structure->atomicPositions()(Eigen::all, idx);

  QString wavefunctionName = QString("%1/%2").arg(params.method).arg(params.basis);
  auto * task = new OccWavefunctionTask();
  task->setWavefunctionParameters(params);
  task->setProperty("name", wavefunctionName);

  auto taskId = m_taskManager->add(task);
  connect(task, &Task::completed, [&, wavefunctionName]() {
	this->wavefunctionComplete(wavefunctionName);
  });

}

void WavefunctionCalculator::wavefunctionComplete(QString name) {
    qDebug() << "Task" << name << "finished in WavefunctionCalculator";
    MolecularWavefunction * wfn = new MolecularWavefunction();
    wfn->setParent(m_structure);
}
