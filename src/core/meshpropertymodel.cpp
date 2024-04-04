#include "meshpropertymodel.h"

MeshPropertyModel::MeshPropertyModel(QObject* parent)
    : QAbstractListModel(parent), m_mesh(nullptr) {}

void MeshPropertyModel::setMesh(Mesh* mesh) {
    beginResetModel();
    m_mesh = mesh;
    endResetModel();
}

int MeshPropertyModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid() || !m_mesh) {
        return 0;
    }
    return m_mesh->availableVertexProperties().size(); // Or face properties, depending on what you're listing
}

QVariant MeshPropertyModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || !m_mesh) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        // Assuming you want to list vertex properties. Adjust accordingly if you're listing something else.
        QStringList properties = m_mesh->availableVertexProperties();
        if (index.row() >= 0 && index.row() < properties.size()) {
            return properties.at(index.row());
        }
    }

    return QVariant();
}


const Mesh::ScalarPropertyValues &MeshPropertyModel::getPropertyValuesAtIndex(int index) const {
    if (!m_mesh || index < 0 || index >= m_mesh->availableVertexProperties().size()) {
        return m_mesh->vertexProperty("None");
    }
    // Retrieve the property name at the given index
    QString propertyName = m_mesh->availableVertexProperties().at(index);

    return m_mesh->vertexProperty(propertyName);
}

void MeshPropertyModel::setSelectedProperty(int row) const {
    if (!m_mesh || row < 0) return;

    QString propertyName = data(index(row, 0), Qt::DisplayRole).toString();
    m_mesh->setSelectedProperty(propertyName);
}

bool MeshPropertyModel::isTransparent() const {
    if (!m_mesh) return false;
    return m_mesh->isTransparent();
}

void MeshPropertyModel::setTransparent(bool transparent) const {
    if (!m_mesh) return;
    m_mesh->setTransparent(transparent);
}
