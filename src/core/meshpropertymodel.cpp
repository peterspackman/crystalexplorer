#include "meshpropertymodel.h"
#include <QIcon>

MeshPropertyModel::MeshPropertyModel(QObject *parent)
    : QAbstractListModel(parent), m_mesh(nullptr) {
  loadSurfaceDescriptionConfiguration(
      m_propertyDescriptions, m_surfaceDescriptions, m_defaultIsovalues);
}

bool MeshPropertyModel::isValid() const {
  if (m_meshInstance || m_mesh)
    return true;
  return false;
}

QString MeshPropertyModel::getSelectedProperty() const {
  if (!m_mesh)
    return "";
  if (m_meshInstance) {
    return m_meshInstance->getSelectedProperty();
  } else {
    return m_mesh->getSelectedProperty();
  }
}

void MeshPropertyModel::setMeshInstance(MeshInstance *meshInstance) {
  if (m_meshInstance == meshInstance)
    return;
  m_blockedWhileResetting = true;
  beginResetModel();
  m_mesh = meshInstance->mesh();
  m_meshInstance = meshInstance;
  QString prop = meshInstance->getSelectedProperty();
  endResetModel();
  setSelectedProperty(prop);
  m_blockedWhileResetting = false;
}

void MeshPropertyModel::setMesh(Mesh *mesh) {
  if ((m_mesh == mesh) && (!m_meshInstance))
    return;
  m_blockedWhileResetting = true;
  beginResetModel();
  m_mesh = mesh;
  m_meshInstance = nullptr;
  QString prop = mesh->getSelectedProperty();
  endResetModel();
  setSelectedProperty(prop);
  m_blockedWhileResetting = false;
}

int MeshPropertyModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid() || !m_mesh) {
    return 0;
  }
  return m_mesh->availableVertexProperties().size();
}

QVariant MeshPropertyModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || !m_mesh) {
    return QVariant();
  }
  QStringList properties = m_mesh->availableVertexProperties();
  if (index.row() < 0 || index.row() >= properties.size()) {
    return QVariant();
  }
  QString propertyName = properties.at(index.row());

  switch (role) {
  case Qt::DisplayRole: {
    // Use display name from property description if available
    auto it = m_propertyDescriptions.find(propertyName);
    if (it != m_propertyDescriptions.end()) {
      return it->displayName;
    }
    return propertyName;
  }
  case Qt::DecorationRole: {
    // Load icon if specified in the property description
    auto it = m_propertyDescriptions.find(propertyName);
    if (it != m_propertyDescriptions.end() && !it->iconName.isEmpty()) {
      return QIcon(QString(":/images/%1").arg(it->iconName));
    }
    return QVariant();
  }
  case PropertyNameRole:
    return propertyName;
  case PropertyUnitsRole: {
    auto it = m_propertyDescriptions.find(propertyName);
    if (it != m_propertyDescriptions.end()) {
      return it->units;
    }
    return "";
  }
  case PropertyDescriptionRole: {
    auto it = m_propertyDescriptions.find(propertyName);
    if (it != m_propertyDescriptions.end()) {
      return it->description;
    }
    return "";
  }
  case PropertyColorMapRole: {
    auto it = m_propertyDescriptions.find(propertyName);
    if (it != m_propertyDescriptions.end()) {
      return it->cmap;
    }
    return "Viridis"; // default colormap
  }
  case VolumeRole:
    return volume();
  case AreaRole:
    return area();
  case GlobularityRole:
    return globularity();
  case AsphericityRole:
    return asphericity();
  case TransparentRole:
    return isTransparent();
  case TransparencyRole:
    return getTransparency();
  case FingerprintableRole:
    return isFingerprintable();
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> MeshPropertyModel::roleNames() const {
  QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
  roles[PropertyNameRole] = "propertyName";
  roles[PropertyUnitsRole] = "propertyUnits";
  roles[PropertyDescriptionRole] = "propertyDescription";
  roles[PropertyColorMapRole] = "propertyColorMap";
  roles[VolumeRole] = "volume";
  roles[AreaRole] = "area";
  roles[GlobularityRole] = "globularity";
  roles[AsphericityRole] = "asphericity";
  roles[TransparentRole] = "transparent";
  roles[TransparencyRole] = "transparency";
  roles[FingerprintableRole] = "fingerprintable";
  return roles;
}

double MeshPropertyModel::volume() const {
  if (!m_mesh)
    return 0.0;
  return m_mesh->volume();
}

double MeshPropertyModel::area() const {
  if (!m_mesh)
    return 0.0;
  return m_mesh->surfaceArea();
}

double MeshPropertyModel::globularity() const {
  if (!m_mesh)
    return 0.0;
  return m_mesh->globularity();
}

double MeshPropertyModel::asphericity() const {
  if (!m_mesh)
    return 0.0;
  return m_mesh->asphericity();
}

bool MeshPropertyModel::isFingerprintable() const {
  if (!m_mesh)
    return false;
  const auto &params = m_mesh->parameters();
  return params.kind == isosurface::Kind::Hirshfeld && params.separation < 0.21;
}

MeshPropertyModel::PropertyStatistics
MeshPropertyModel::getSelectedPropertyStatistics() const {

  if (!m_mesh && !m_meshInstance)
    return {};

  const auto &values = m_mesh->vertexProperty(getSelectedProperty());
  return {values.minCoeff(), values.maxCoeff(), values.mean()};
}

Mesh::ScalarPropertyRange MeshPropertyModel::getSelectedPropertyRange() const {
  if (!m_mesh)
    return {};
  return m_mesh->vertexPropertyRange(getSelectedProperty());
}

QString MeshPropertyModel::getSelectedPropertyColorMap() const {
  if (!m_mesh)
    return "Viridis";
  QString propertyName = getSelectedProperty();
  auto it = m_propertyDescriptions.find(propertyName);
  if (it != m_propertyDescriptions.end()) {
    return it->cmap;
  }
  return "Viridis";
}

void MeshPropertyModel::setSelectedPropertyColorMap(const QString &mapName) {
  // TODO
  qDebug() << "TODO implement setting property color map";
}

void MeshPropertyModel::setSelectedPropertyRange(
    Mesh::ScalarPropertyRange range) {
  if (!m_mesh)
    return;

  QString propertyName;
  if (m_meshInstance) {
    propertyName = m_meshInstance->getSelectedProperty();
  } else if (m_mesh) {
    propertyName = m_mesh->getSelectedProperty();
  }
  m_mesh->setVertexPropertyRange(propertyName, range);
  emit propertySelectionChanged(propertyName);
}

void MeshPropertyModel::setSelectedProperty(QString propertyName) {
  if (!(m_mesh || m_meshInstance))
    return;
  if (m_blockedWhileResetting) {
    emit propertySelectionChanged(propertyName);
    return;
  }

  if (m_meshInstance) {
    m_meshInstance->setSelectedProperty(propertyName);
  } else {
    m_mesh->setSelectedProperty(propertyName);
  }
  emit propertySelectionChanged(propertyName);
}

bool MeshPropertyModel::isTransparent() const {
  if (m_meshInstance)
    return m_meshInstance->isTransparent();
  if (!m_mesh)
    return false;
  return m_mesh->isTransparent();
}

void MeshPropertyModel::setTransparent(bool transparent) {
  if (!m_mesh)
    return;
  if (m_meshInstance) {
    m_meshInstance->setTransparent(transparent);
  } else {
    m_mesh->setTransparent(transparent);
  }
}

float MeshPropertyModel::getTransparency() const {
  if (m_meshInstance)
    return m_meshInstance->getTransparency();
  if (!m_mesh)
    return false;
  return m_mesh->getTransparency();
}
void MeshPropertyModel::setTransparency(float transparency) {
  if (!m_mesh)
    return;
  if (m_meshInstance) {
    m_meshInstance->setTransparency(transparency);
  } else {
    m_mesh->setTransparency(transparency);
  }
}
Mesh *MeshPropertyModel::getMesh() { return m_mesh; }
MeshInstance *MeshPropertyModel::getMeshInstance() { return m_meshInstance; }
