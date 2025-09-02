#include "mesh.h"
#include "eigen_json.h"
#include "isosurface_parameters.h"
#include "json.h"
#include "meshinstance.h"
#include <QFile>
#include <QSignalBlocker>
#include <fmt/os.h>

using VertexList = Mesh::VertexList;
using FaceList = Mesh::FaceList;
using ScalarPropertyValues = Mesh::ScalarPropertyValues;
using ScalarProperties = Mesh::ScalarProperties;

void to_json(nlohmann::json &j, const Mesh::Attributes &attr) {
  j = {
      {"kind", attr.kind},
      {"isovalue", attr.isovalue},
      {"separation", attr.separation},
  };
}

void from_json(const nlohmann::json &j, Mesh::Attributes &attr) {
  j.at("kind").get_to(attr.kind);
  j.at("isovalue").get_to(attr.isovalue);
  j.at("separation").get_to(attr.separation);
}

Mesh::Mesh(QObject *parent) : QObject(parent) {}

Mesh::Mesh(Eigen::Ref<const VertexList> vertices,
           Eigen::Ref<const FaceList> faces, QObject *parent)
    : QObject(parent), m_vertices(vertices), m_faces(faces) {
  m_centroid = m_vertices.rowwise().mean();
  updateVertexFaceMapping();
  updateFaceProperties();
  m_vertexAreas = computeVertexAreas();
  m_vertexMask = Eigen::Matrix<bool, Eigen::Dynamic, 1>(m_vertices.cols());
  resetVertexMask(true);
}

Mesh::Mesh(Eigen::Ref<const VertexList> vertices, QObject *parent)
    : QObject(parent), m_vertices(vertices) {
  m_vertexMask = Eigen::Matrix<bool, Eigen::Dynamic, 1>(m_vertices.cols());
  resetVertexMask(true);
}

void Mesh::updateVertexFaceMapping() {
  m_facesUsingVertex = std::vector<std::vector<int>>(numberOfVertices());
  for (int f = 0; f < numberOfFaces(); f++) {
    for (int i = 0; i < 3; i++) {
      m_facesUsingVertex[m_faces(i, f)].push_back(f);
    }
  }
}

VertexList Mesh::computeFaceNormals() const {
  VertexList normals(3, m_faces.cols());
  for (int i = 0; i < m_faces.cols(); ++i) {
    Eigen::Vector3d v0 = m_vertices.col(m_faces(0, i));
    Eigen::Vector3d v1 = m_vertices.col(m_faces(1, i));
    Eigen::Vector3d v2 = m_vertices.col(m_faces(2, i));
    Eigen::Vector3d edge1 = v1 - v0;
    Eigen::Vector3d edge2 = v2 - v0;
    Eigen::Vector3d normal = edge1.cross(edge2).normalized();

    normals.col(i) = normal;
  }
  return normals;
}

void Mesh::setVertexNormals(Eigen::Ref<const VertexList> normals) {
  m_vertexNormals = normals;
}

VertexList Mesh::computeVertexNormals(Mesh::NormalSetting setting) const {
  const VertexList faceNormals = computeFaceNormals();
  VertexList normals = VertexList::Zero(3, m_facesUsingVertex.size());
  for (size_t i = 0; i < m_facesUsingVertex.size(); ++i) {
    Eigen::Vector3d normal = Eigen::Vector3d::Zero();
    for (int faceIndex : m_facesUsingVertex[i]) {
      normal.array() += faceNormals.col(faceIndex).array();
      if (setting == Mesh::NormalSetting::Flat)
        break;
    }
    normals.col(i) = normal.normalized();
  }
  return normals;
}

// Description related stuff
QString Mesh::description() const { return m_description; }

void Mesh::setDescription(const QString &description) {
  m_description = description;
}

void Mesh::updateFaceProperties() {

  const int N = numberOfFaces();
  m_faceAreas.resize(N);
  m_faceVolumeContributions.resize(N);
  m_faceNormals.resize(3, N);
  m_faceMask = Eigen::Matrix<bool, Eigen::Dynamic, 1>(N);
  resetFaceMask(true);

  for (int i = 0; i < N; i++) {
    Eigen::Vector3d v0 = m_vertices.col(m_faces(0, i));
    Eigen::Vector3d v1 = m_vertices.col(m_faces(1, i));
    Eigen::Vector3d v2 = m_vertices.col(m_faces(2, i));

    Eigen::Vector3d edge1 = v0 - v1;
    Eigen::Vector3d edge2 = v1 - v2;

    Eigen::Vector3d normal = edge1.cross(edge2);
    double norm = normal.norm();
    m_faceAreas(i) = 0.5 * norm;
    m_faceNormals.col(i) = normal / norm;
    m_faceVolumeContributions(i) =
        m_faceAreas(i) * m_faceNormals.col(i).dot(v0) / 3.0;
  }
  updateAsphericity();
  m_volume = m_faceVolumeContributions.sum();
  m_surfaceArea = m_faceAreas.sum();
  m_globularity = 0.0;
  if (m_volume > 0.0 && m_surfaceArea > 0.0) {
    const double fac = std::cbrt(36.0 * M_PI);
    m_globularity = fac * std::cbrt(m_volume * m_volume) / m_surfaceArea;
  }
}

void Mesh::updateAsphericity() {
  const int N = numberOfVertices();

  Eigen::Vector3d centroid = m_vertices.rowwise().mean();

  double xx, xy, xz, yy, yz, zz;
  xx = xy = xz = yy = yz = zz = 0.0;
  for (int i = 0; i < N; i++) {
    double dx = m_vertices(0, i) - centroid(0);
    double dy = m_vertices(1, i) - centroid(1);
    double dz = m_vertices(2, i) - centroid(2);
    xx += dx * dx;
    xy += dx * dy;
    xz += dx * dz;
    yy += dy * dy;
    yz += dy * dz;
    zz += dz * dz;
  }

  // Create matrix m, calculate eigenvalues and store them in e
  Eigen::Matrix3d m;
  m << xx, xy, xz, xy, yy, yz, xz, yz, zz;
  Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> solver(m, false);
  Eigen::Vector3d e = solver.eigenvalues();

  double first_term = 0.0;
  double second_term = 0.0;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (i == j) {
        continue;
      }
      first_term += (e(i) - e(j)) * (e(i) - e(j));
    }
    second_term += e(i);
  }
  m_asphericity = (0.25 * first_term) / (second_term * second_term);
}

double Mesh::surfaceArea() const { return m_surfaceArea; }

double Mesh::volume() const { return m_volume; }

double Mesh::globularity() const { return m_globularity; }

// Vertex Properties
const ScalarProperties &Mesh::vertexProperties() const {
  return m_vertexProperties;
}

void Mesh::setVertexProperty(const QString &name,
                             const ScalarPropertyValues &values) {
  m_vertexProperties[name] = values;

  // update to default property range
  Mesh::ScalarPropertyRange range;
  range.lower = values.minCoeff();
  range.upper = values.maxCoeff();

  // only send a signal once
  {
    QSignalBlocker blocker(this);
    setVertexPropertyRange(name, range);
  }

  setSelectedProperty(name);
}

const ScalarPropertyValues &Mesh::vertexProperty(const QString &name) const {
  const auto loc = m_vertexProperties.find(name);
  if (loc != m_vertexProperties.end()) {
    return loc->second;
  }
  qDebug() << "Empty property" << name;
  return m_emptyProperty;
}

ScalarPropertyValues Mesh::averagedFaceProperty(const QString &name) const {
  const auto &prop = vertexProperty(name);
  if (prop.size() == 0)
    return prop;

  ScalarPropertyValues result(m_faces.cols());

  for (int f = 0; f < m_faces.cols(); f++) {
    result(f) =
        (prop(m_faces(0, f)) + prop(m_faces(1, f)) + prop(m_faces(2, f))) / 3.0;
  }
  return result;
}

void Mesh::setVertexPropertyRange(const QString &name,
                                  Mesh::ScalarPropertyRange range) {
  m_vertexPropertyRanges[name] = range;
  emit selectedPropertyChanged();
}

Mesh::ScalarPropertyRange Mesh::vertexPropertyRange(const QString &name) const {
  const auto loc = m_vertexPropertyRanges.find(name);
  if (loc != m_vertexPropertyRanges.end()) {
    return loc->second;
  }
  return {};
}

QStringList Mesh::availableVertexProperties() const {
  QStringList result;
  for (const auto &prop : m_vertexProperties) {
    result.append(prop.first);
  }
  return result;
}

bool Mesh::haveVertexProperty(const QString &prop) const {
  return m_vertexProperties.find(prop) != m_vertexProperties.end();
}

// Face properties
const ScalarProperties &Mesh::faceProperties() const {
  return m_faceProperties;
}

void Mesh::setFaceProperty(const QString &name,
                           const ScalarPropertyValues &values) {
  m_faceProperties[name] = values;
}

const ScalarPropertyValues &Mesh::faceProperty(const QString &name) const {
  const auto loc = m_faceProperties.find(name);
  if (loc != m_faceProperties.end()) {
    return loc->second;
  }
  return m_emptyProperty;
}

QStringList Mesh::availableFaceProperties() const {
  QStringList result;
  for (const auto &prop : m_faceProperties) {
    result.append(prop.first);
  }
  return result;
}

bool Mesh::haveFaceProperty(const QString &prop) const {
  return m_faceProperties.find(prop) != m_faceProperties.end();
}

bool Mesh::isTransparent() const { return m_transparent; }
float Mesh::getTransparency() const { return m_transparency; }

void Mesh::setTransparent(bool transparent) {
  if (transparent == m_transparent)
    return;
  m_transparent = transparent;
  for (auto *child : children()) {
    auto *instance = qobject_cast<MeshInstance *>(child);
    if (instance)
      instance->setTransparent(m_transparent);
  }

  emit transparencyChanged();
}

void Mesh::setTransparency(float transparency) {
  if (transparency == m_transparency)
    return;
  m_transparency = transparency;
  for (auto *child : children()) {
    auto *instance = qobject_cast<MeshInstance *>(child);
    if (instance)
      instance->setTransparency(m_transparency);
  }

  emit transparencyChanged();
}
bool Mesh::isVisible() const { return m_visible; }

void Mesh::setVisible(bool visible) {
  if (m_visible == visible)
    return;
  m_visible = visible;
  {
    for (auto *child : children()) {
      auto *instance = qobject_cast<MeshInstance *>(child);
      if (instance)
        instance->setVisible(m_visible);
    }
  }
  emit visibilityChanged();
}

const QString &Mesh::getSelectedProperty() const { return m_selectedProperty; }

bool Mesh::setSelectedProperty(const QString &propName) {
  if (m_selectedProperty == propName)
    return true;
  if (m_vertexProperties.find(propName) == m_vertexProperties.end())
    return false;

  m_selectedProperty = propName;
  {
    for (auto *child : children()) {
      auto *instance = qobject_cast<MeshInstance *>(child);
      if (instance)
        instance->setSelectedProperty(m_selectedProperty);
    }
  }
  emit selectedPropertyChanged();
  return true;
}

Mesh *Mesh::newFromJsonFile(const QString &filename, QObject *parent) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning("Failed to open the JSON file.");
    return nullptr;
  }

  nlohmann::json doc;
  try {
    doc = nlohmann::json::parse(file.readAll().constData());
  } catch (nlohmann::json::parse_error &e) {
    qWarning() << "JSON parse error:" << e.what();
    return nullptr;
  }
  file.close();

  return Mesh::newFromJson(doc, parent);
}

Mesh *Mesh::newFromJson(const nlohmann::json &object, QObject *parent) {
  Mesh *pointCloud = new Mesh(parent);
  // Parse points
  const auto pointsArray = object.at("points");
  pointsArray.get_to(pointCloud->m_vertices);

  // Parse properties
  const auto propertiesObj = object["properties"];
  for (const auto &item : propertiesObj.items()) {
    const auto propertyValuesArray = item.value();
    ScalarPropertyValues propertyValues(propertyValuesArray.size());
    propertyValuesArray.get_to(propertyValues);
    pointCloud->setVertexProperty(QString::fromStdString(item.key()),
                                  propertyValues);
  }

  return pointCloud;
}

size_t Mesh::rendererIndex() const { return m_rendererIndex; }

void Mesh::setRendererIndex(size_t idx) { m_rendererIndex = idx; }

void Mesh::setAttributes(Mesh::Attributes attr) {
  m_attr = attr;

  if (m_vertices.size() > 0) {
    ScalarPropertyValues isovalues(numberOfVertices());
    isovalues.setConstant(attr.isovalue);
    setVertexProperty("Isovalue", isovalues);
  }
}

void Mesh::setAtomsInside(const std::vector<GenericAtomIndex> &idxs) {
  m_atomsInside = idxs;
}

const std::vector<GenericAtomIndex> &Mesh::atomsInside() const {
  return m_atomsInside;
}

void Mesh::setAtomsOutside(const std::vector<GenericAtomIndex> &idxs) {
  m_atomsOutside = idxs;
}

const std::vector<GenericAtomIndex> &Mesh::atomsOutside() const {
  return m_atomsOutside;
}

bool Mesh::containsPoint(const occ::Vec3& point) const {
  // Multi-ray consensus algorithm for robust point-in-mesh testing
  // Works for arbitrary closed meshes (convex and non-convex)
  
  // Use carefully chosen ray directions to avoid edge cases
  // Based on golden ratio and other irrational numbers to minimize 
  // chance of hitting edges exactly
  const double phi = (1.0 + std::sqrt(5.0)) / 2.0; // Golden ratio
  const double sqrt2 = std::sqrt(2.0);
  const double sqrt3 = std::sqrt(3.0);
  
  std::vector<occ::Vec3> rayDirections = {
    occ::Vec3(1.0, phi, 1.0/phi).normalized(),           // Golden ratio based
    occ::Vec3(-phi, 1.0, 1.0/phi).normalized(),          // Golden ratio based  
    occ::Vec3(1.0/phi, -1.0, phi).normalized(),          // Golden ratio based
    occ::Vec3(sqrt2, sqrt3, 1.0).normalized(),           // Irrational direction
    occ::Vec3(-1.0, sqrt2, sqrt3).normalized(),          // Irrational direction
    occ::Vec3(sqrt3, -sqrt2, 1.0).normalized(),          // Irrational direction
    occ::Vec3(1.0, 1.0, 1.0).normalized()                // Simple diagonal
  };
  
  int insideVotes = 0;
  const double epsilon = 1e-12; // Very tight epsilon for robustness
  
  for (const auto& rayDir : rayDirections) {
    int intersectionCount = 0;
    
    for (int f = 0; f < m_faces.cols(); ++f) {
      // Get triangle vertices
      occ::Vec3 v0 = m_vertices.col(m_faces(0, f));
      occ::Vec3 v1 = m_vertices.col(m_faces(1, f));
      occ::Vec3 v2 = m_vertices.col(m_faces(2, f));
      
      // Möller-Trumbore ray-triangle intersection algorithm
      occ::Vec3 edge1 = v1 - v0;
      occ::Vec3 edge2 = v2 - v0;
      occ::Vec3 h = rayDir.cross(edge2);
      double a = edge1.dot(h);
      
      // Skip if ray is parallel to triangle (within epsilon)
      if (std::abs(a) < epsilon) {
        continue;
      }
      
      double f_inv = 1.0 / a;
      occ::Vec3 s = point - v0;
      double u = f_inv * s.dot(h);
      
      // Check if intersection is within triangle bounds
      if (u < 0.0 || u > 1.0) {
        continue;
      }
      
      occ::Vec3 q = s.cross(edge1);
      double v = f_inv * rayDir.dot(q);
      
      if (v < 0.0 || u + v > 1.0) {
        continue;
      }
      
      // Compute t to find intersection point on ray
      double t = f_inv * edge2.dot(q);
      
      // Only count intersections in positive ray direction
      // Use small epsilon but be more inclusive for boundary points
      if (t > epsilon) {
        intersectionCount++;
      }
    }
    
    // Odd number of intersections means point is inside for this ray
    if ((intersectionCount % 2) == 1) {
      insideVotes++;
    }
  }
  
  // Use majority vote with tie-breaking favoring inside
  const int totalRays = rayDirections.size();
  const int majorityThreshold = totalRays / 2;
  
  if (insideVotes > majorityThreshold) {
    return true;  // Clear majority says inside
  } else if (insideVotes < majorityThreshold) {
    return false; // Clear majority says outside  
  } else {
    // Tie case - favor inside for boundary points
    // This handles points exactly on edges/faces more inclusively
    return true;
  }
}

std::pair<occ::Vec3, occ::Vec3> Mesh::boundingBox() const {
  if (m_vertices.cols() == 0) {
    return {occ::Vec3::Zero(), occ::Vec3::Zero()};
  }
  
  occ::Vec3 min = m_vertices.col(0);
  occ::Vec3 max = m_vertices.col(0);
  
  for (int i = 1; i < m_vertices.cols(); ++i) {
    const occ::Vec3& v = m_vertices.col(i);
    min = min.cwiseMin(v);
    max = max.cwiseMax(v);
  }
  
  return {min, max};
}

std::vector<GenericAtomIndex> Mesh::findAtomsInside(const ChemicalStructure* structure) const {
  if (!structure) {
    return {};
  }
  
  // Use bounding box to get candidate atoms first (much more efficient for crystals)
  auto [minBox, maxBox] = boundingBox();
  auto candidateAtoms = structure->atomsInBoundingBox(minBox, maxBox);
  
  if (candidateAtoms.empty()) {
    return {};
  }
  
  // Get actual positions for all candidate atoms (handles periodic images correctly)
  auto positions = structure->atomicPositionsForIndices(candidateAtoms);
  
  std::vector<GenericAtomIndex> result;
  
  for (int i = 0; i < candidateAtoms.size(); ++i) {
    const auto& atomIndex = candidateAtoms[i];
    const occ::Vec3& atomPos = positions.col(i);
    
    if (containsPoint(atomPos)) {
      result.push_back(atomIndex);
    }
  }
  
  return result;
}

bool Mesh::haveChildMatchingTransform(
    const Eigen::Isometry3d &transform) const {
  // TODO do this in bulk

  auto check = [](occ::Vec3 a, occ::Vec3 b, double tolerance = 1e-1) {
    return (a - b).norm() < tolerance;
  };

  auto candidateCentroid =
      transform.rotation() * m_centroid + transform.translation();

  for (auto *child : children()) {
    auto *instance = qobject_cast<MeshInstance *>(child);
    if (!instance)
      continue;
    if (check(instance->centroid(), candidateCentroid))
      return true;
  }
  return false;
}

void Mesh::resetFaceMask(bool value) { m_faceMask.array() = value; }

void Mesh::resetVertexMask(bool value) { m_vertexMask.array() = value; }

ScalarPropertyValues Mesh::computeVertexAreas() const {
  ScalarPropertyValues vertexAreas =
      ScalarPropertyValues::Zero(m_vertices.cols());

  for (int i = 0; i < m_faces.cols(); ++i) {
    float v = m_faceAreas(i) / 3.0f;
    vertexAreas(m_faces(0, i)) += v;
    vertexAreas(m_faces(1, i)) += v;
    vertexAreas(m_faces(2, i)) += v;
  }
  return vertexAreas;
}

Mesh *Mesh::combine(const QList<Mesh *> &meshes) {
  if (meshes.isEmpty()) {
    return nullptr;
  }

  // Validate meshes are compatible
  const auto &first = meshes.first();
  for (const auto *mesh : meshes) {
    if (!mesh)
      continue;

    if (mesh->kind() != first->kind()) {
      qWarning() << "Cannot combine meshes of different kinds";
      return nullptr;
    }

    if (mesh->atomsInside() != first->atomsInside() ||
        mesh->atomsOutside() != first->atomsOutside()) {
      qWarning() << "Cannot combine meshes with different atom configurations";
      return nullptr;
    }
  }

  int totalVertices = 0;
  int totalFaces = 0;
  for (const auto *mesh : meshes) {
    if (!mesh)
      continue;
    totalVertices += mesh->numberOfVertices();
    totalFaces += mesh->numberOfFaces();
  }

  // Prepare combined vertex and face matrices
  VertexList combinedVertices(3, totalVertices);
  FaceList combinedFaces(3, totalFaces);
  ScalarPropertyValues isovalues(totalVertices);

  // Copy data with offset tracking
  int vertexOffset = 0;
  int faceOffset = 0;

  for (const auto *mesh : meshes) {
    if (!mesh)
      continue;

    const int nVerts = mesh->numberOfVertices();
    const int nFaces = mesh->numberOfFaces();

    // Copy vertices
    combinedVertices.block(0, vertexOffset, 3, nVerts) = mesh->vertices();

    // Copy faces with offset
    auto faces = mesh->faces();
    faces.array() += vertexOffset;
    combinedFaces.block(0, faceOffset, 3, nFaces) = faces;

    // Set isovalues for this mesh's vertices
    isovalues.segment(vertexOffset, nVerts).array() = mesh->isovalue();

    vertexOffset += nVerts;
    faceOffset += nFaces;
  }

  // Create new mesh
  auto *combinedMesh = new Mesh(combinedVertices, combinedFaces);

  // Copy parameters from first mesh
  combinedMesh->setAttributes(first->attributes());

  // Set the isovalue property
  combinedMesh->setVertexProperty("Isovalue", isovalues);

  // Copy description
  combinedMesh->setDescription(
      QString("Combined mesh from %1 isosurfaces").arg(meshes.size()));

  return combinedMesh;
}

void to_json(nlohmann::json &j, const Mesh::ScalarPropertyRange &range) {
  j = {
      {"lower", range.lower}, {"upper", range.upper}, {"middle", range.middle}};
}

void from_json(const nlohmann::json &j, Mesh::ScalarPropertyRange &range) {
  j.at("lower").get_to(range.lower);
  j.at("upper").get_to(range.upper);
  j.at("middle").get_to(range.middle);
}

void to_json(nlohmann::json &j, const Mesh::ScalarProperties &props) {
  j = nlohmann::json::object();
  for (const auto &[key, values] : props) {
    j[key.toStdString()] = values;
  }
}

void from_json(const nlohmann::json &j, Mesh::ScalarProperties &props) {
  props.clear();
  for (const auto &[key, values] : j.items()) {
    props[QString::fromStdString(key)] =
        values.get<Mesh::ScalarPropertyValues>();
  }
}

void to_json(nlohmann::json &j, const Mesh::ScalarPropertyRanges &ranges) {
  j = nlohmann::json::object();
  for (const auto &[key, range] : ranges) {
    j[key.toStdString()] = range;
  }
}

void from_json(const nlohmann::json &j, Mesh::ScalarPropertyRanges &ranges) {
  ranges.clear();
  for (const auto &[key, range] : j.items()) {
    ranges[QString::fromStdString(key)] =
        range.get<Mesh::ScalarPropertyRange>();
  }
}

nlohmann::json Mesh::toJson() const {
  nlohmann::json j = {{"vertices", m_vertices},
                      {"faces", m_faces},
                      {"description", m_description.toStdString()},
                      {"visible", m_visible},
                      {"transparent", m_transparent},
                      {"transparency", m_transparency},
                      {"rendererIndex", m_rendererIndex}};

  j["name"] = objectName();
  // Existing properties
  if (!m_vertexProperties.empty()) {
    j["vertexProperties"] = m_vertexProperties;
    j["vertexPropertyRanges"] = m_vertexPropertyRanges;
  }
  if (!m_faceProperties.empty()) {
    j["faceProperties"] = m_faceProperties;
  }

  // Normals
  if (m_vertexNormals.cols() > 0) {
    j["vertexNormals"] = m_vertexNormals;
  }
  if (m_faceNormals.cols() > 0) {
    j["faceNormals"] = m_faceNormals;
  }

  // Areas and other calculated properties
  if (m_faceAreas.size() > 0)
    j["faceAreas"] = m_faceAreas;
  if (m_vertexAreas.size() > 0)
    j["vertexAreas"] = m_vertexAreas;
  if (m_faceVolumeContributions.size() > 0)
    j["faceVolumeContributions"] = m_faceVolumeContributions;

  // Masks
  if (m_faceMask.size() > 0)
    j["faceMask"] = m_faceMask;
  if (m_vertexMask.size() > 0)
    j["vertexMask"] = m_vertexMask;
  if (!m_vertexHighlights.empty()) {
    std::vector<int> highlights(m_vertexHighlights.begin(),
                                m_vertexHighlights.end());
    j["vertexHighlights"] = highlights;
  }

  // Parameters and atoms
  if (!m_atomsInside.empty())
    j["atomsInside"] = m_atomsInside;
  if (!m_atomsOutside.empty())
    j["atomsOutside"] = m_atomsOutside;

  j["attributes"] = m_attr;

  // Selected property
  if (!m_selectedProperty.isEmpty()) {
    j["selectedProperty"] = m_selectedProperty.toStdString();
  }

  // Instance transforms
  j["instances"] = nlohmann::json::array();
  for (const auto *child : children()) {
    if (const auto *instance = qobject_cast<const MeshInstance *>(child)) {
      nlohmann::json instanceJson;
      instanceJson["name"] = instance->objectName().toStdString();
      instanceJson["transform"] = instance->transform().matrix();
      j["instances"].push_back(instanceJson);
    }
  }

  return j;
}

bool Mesh::fromJson(const nlohmann::json &j) {
  try {
    if (j.contains("name")) {
      setObjectName(j.at("name").get<QString>());
    }
    // Core mesh data
    j.at("vertices").get_to(m_vertices);
    j.at("faces").get_to(m_faces);
    m_description = j.at("description").get<QString>();
    m_visible = j.at("visible").get<bool>();
    m_transparent = j.at("transparent").get<bool>();
    m_transparency = j.at("transparency").get<float>();
    m_rendererIndex = j.at("rendererIndex").get<size_t>();

    // Properties
    if (j.contains("vertexProperties")) {
      j.at("vertexProperties").get_to(m_vertexProperties);
      j.at("vertexPropertyRanges").get_to(m_vertexPropertyRanges);
    }
    if (j.contains("faceProperties")) {
      j.at("faceProperties").get_to(m_faceProperties);
    }

    // Normals
    if (j.contains("vertexNormals"))
      j.at("vertexNormals").get_to(m_vertexNormals);
    if (j.contains("faceNormals"))
      j.at("faceNormals").get_to(m_faceNormals);

    // Areas and volumes
    if (j.contains("faceAreas"))
      j.at("faceAreas").get_to(m_faceAreas);
    if (j.contains("vertexAreas"))
      j.at("vertexAreas").get_to(m_vertexAreas);
    if (j.contains("faceVolumeContributions"))
      j.at("faceVolumeContributions").get_to(m_faceVolumeContributions);

    // Masks and highlights
    if (j.contains("faceMask"))
      j.at("faceMask").get_to(m_faceMask);
    if (j.contains("vertexMask"))
      j.at("vertexMask").get_to(m_vertexMask);
    if (j.contains("vertexHighlights")) {
      std::vector<int> highlights;
      j.at("vertexHighlights").get_to(highlights);
      m_vertexHighlights.clear();
      m_vertexHighlights.insert(highlights.begin(), highlights.end());
    }

    // Parameters and atoms
    if (j.contains("atomsInside"))
      j.at("atomsInside").get_to(m_atomsInside);
    if (j.contains("atomsOutside"))
      j.at("atomsOutside").get_to(m_atomsOutside);

    j.at("attributes").get_to(m_attr);

    if (j.contains("selectedProperty")) {
      m_selectedProperty =
          QString::fromStdString(j.at("selectedProperty").get<std::string>());
    }

    updateVertexFaceMapping();
    updateFaceProperties();
    updateAsphericity();

    return true;
  } catch (const std::exception &e) {
    qDebug() << "Mesh loading failed:" << e.what();
    return false;
  }
}
