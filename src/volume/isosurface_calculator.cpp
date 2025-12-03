#include "isosurface_calculator.h"
#include "crystalstructure.h"
#include "exefileutilities.h"
#include "load_mesh.h"
#include "meshinstance.h"
#include "occsurfacetask.h"
#include "settings.h"
#include "xyzfile.h"
#include <occ/core/element.h>

namespace volume {

inline QString surfaceName(const isosurface::Parameters &parameters,
                           int index) {
  return isosurface::generateSurfaceName(parameters);
}

inline Mesh::Attributes makeAttributes(const isosurface::Parameters &params) {
  Mesh::Attributes attr;
  attr.kind = params.kind;
  attr.isovalue = params.isovalue;
  attr.separation = params.separation;
  return attr;
}

IsosurfaceCalculator::IsosurfaceCalculator(QObject *parent) : QObject(parent) {
  // TODO streamline this
  m_occExecutable =
      settings::readSetting(settings::keys::OCC_EXECUTABLE).toString();
  m_environment = QProcessEnvironment::systemEnvironment();
  QString dataDir =
      settings::readSetting(settings::keys::OCC_DATA_DIRECTORY).toString();
  m_deleteWorkingFiles =
      settings::readSetting(settings::keys::DELETE_WORKING_FILES).toBool();
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

  QString interiorFilename, exteriorFilename, wavefunctionFilename;
  m_atomsInside = {};
  m_atomsOutside = {};

  if (params.wfn) {
    QString suffix = params.wfn->fileFormatSuffix();
    wavefunctionFilename = m_structure->name() + "_wfn" + suffix;
    if (!params.wfn->writeToFile(wavefunctionFilename)) {
      qWarning() << "Failed to write wavefunction file:" << wavefunctionFilename;
      emit errorOccurred("Failed to write wavefunction file: " + wavefunctionFilename);
      return;
    }
  }

  if (params.kind == isosurface::Kind::Void) {
    CrystalStructure *crystal = qobject_cast<CrystalStructure *>(m_structure);
    if (!crystal)
      return;
    interiorFilename = m_structure->name() + "_" +
                       isosurface::kindToString(params.kind) + ".cif";

    if (!writeByteArrayToFile(crystal->fileContents(), interiorFilename))
      return;
  } else {
    m_atomsInside = params.structure->atomsWithFlags(AtomFlag::Selected);
    occ::IVec nums = params.structure->atomicNumbersForIndices(m_atomsInside);
    occ::Mat3N pos = params.structure->atomicPositionsForIndices(m_atomsInside);

    interiorFilename = m_structure->name() + "_" +
                       isosurface::kindToString(params.kind) + "_inside.xyz";
    XYZFile xyz;
    xyz.setElements(nums);
    xyz.setAtomPositions(pos);
    if (!xyz.writeToFile(interiorFilename))
      return;
    exteriorFilename = m_structure->name() + "_" +
                       isosurface::kindToString(params.kind) + "_outside.xyz";
    {
      // TODO expand this based on density or something
      m_atomsOutside = params.structure->atomsSurroundingAtomsWithFlags(
          AtomFlag::Selected, 12.0);
      auto nums_outside =
          params.structure->atomicNumbersForIndices(m_atomsOutside);
      auto pos_outside =
          params.structure->atomicPositionsForIndices(m_atomsOutside);

      XYZFile xyz;
      xyz.setElements(nums_outside);
      xyz.setAtomPositions(pos_outside);
      if (!xyz.writeToFile(exteriorFilename))
        return;
    }
  }
  m_parameters = params;
  m_name = surfaceName(params, 0);

  OccSurfaceTask *surfaceTask = new OccSurfaceTask();
  surfaceTask->setExecutable(m_occExecutable);
  surfaceTask->setEnvironment(m_environment);
  surfaceTask->setSurfaceParameters(params);
  surfaceTask->setProperty("name", m_name);
  surfaceTask->setProperty("inputFile", interiorFilename);
  surfaceTask->setProperty("environmentFile", exteriorFilename);
  surfaceTask->setProperty("wavefunctionFile", wavefunctionFilename);
  surfaceTask->setProperty("orbitalLabels", params.orbitalLabels);
  surfaceTask->setDeleteWorkingFiles(m_deleteWorkingFiles);
  qDebug() << "Generating " << isosurface::kindToString(params.kind)
           << "surface with isovalue: " << params.isovalue;
  surfaceTask->setProperty("isovalue", params.isovalue);
  if (params.computeNegativeIsovalue) {
    surfaceTask->setProperty("computeNegativeIsovalue", true);
  }
  
  // For slab structures, automatically enable background density for Hirshfeld surfaces
  if (params.structure && params.structure->structureType() == ChemicalStructure::StructureType::Surface) {
    if (params.kind == isosurface::Kind::Hirshfeld) {
      surfaceTask->setProperty("background_density", 0.002f);
      qDebug() << "Automatically enabled background density (0.002) for slab Hirshfeld surface";
    }
  }

  auto taskId = m_taskManager->add(surfaceTask);
  m_fileNames = surfaceTask->outputFileNames();

  connect(surfaceTask, &Task::completed, this,
          &IsosurfaceCalculator::surfaceComplete);
}

void setFragmentPatchForMesh(Mesh *mesh, ChemicalStructure *structure) {
  if (!mesh)
    return;
  if (!structure)
    return;
  ankerl::unordered_dense::map<FragmentIndex, int, FragmentIndexHash>
      fragmentIndices;

  Mesh::ScalarPropertyValues fragmentPatch(mesh->numberOfVertices());
  fragmentPatch.setConstant(-1.0);
  occ::IVec de_idxs = mesh->vertexProperty("External atom index").cast<int>();
  auto atomIndices = mesh->atomsOutside();
  for (int i = 0; i < de_idxs.size(); i++) {
    int idx = de_idxs(i);
    if (idx >= atomIndices.size()) {
      continue;
    }
    const auto &genericIndex = atomIndices[idx];
    FragmentIndex fidx = structure->fragmentIndexForGeneralAtom(genericIndex);
    if (fidx.u == -1) {
      continue;
    }
    const auto kv = fragmentIndices.find(fidx);
    if (kv != fragmentIndices.end()) {
      fragmentPatch(i) = kv->second;
    } else {
      fragmentPatch(i) = fragmentIndices.size();
      fragmentIndices.insert({fidx, fragmentIndices.size()});
    }
  }
  mesh->setVertexProperty("Fragment Patch", fragmentPatch);
}

void IsosurfaceCalculator::surfaceComplete() {
  qDebug() << "Task" << m_name << "finished in IsosurfaceCalculator";
  bool preload =
      settings::readSetting(settings::keys::PRELOAD_MESH_FILES).toBool();
  qDebug() << "Raeding" << m_fileNames;
  QList<Mesh *> meshes = io::loadMeshes(m_fileNames, preload);
  if (m_deleteWorkingFiles) {
    io::deleteFiles(m_fileNames);
  }
  int idx = 0;
  for (auto *mesh : meshes) {
    if (!mesh)
      continue;
    auto params = m_parameters;
    if (idx > 0)
      params.isovalue = -params.isovalue;
    mesh->setAtomsInside(m_atomsInside);
    mesh->setAtomsOutside(m_atomsOutside);
    setFragmentPatchForMesh(mesh, params.structure);

    // Set the mesh name using our improved naming
    QString meshName = surfaceName(params, idx);
    mesh->setObjectName(meshName);

    if (params.additionalProperties.size() > 0) {
      mesh->setSelectedProperty(isosurface::getSurfacePropertyDisplayName(
          params.additionalProperties[0]));
    } else {
      mesh->setSelectedProperty(isosurface::getSurfacePropertyDisplayName(
          isosurface::defaultPropertyForKind(params.kind)));
    }

    mesh->setParent(m_structure);
    // create the child instance that will be shown
    MeshInstance *instance = new MeshInstance(mesh);
    instance->setObjectName("+ {x,y,z} [0,0,0]");
    idx++;
  }
}

} // namespace volume
