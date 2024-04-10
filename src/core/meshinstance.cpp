#include "meshinstance.h"

MeshInstance::MeshInstance(Mesh *parent, const MeshTransform &transform)
    : QObject(parent), m_mesh(parent), m_transform(transform) {
    if(m_mesh) {
	m_selectedProperty = m_mesh->getSelectedProperty();
	// TODO remove these connections
	connect(this, &MeshInstance::visibilityChanged,
		m_mesh, &Mesh::visibilityChanged);
	connect(this, &MeshInstance::transparencyChanged,
		m_mesh, &Mesh::transparencyChanged);
	connect(this, &MeshInstance::selectedPropertyChanged,
		m_mesh, &Mesh::selectedPropertyChanged);
    }
}

const Mesh* MeshInstance::mesh() const { return m_mesh; }
Mesh* MeshInstance::mesh() { return m_mesh; }

Mesh::VertexList MeshInstance::vertices() const {
    if (!m_mesh) return {};
    return (m_transform.rotation() * m_mesh->vertices()).colwise() + m_transform.translation();
}

Mesh::VertexList MeshInstance::vertexNormals() const {
    if (!m_mesh) return {};
    return m_transform.rotation() * m_mesh->vertexNormals();
}


const MeshTransform& MeshInstance::transform() const {
    return m_transform; 
}

QMatrix3x3 MeshInstance::rotationMatrix() const {
    auto r = m_transform.rotation();
    QMatrix3x3 result;
    for(int i = 0; i < 3; i++) {
	for(int j = 0; j < 3; j++) {
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
    if (m_visible == visible) return;
    m_visible = visible;
    emit visibilityChanged();
}

const QString &MeshInstance::getSelectedProperty() const {
    return m_selectedProperty;
}

bool MeshInstance::setSelectedProperty(const QString &propName) {
    if(!m_mesh) return false;
    if(m_selectedProperty == propName) return true;
    qDebug() << "Called MeshInstance.setSelectedProperty" << propName;
    const auto &props = m_mesh->vertexProperties();
    if(props.find(propName) == props.end()) return false;
    m_selectedProperty = propName;
    emit selectedPropertyChanged();
    return true;
}


