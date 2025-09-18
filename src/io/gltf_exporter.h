#pragma once

#include <QColor>
#include <QString>
#include <QVector3D>
#include <ankerl/unordered_dense.h>
#include <memory>
#include <vector>

class Scene;
class ChemicalStructure;
class Mesh;
class MeshInstance;

namespace cx::graphics {
struct ExportSphere;
struct ExportCylinder;
struct ExportMesh;
struct ExportCamera;
} // namespace cx::graphics

namespace fastgltf {
class Asset;
}

namespace cx::core {

/**
 * @brief GLTF export using fastgltf library
 *
 * Supports exporting:
 * - Atoms as spheres with element-based colors and sizes
 * - Bonds as cylinders
 * - Mesh surfaces with colors and transparency
 * - Framework tubes and structures
 * - Materials and lighting information
 */
class GLTFExporter {
public:
  struct ExportOptions {
    bool exportAtoms = true;
    bool exportBonds = true;
    bool exportMeshes = true;
    bool exportFramework = true;

    // Quality settings
    int sphereSubdivisions = 2; // Icosphere subdivision level
    int cylinderSegments = 12;  // Radial segments for cylinders

    // Size scaling
    float atomRadiusScale = 1.0f;
    float bondRadiusScale = 1.0f;

    // Output options
    bool binaryFormat = false; // Export as .glb instead of .gltf
    bool prettyPrint = true;
  };

  GLTFExporter();
  ~GLTFExporter() = default;

  /**
   * @brief Export a Scene to GLTF format
   */
  bool exportScene(const Scene *scene, const QString &filePath,
                   const ExportOptions &options);
  bool exportScene(const Scene *scene, const QString &filePath);

  /**
   * @brief Export a ChemicalStructure to GLTF format
   */
  bool exportStructure(const ChemicalStructure *structure,
                       const QString &filePath, const ExportOptions &options);
  bool exportStructure(const ChemicalStructure *structure,
                       const QString &filePath);
  bool exportStructure(const ChemicalStructure *structure,
                       const QString &filePath, const ExportOptions &options,
                       const Scene *scene);

private:
  void loadIcosphereMesh();
  void loadCylinderMesh();
  void addAtomsToAsset(
      fastgltf::Asset &asset,
      const ankerl::unordered_dense::map<
          int, std::vector<std::pair<QVector3D, float>>> &atomsByElement,
      const ExportOptions &options);
  void addBondsToAsset(fastgltf::Asset &asset,
                       const ChemicalStructure *structure,
                       const ExportOptions &options);
  void addMeshesToAsset(fastgltf::Asset &asset,
                        const ChemicalStructure *structure,
                        const ExportOptions &options);
  void addFrameworkToAsset(fastgltf::Asset &asset,
                           const ChemicalStructure *structure,
                           const ExportOptions &options);
  void addStructureByFragments(fastgltf::Asset &asset,
                               const ChemicalStructure *structure,
                               const ExportOptions &options,
                               const Scene *scene);
  float getAtomDisplayRadius(const ChemicalStructure *structure, int atomIndex,
                             const Scene *scene, const ExportOptions &options);

  // SceneExportData-based methods
  void addSpheresToAsset(fastgltf::Asset &asset,
                         const std::vector<cx::graphics::ExportSphere> &spheres,
                         const ExportOptions &options);
  void addCylindersToAsset(
      fastgltf::Asset &asset,
      const std::vector<cx::graphics::ExportCylinder> &cylinders,
      const ExportOptions &options);
  void addMeshesToAsset(fastgltf::Asset &asset,
                        const std::vector<cx::graphics::ExportMesh> &meshes,
                        const ExportOptions &options);
  void addCameraToAsset(fastgltf::Asset &asset,
                        const cx::graphics::ExportCamera &camera);

  std::vector<float> m_sphereVertices;
  std::vector<uint32_t> m_sphereIndices;
  std::vector<float> m_cylinderVertices;
  std::vector<uint32_t> m_cylinderIndices;

  // Material properties from settings
  float m_materialRoughness = 0.5f;
  float m_materialMetallic = 0.0f;
};

} // namespace cx::core
