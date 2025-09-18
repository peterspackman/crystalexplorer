#include "scene_export_data.h"
#include <ankerl/unordered_dense.h>

namespace cx::graphics {

void SceneExportData::clear() {
  m_spheres.clear();
  m_cylinders.clear();
  m_meshes.clear();
}

std::vector<QString> SceneExportData::getGroups() const {
  ankerl::unordered_dense::set<QString> groups;

  for (const auto &sphere : m_spheres) {
    groups.insert(sphere.group);
  }
  for (const auto &cylinder : m_cylinders) {
    groups.insert(cylinder.group);
  }
  for (const auto &mesh : m_meshes) {
    groups.insert(mesh.group);
  }

  return std::vector<QString>(groups.begin(), groups.end());
}

std::vector<ExportSphere>
SceneExportData::getSpheresInGroup(const QString &group) const {
  std::vector<ExportSphere> result;
  for (const auto &sphere : m_spheres) {
    if (sphere.group == group) {
      result.push_back(sphere);
    }
  }
  return result;
}

std::vector<ExportCylinder>
SceneExportData::getCylindersInGroup(const QString &group) const {
  std::vector<ExportCylinder> result;
  for (const auto &cylinder : m_cylinders) {
    if (cylinder.group == group) {
      result.push_back(cylinder);
    }
  }
  return result;
}

std::vector<ExportMesh>
SceneExportData::getMeshesInGroup(const QString &group) const {
  std::vector<ExportMesh> result;
  for (const auto &mesh : m_meshes) {
    if (mesh.group == group) {
      result.push_back(mesh);
    }
  }
  return result;
}

} // namespace cx::graphics