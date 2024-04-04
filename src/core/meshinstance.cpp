#include "meshinstance.h"

MeshInstance::MeshInstance(Mesh *parent, const Eigen::Affine3d &transform)
    : QObject(parent), m_mesh(parent), m_transform(transform) {
}

const Mesh* MeshInstance::mesh() const { return m_mesh; }
Mesh* MeshInstance::mesh() { return m_mesh; }

Mesh::VertexList MeshInstance::vertices() const {
    if (!m_mesh) return {};
    return m_transform * m_mesh->vertices();
}

const MeshTransform& MeshInstance::transform() const {
    return m_transform; 
}

void MeshInstance::setTransform(const Eigen::Affine3d &transform) {
    m_transform = transform;
}

bool MeshInstance::isTransparent() const {
    return m_transparent;
}

void MeshInstance::setTransparent(bool transparent) {
    if(transparent == m_transparent) return;

    m_transparent = transparent;
    emit transparencyChanged();
}

bool MeshInstance::isVisible() const {
    return m_visible;
}

void MeshInstance::setVisible(bool visible) {
    if (m_visible != visible) {
	m_visible = visible;
	emit visibilityChanged();
    }
}

const QString &MeshInstance::getSelectedProperty() const {
    return m_selectedProperty;
}

bool MeshInstance::setSelectedProperty(const QString &propName) {
    if(!m_mesh) return false;
    if(m_selectedProperty == propName) return true;
    const auto &props = m_mesh->vertexProperties();
    if(props.find(propName) == props.end()) return false;
    m_selectedProperty = propName;
    emit selectedPropertyChanged();
    return true;
}


