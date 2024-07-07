#include "isosurface_calculator.h"
#include "crystalstructure.h"
#include "load_mesh.h"
#include "occsurfacetask.h"
#include "xyzfile.h"
#include "settings.h"
#include <occ/core/element.h>

namespace volume {

IsosurfaceCalculator::IsosurfaceCalculator(QObject *parent) : QObject(parent) {
  // TODO streamline this
  m_occExecutable = settings::readSetting(settings::keys::OCC_EXECUTABLE).toString();
  m_environment = QProcessEnvironment::systemEnvironment();
  QString dataDir = settings::readSetting(settings::keys::OCC_DATA_DIRECTORY).toString();
  m_environment.insert("OCC_DATA_PATH", dataDir);
  m_environment.insert("OCC_BASIS_PATH", dataDir);
}

void IsosurfaceCalculator::setTaskManager(TaskManager *mgr) {
  m_taskManager = mgr;
}

inline bool writeByteArrayToFile(const QByteArray &contents,
                                 const QString &filename) {
  QFile file(filename);
  if (file.open(QIODevice::WriteOnly)) {
    file.write(contents);
    file.close();
    return true;
  } else {
    qDebug() << "Could not open file for writing in "
                "MolecularWavefunction::writeToFile";
    return false;
  }
}

void IsosurfaceCalculator::start(isosurface::Parameters params) {
  if (!params.structure) {
    qDebug() << "Found nullptr for chemical structure in IsosurfaceCalculator";
    return;
  }
  m_structure = params.structure;

  QString filename, filename_outside;
  m_atomsInside = {};
  m_atomsOutside = {};

  if (params.kind == isosurface::Kind::Void) {
    CrystalStructure *crystal = qobject_cast<CrystalStructure *>(m_structure);
    if (!crystal)
      return;
    filename = m_structure->name() + "_" +
               isosurface::kindToString(params.kind) + ".cif";

    if (!writeByteArrayToFile(crystal->fileContents(), filename))
      return;
  } else {
    m_atomsInside = params.structure->atomsWithFlags(AtomFlag::Selected);
    occ::IVec nums = params.structure->atomicNumbersForIndices(m_atomsInside);
    occ::Mat3N pos = params.structure->atomicPositionsForIndices(m_atomsInside);

    if (params.wfn) {
      filename = m_structure->name() + "_" +
                 isosurface::kindToString(params.kind) + "_inside.owf.json";
      params.wfn->writeToFile(filename);
    } else {
      filename = m_structure->name() + "_" +
                 isosurface::kindToString(params.kind) + "_inside.xyz";
      XYZFile xyz;
      xyz.setElements(nums);
      xyz.setAtomPositions(pos);
      xyz.writeToFile(filename);
    }
    filename_outside = m_structure->name() + "_" +
                       isosurface::kindToString(params.kind) + "_outside.xyz";
    {
      m_atomsOutside = params.structure->atomsSurroundingAtomsWithFlags(
          AtomFlag::Selected, 12.0);
      auto nums_outside =
          params.structure->atomicNumbersForIndices(m_atomsOutside);
      auto pos_outside =
          params.structure->atomicPositionsForIndices(m_atomsOutside);

      XYZFile xyz;
      xyz.setElements(nums_outside);
      xyz.setAtomPositions(pos_outside);
      xyz.writeToFile(filename_outside);
    }
  }

  QString surfaceName = isosurface::kindToString(params.kind);
  m_defaultProperty = isosurface::defaultPropertyForKind(params.kind);
  OccSurfaceTask *surface_task = new OccSurfaceTask();
  surface_task->setExecutable(m_occExecutable);
  surface_task->setEnvironment(m_environment);
  surface_task->setSurfaceParameters(params);
  surface_task->setProperty("name", surfaceName);
  surface_task->setProperty("inputFile", filename);
  surface_task->setProperty("environmentFile", filename_outside);
  qDebug() << "Generating " << isosurface::kindToString(params.kind)
           << "surface with isovalue: " << params.isovalue;
  surface_task->setProperty("isovalue", params.isovalue);

  auto taskId = m_taskManager->add(surface_task);
  m_name = surfaceName;
  m_filename = "surface.ply";
  m_parameters = params;

  connect(surface_task, &Task::completed, this,
          &IsosurfaceCalculator::surfaceComplete);
}

void IsosurfaceCalculator::surfaceComplete() {
  qDebug() << "Task" << m_name << "finished in IsosurfaceCalculator";
  Mesh *mesh = io::loadMesh(m_filename);
  mesh->setObjectName(m_name);
  mesh->setParameters(m_parameters);
  mesh->setSelectedProperty(m_defaultProperty);
  mesh->setAtomsInside(m_atomsInside);
  mesh->setAtomsOutside(m_atomsOutside);
  mesh->setParent(m_structure);
}

} // namespace volume
