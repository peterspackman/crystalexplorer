#include "isosurface_calculator.h"
#include "exefileutilities.h"
#include "crystalstructure.h"
#include "load_mesh.h"
#include "meshinstance.h"
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
  m_deleteWorkingFiles = settings::readSetting(settings::keys::DELETE_WORKING_FILES).toBool();
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
      if(!xyz.writeToFile(filename)) return;
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
      if(!xyz.writeToFile(filename_outside)) return;
    }
  }
  m_parameters = params;
  m_name = surfaceName();

  m_defaultProperty = isosurface::defaultPropertyForKind(params.kind);
  OccSurfaceTask *surface_task = new OccSurfaceTask();
  surface_task->setExecutable(m_occExecutable);
  surface_task->setEnvironment(m_environment);
  surface_task->setSurfaceParameters(params);
  surface_task->setProperty("name", m_name);
  surface_task->setProperty("inputFile", filename);
  surface_task->setProperty("environmentFile", filename_outside);
  surface_task->setDeleteWorkingFiles(m_deleteWorkingFiles);
  qDebug() << "Generating " << isosurface::kindToString(params.kind)
           << "surface with isovalue: " << params.isovalue;
  surface_task->setProperty("isovalue", params.isovalue);

  auto taskId = m_taskManager->add(surface_task);
  m_filename = "surface.ply";

  connect(surface_task, &Task::completed, this,
          &IsosurfaceCalculator::surfaceComplete);
}

QString IsosurfaceCalculator::surfaceName() {
    isosurface::SurfaceDescription desc = isosurface::getSurfaceDescription(m_parameters.kind);
    return QString("%1 (%2) [isovalue = %3]").arg(desc.displayName).arg(m_parameters.separation).arg(m_parameters.isovalue);
}

void IsosurfaceCalculator::surfaceComplete() {
  qDebug() << "Task" << m_name << "finished in IsosurfaceCalculator";
  Mesh *mesh = io::loadMesh(m_filename);
  if(m_deleteWorkingFiles) {
    exe::deleteFile(m_filename);
  }
  mesh->setObjectName(m_name);
  mesh->setParameters(m_parameters);
  mesh->setSelectedProperty(m_defaultProperty);
  mesh->setAtomsInside(m_atomsInside);
  mesh->setAtomsOutside(m_atomsOutside);
  mesh->setParent(m_structure);
  // create the child instance that will be shown
  MeshInstance *instance = new MeshInstance(mesh);
  instance->setObjectName("+ {x,y,z} [0,0,0]");

}

} // namespace volume
