#pragma once

#include <QColor>
#include <QString>
#include <QVector3D>
#include <vector>

namespace cx::graphics {

struct ExportSphere {
  QVector3D position;
  float radius;
  QColor color;
  QString name;
  QString group; // e.g., "Atoms/Carbon", "Atoms/Hydrogen"
};

struct ExportCylinder {
  QVector3D startPosition;
  QVector3D endPosition;
  float radius;
  QColor color;
  QString name;
  QString group; // e.g., "Bonds", "Framework"
};

struct ExportMesh {
  std::vector<float> vertices; // x,y,z,x,y,z...
  std::vector<float> normals;  // nx,ny,nz,nx,ny,nz...
  std::vector<float> colors;   // r,g,b,r,g,b... per vertex (0-1 range)
  std::vector<uint32_t> indices;
  QColor fallbackColor; // Used if no vertex colors
  float opacity;
  QString name;
  QString group; // e.g., "Surfaces"
};

struct ExportCamera {
  enum Type { Perspective, Orthographic };

  Type type = Orthographic; // CrystalExplorer typically uses orthographic
  QString name = "Camera";

  // Camera transform (position and orientation)
  QVector3D position;
  QVector3D target; // What the camera is looking at
  QVector3D up;     // Up vector

  // Perspective camera parameters (GLTF spec)
  struct {
    float aspectRatio = 1.0f; // width/height
    float yfov = 0.785398f;   // Y field of view in radians (45 degrees)
    float znear = 0.01f;
    float zfar = 1000.0f; // Optional, infinite if not set
  } perspective;

  // Orthographic camera parameters (GLTF spec)
  struct {
    float xmag = 10.0f; // Half the orthographic width
    float ymag = 10.0f; // Half the orthographic height
    float znear = 0.01f;
    float zfar = 1000.0f;
  } orthographic;
};

/**
 * @brief Simple data container for export-ready primitive data
 *
 * Contains spheres, cylinders, and meshes with their transforms, colors, etc.
 * Populated by Scene::getExportData() to match exactly what's displayed.
 */
class SceneExportData {
public:
  SceneExportData() = default;

  // Data accessors
  const std::vector<ExportSphere> &spheres() const { return m_spheres; }
  const std::vector<ExportCylinder> &cylinders() const { return m_cylinders; }
  const std::vector<ExportMesh> &meshes() const { return m_meshes; }
  const ExportCamera &camera() const { return m_camera; }

  // Data setters (for Scene to populate)
  std::vector<ExportSphere> &spheres() { return m_spheres; }
  std::vector<ExportCylinder> &cylinders() { return m_cylinders; }
  std::vector<ExportMesh> &meshes() { return m_meshes; }
  ExportCamera &camera() { return m_camera; }

  // Grouped access for hierarchical exports
  std::vector<QString> getGroups() const;
  std::vector<ExportSphere> getSpheresInGroup(const QString &group) const;
  std::vector<ExportCylinder> getCylindersInGroup(const QString &group) const;
  std::vector<ExportMesh> getMeshesInGroup(const QString &group) const;

  // Utility
  void clear();

private:
  std::vector<ExportSphere> m_spheres;
  std::vector<ExportCylinder> m_cylinders;
  std::vector<ExportMesh> m_meshes;
  ExportCamera m_camera;
};

} // namespace cx::graphics