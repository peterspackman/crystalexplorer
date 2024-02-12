#include "isosurface.h"


Isosurface::Isosurface(const Isosurface::VertexList &vertices,
		       const Isosurface::FacesList &faces,
		       QObject *parent) : 
    QObject(parent), m_vertices(vertices), m_faces(faces) {

    updateVolume();
}


void Isosurface::updateVolume() {
    constexpr double volume_factor = 6.0;
    m_volume = 0.0;
    for (Eigen::Index i = 0; i < m_faces.rows(); ++i) {
	const auto &v0 = m_vertices.col(m_faces(i, 0));
	const auto &v1 = m_vertices.col(m_faces(i, 1));
	const auto &v2 = m_vertices.col(m_faces(i, 2));
	m_volume += v0.dot(v1.cross(v2));
    }
    m_volume /= volume_factor;
}

QString Isosurface::description() const {
    return m_description;
}

void Isosurface::setDescription(const QString &description) {
    m_description = description;
}

double Isosurface::volume() const {
    return m_volume;
}

Eigen::Index Isosurface::numberOfFaces() const {
    return m_faces.rows();
}

Eigen::Index Isosurface::numberOfVertices() const {
    return m_vertices.cols();
}

bool Isosurface::haveVertexNormals() const {
    return m_vertexNormals.cols() == m_vertices.cols();
}

Isosurface::Kind Isosurface::kind() const {
    return m_kind;
}
