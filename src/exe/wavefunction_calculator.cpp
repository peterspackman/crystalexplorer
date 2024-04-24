#include "wavefunction_calculator.h"
#include "molecular_wavefunction.h"
#include "load_wavefunction.h"
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
  task->setParameters(params);
  task->setProperty("name", wavefunctionName);
  QString wavefunctionFilename = task->wavefunctionFilename();

  auto taskId = m_taskManager->add(task);
  connect(task, &Task::completed, [&, params, wavefunctionName, wavefunctionFilename]() {
	this->wavefunctionComplete(params, wavefunctionFilename, wavefunctionName);
  });

}

void WavefunctionCalculator::wavefunctionComplete(wfn::Parameters params, QString filename, QString name) {
    qDebug() << "Task" << name << "finished in WavefunctionCalculator";
    m_wavefunction = io::loadWavefunction(filename);
    if(m_wavefunction) {
	m_wavefunction->setParameters(params);
	m_wavefunction->setObjectName(name);
	m_wavefunction->setParent(m_structure);
	emit calculationComplete();
    }
}

MolecularWavefunction * WavefunctionCalculator::getWavefunction() const {
    return m_wavefunction;
}
