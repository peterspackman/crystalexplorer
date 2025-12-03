#include "meshinstance.h"
#include "chemicalstructure.h"
#include <fmt/core.h>

MeshInstance::MeshInstance(Mesh *parent, const MeshTransform &transform)
    : QObject(parent), m_mesh(parent), m_transform(transform) {
  if (m_mesh) {
    populateSurroundingAtoms();
    m_selectedProperty = m_mesh->getSelectedProperty();

    // Forward instance property changes to parent mesh for backward compatibility
    // These connections maintain compatibility with code that listens to mesh-level signals
    // rather than instance-level signals. This design allows:
    //   1. Legacy code to work without modification (listens to Mesh signals)
    //   2. New code to listen directly to specific instances (more precise)
    //
    // ARCHITECTURAL NOTE: In an ideal design, listeners would connect directly to
    // MeshInstance signals rather than relying on these forwarded Mesh signals.
    // This would eliminate the need for these connections and make the signal flow
    // more explicit. Consider refactoring when updating mesh-related rendering code.
    //
    // Current behavior: When ANY instance changes a property, ALL listeners on the
    // parent mesh are notified, even if they only care about specific instances.
    connect(this, &MeshInstance::visibilityChanged, m_mesh,
            &Mesh::visibilityChanged);
    connect(this, &MeshInstance::transparencyChanged, m_mesh,
            &Mesh::transparencyChanged);
    connect(this, &MeshInstance::selectedPropertyChanged, m_mesh,
            &Mesh::selectedPropertyChanged);
  }
}

const Mesh *MeshInstance::mesh() const { return m_mesh; }
Mesh *MeshInstance::mesh() { return m_mesh; }

Eigen::Vector3d MeshInstance::vertex(int index) const {
  if (!m_mesh)
    return {};
  return (m_transform.rotation() * m_mesh->vertex(index)) +
         m_transform.translation();
}

Eigen::Vector3d MeshInstance::centroid() const {
  if (!m_mesh)
    return {};
  return (m_transform.rotation() * m_mesh->centroid()) +
         m_transform.translation();
}

QVector3D MeshInstance::vertexVector3D(int index) const {
  auto v = vertex(index);
  return QVector3D(v.x(), v.y(), v.z());
}

Mesh::VertexList MeshInstance::vertices() const {
  if (!m_mesh)
    return {};
  return (m_transform.rotation() * m_mesh->vertices()).colwise() +
         m_transform.translation();
}

Eigen::Vector3d MeshInstance::vertexNormal(int index) const {
  if (!m_mesh)
    return {};
  return (m_transform.rotation() * m_mesh->vertexNormal(index));
}

QVector3D MeshInstance::vertexNormalVector3D(int index) const {
  auto v = vertexNormal(index);
  return QVector3D(v.x(), v.y(), v.z());
}

Mesh::VertexList MeshInstance::vertexNormals() const {
  if (!m_mesh)
    return {};
  return m_transform.rotation() * m_mesh->vertexNormals();
}

const MeshTransform &MeshInstance::transform() const { return m_transform; }

QMatrix3x3 MeshInstance::rotationMatrix() const {
  auto r = m_transform.rotation();
  QMatrix3x3 result;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      result(i, j) = r(i, j);
    }
  }
  return result;
}

QVector3D MeshInstance::translationVector() const {
  auto t = m_transform.translation();
  return QVector3D(t(0), t(1), t(2));
}

void MeshInstance::setTransform(const MeshTransform &transform) {
  m_transform = transform;
}

bool MeshInstance::isTransparent() const { return m_transparent; }
float MeshInstance::getTransparency() const { return m_transparency; }

void MeshInstance::setTransparent(bool transparent) {
  if (transparent == m_transparent)
    return;
  m_transparent = transparent;
  emit transparencyChanged();
}

void MeshInstance::setTransparency(float transparency) {
  if (transparency == m_transparency)
    return;
  m_transparency = transparency;
  emit transparencyChanged();
}
bool MeshInstance::isVisible() const { return m_visible; }

void MeshInstance::setVisible(bool visible) {
  if (m_visible == visible)
    return;
  m_visible = visible;
  emit visibilityChanged();
}

const QString &MeshInstance::getSelectedProperty() const {
  return m_selectedProperty;
}

bool MeshInstance::setSelectedProperty(const QString &propName) {
  if (!m_mesh)
    return false;
  if (m_selectedProperty == propName)
    return true;
  qDebug() << "Called MeshInstance.setSelectedProperty" << propName;
  const auto &props = m_mesh->vertexProperties();
  if (props.find(propName) == props.end())
    return false;
  m_selectedProperty = propName;
  emit selectedPropertyChanged();
  return true;
}

float MeshInstance::valueForSelectedPropertyAt(size_t index) const {
  if (!m_mesh)
    return 0.0f;
  const auto prop = m_mesh->vertexProperty(m_selectedProperty);
  if (index > prop.rows())
    return 0.0f;
  return prop(index);
}

MeshInstance *MeshInstance::newInstanceFromSelectedAtoms(
    Mesh *mesh, const std::vector<GenericAtomIndex> &atoms) {
  if (!mesh)
    return nullptr;
  const auto &meshAtoms = mesh->atomsInside();
  if (meshAtoms.size() < 1)
    return nullptr;
  ChemicalStructure *structure =
      qobject_cast<ChemicalStructure *>(mesh->parent());

  if (!structure)
    return nullptr;

  MeshTransform transform;
  if (!structure->getTransformation(meshAtoms, atoms, transform))
    return nullptr;
  if (mesh->haveChildMatchingTransform(transform))
    return nullptr;
  auto instance = new MeshInstance(mesh, transform);
  auto fragment = structure->makeFragment(atoms);
  QString fragmentLabel = fragment.name;

  instance->setObjectName(fragmentLabel);
  return instance;
}

// TODO speed up these queries rather than iterating through all points
MeshInstance::NearestPointResult
MeshInstance::nearestPoint(const Fragment &other) const {
  const auto v = vertices();
  MeshInstance::NearestPointResult result;

  for (size_t i = 0; i < v.cols(); i++) {
    const occ::Vec3 &p1 = v.col(i);
    for (size_t j = 0; j < other.size(); j++) {
      const occ::Vec3 &p2 = other.positions.col(j);
      double d = (p2 - p1).norm();
      if (d < result.distance) {
        result.idx_this = i;
        result.idx_other = j;
        result.distance = d;
      }
    }
  }
  return result;
}

// TODO speed up these queries rather than iterating through all points
MeshInstance::NearestPointResult
MeshInstance::nearestPoint(const occ::Vec3 &p2) const {
  const auto v = vertices();
  MeshInstance::NearestPointResult result;

  for (size_t i = 0; i < v.cols(); i++) {
    const occ::Vec3 &p1 = v.col(i);
    double d = (p2 - p1).norm();
    if (d < result.distance) {
      result.idx_this = i;
      result.idx_other = 0;
      result.distance = d;
    }
  }
  return result;
}

MeshInstance::NearestPointResult
MeshInstance::nearestPoint(const MeshInstance *other) const {
  const auto v = vertices();
  const auto v2 = other->vertices();

  MeshInstance::NearestPointResult result;
  for (size_t i = 0; i < v.cols(); i++) {
    const occ::Vec3 &p1 = v.col(i);
    for (size_t j = 0; j < v2.cols(); j++) {
      const occ::Vec3 &p2 = v2.col(j);
      double d = (p2 - p1).norm();
      if (d < result.distance) {
        result.idx_this = i;
        result.idx_other = j;
        result.distance = d;
      }
    }
  }
  return result;
}

void MeshInstance::populateSurroundingAtoms() {
  ChemicalStructure *structure =
      qobject_cast<ChemicalStructure *>(m_mesh->parent());
  if (structure) {
    m_atomsInside = structure->getAtomIndicesUnderTransformation(
        m_mesh->atomsInside(), m_transform);
    m_atomsOutside = structure->getAtomIndicesUnderTransformation(
        m_mesh->atomsOutside(), m_transform);
    qDebug() << "m_atomsOutside" << m_atomsOutside.size();
  }
}
