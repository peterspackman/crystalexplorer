#include "isosurface_calculator.h"
#include "occsurfacetask.h"
#include "load_mesh.h"
#include <occ/core/element.h>
#include "crystalstructure.h"
#include "xyzfile.h"

namespace volume {

IsosurfaceCalculator::IsosurfaceCalculator(QObject * parent) : QObject(parent) {}

void IsosurfaceCalculator::setTaskManager(TaskManager *mgr) {
    m_taskManager = mgr;
}

inline bool writeByteArrayToFile(const QByteArray &contents, const QString &filename) {
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
	file.write(contents);
	file.close();
	return true;
    }
    else {
	qDebug() << "Could not open file for writing in MolecularWavefunction::writeToFile";
	return false;
    }
}

void IsosurfaceCalculator::start(isosurface::Parameters params) {
  if(!params.structure) {
    qDebug() << "Found nullptr for chemical structure in IsosurfaceCalculator";
    return;
  }
  m_structure = params.structure;

  QString filename, filename_outside;
  m_atoms = {};

  if(params.kind == isosurface::Kind::Void) {
      CrystalStructure *crystal = qobject_cast<CrystalStructure *>(m_structure);
      if(!crystal) return;
      filename = m_structure->name() + "_" + isosurface::kindToString(params.kind) + ".cif";

      if(!writeByteArrayToFile(crystal->fileContents(), filename)) return;
  }
  else {
      m_atoms = params.structure->atomsWithFlags(AtomFlag::Selected);
      occ::IVec nums = params.structure->atomicNumbersForIndices(m_atoms);
      occ::Mat3N pos = params.structure->atomicPositionsForIndices(m_atoms);

      if(params.wfn) {
        filename = m_structure->name() + "_" + isosurface::kindToString(params.kind) + "_inside.owf.json";
	params.wfn->writeToFile(filename);
      }
      else {
        filename = m_structure->name() + "_" + isosurface::kindToString(params.kind) + "_inside.xyz";
	XYZFile xyz;
	xyz.setElements(nums);
	xyz.setAtomPositions(pos);
	xyz.writeToFile(filename);
      }
      filename_outside = m_structure->name() + "_" + isosurface::kindToString(params.kind) + "_outside.xyz";
      {
	  auto idxs = params.structure->atomsSurroundingAtomsWithFlags(AtomFlag::Selected, 12.0);
	  qDebug() << "Idxs size: " << idxs.size();
	  auto nums_outside = params.structure->atomicNumbersForIndices(idxs);
	  auto pos_outside = params.structure->atomicPositionsForIndices(idxs);
	  XYZFile xyz;
	  xyz.setElements(nums_outside);
	  xyz.setAtomPositions(pos_outside);
	  xyz.writeToFile(filename_outside);
      }
  }

  QString surfaceName = isosurface::kindToString(params.kind);
  OccSurfaceTask * surface_task = new OccSurfaceTask();
  surface_task->setSurfaceParameters(params);
  surface_task->setProperty("name", surfaceName);
  surface_task->setProperty("inputFile", filename);
  surface_task->setProperty("environmentFile", filename_outside);
  qDebug() << "Generating " << isosurface::kindToString(params.kind) << "surface with isovalue: " << params.isovalue;
  surface_task->setProperty("isovalue", params.isovalue);

  auto taskId = m_taskManager->add(surface_task);
  m_name = surfaceName;
  m_filename = "surface.ply";

  connect(surface_task, &Task::completed, this, &IsosurfaceCalculator::surfaceComplete);
}

void IsosurfaceCalculator::surfaceComplete() {
    qDebug() << "Task" << m_name << "finished in IsosurfaceCalculator";
    Mesh * mesh = io::loadMesh(m_filename);
    mesh->setObjectName(m_name);
    mesh->setAtoms(m_atoms);
    mesh->setParent(m_structure);
}

}
