#include "mesh.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <fmt/os.h>
#include <QSignalBlocker>
#include "meshinstance.h"

using VertexList = Mesh::VertexList;
using FaceList = Mesh::FaceList;
using ScalarPropertyValues = Mesh::ScalarPropertyValues;
using ScalarProperties = Mesh::ScalarProperties;

Mesh::Mesh(QObject * parent) : QObject(parent) {}

Mesh::Mesh(Eigen::Ref<const VertexList> vertices, Eigen::Ref<const FaceList> faces, QObject * parent) : QObject(parent), m_vertices(vertices), m_faces(faces) {
    updateVertexFaceMapping();
    updateFaceProperties();
}

Mesh::Mesh(Eigen::Ref<const VertexList> vertices, QObject * parent) : QObject(parent), m_vertices(vertices) {
}

void Mesh::updateVertexFaceMapping() {
  m_facesUsingVertex = std::vector<std::vector<int>>(numberOfVertices());
  for (int f = 0; f < numberOfFaces(); f++) {
      for(int i = 0; i < 3; i++) {
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
	    if(setting == Mesh::NormalSetting::Flat) break;
        }
        normals.col(i) = normal.normalized();
    }
    return normals;
}

// Description related stuff
QString Mesh::description() const {
    return m_description;
}

void Mesh::setDescription(const QString &description) {
    m_description = description;
}

void Mesh::updateFaceProperties() {

    const int N = numberOfFaces();
    m_faceAreas.resize(N);
    m_faceVolumeContributions.resize(N);
    m_faceNormals.resize(3, N);

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
	m_faceVolumeContributions(i) = m_faceAreas(i) * m_faceNormals.col(i).dot(v0) / 3.0;
    }
    updateAsphericity();
    m_volume = m_faceVolumeContributions.sum();
    m_surfaceArea = m_faceAreas.sum();
    m_globularity = 0.0;
    if(m_volume > 0.0 && m_surfaceArea > 0.0) {
	const double fac = std::cbrt(36.0 * M_PI);
	m_globularity = fac * std::cbrt(m_volume * m_volume) / m_surfaceArea;
    }

}

void Mesh::updateAsphericity() {
    const int N = numberOfVertices();

    Eigen::Vector3d centroid = m_vertices.rowwise().mean();
    fmt::print("Centroid\n{} {} {}\n", centroid(0), centroid(1), centroid(2));

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
    qDebug() << "asphericity" << m_asphericity;
}

double Mesh::surfaceArea() const {
    return m_surfaceArea;
}

double Mesh::volume() const {
    return m_volume;
}

double Mesh::globularity() const {
    return m_globularity;
}

// Vertex Properties
const ScalarProperties &Mesh::vertexProperties() const {
    return m_vertexProperties;
}

void Mesh::setVertexProperty(const QString &name, const ScalarPropertyValues &values) {
    m_vertexProperties[name] = values;
    setSelectedProperty(name);
}

const ScalarPropertyValues &Mesh::vertexProperty(const QString &name) const {
    const auto loc = m_vertexProperties.find(name);
    if(loc != m_vertexProperties.end()) {
	return loc->second;
    }
    return m_emptyProperty;
}

QStringList Mesh::availableVertexProperties() const {
    QStringList result;
    for(const auto &prop: m_vertexProperties) {
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

void Mesh::setFaceProperty(const QString &name, const ScalarPropertyValues &values) {
    m_faceProperties[name] = values;
}

const ScalarPropertyValues &Mesh::faceProperty(const QString &name) const {
    const auto loc = m_faceProperties.find(name);
    if(loc != m_faceProperties.end()) {
	return loc->second;
    }
    return m_emptyProperty;
}

QStringList Mesh::availableFaceProperties() const {
    QStringList result;
    for(const auto &prop: m_faceProperties) {
	result.append(prop.first);
    }
    return result;
}

bool Mesh::haveFaceProperty(const QString &prop) const {
    return m_faceProperties.find(prop) != m_faceProperties.end();
}


bool Mesh::isTransparent() const {
    return m_transparent;
}

void Mesh::setTransparent(bool transparent) {
    if(transparent == m_transparent) return;
    m_transparent = transparent;
    {
	QSignalBlocker blocker(this);
	for(auto * child: children()) {
	    auto * instance = qobject_cast<MeshInstance *>(child);
	    if(instance) instance->setTransparent(m_visible);
	}
    }

    emit transparencyChanged();
}

bool Mesh::isVisible() const {
    return m_visible;
}

void Mesh::setVisible(bool visible) {
    if (m_visible != visible) {
	m_visible = visible;
	{
	    QSignalBlocker blocker(this);
	    for(auto * child: children()) {
		auto * instance = qobject_cast<MeshInstance *>(child);
		if(instance) instance->setVisible(m_visible);
	    }
	}
	emit visibilityChanged();
    }
}

const QString &Mesh::getSelectedProperty() const {
    return m_selectedProperty;
}

bool Mesh::setSelectedProperty(const QString &propName) {
    if(m_selectedProperty == propName) return true;
    if(m_vertexProperties.find(propName) == m_vertexProperties.end()) return false;

    m_selectedProperty = propName;
    {
	QSignalBlocker blocker(this);
	for(auto * child: children()) {
	    auto * instance = qobject_cast<MeshInstance *>(child);
	    if(instance) instance->setSelectedProperty(m_selectedProperty);
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

    QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (document.isNull() || !document.isObject()) {
	qWarning("Invalid JSON format.");
	return nullptr;
    }

    QJsonObject object = document.object();
    return Mesh::newFromJson(object, parent);
}

Mesh *Mesh::newFromJson(const QJsonObject &object, QObject *parent) {
  Mesh *pointCloud = new Mesh(parent);
  // Parse points
  QJsonArray pointsArray = object["points"].toArray();
  pointCloud->m_vertices.resize(3, pointsArray.size());
  for (int i = 0; i < pointsArray.size(); ++i) {
    QJsonArray pointArray = pointsArray[i].toArray();
    pointCloud->m_vertices(0, i) = pointArray[0].toDouble();
    pointCloud->m_vertices(1, i) = pointArray[1].toDouble();
    pointCloud->m_vertices(2, i) = pointArray[2].toDouble();
  }

  // Parse properties
  QJsonObject propertiesObj = object["properties"].toObject();
  QStringList propertyKeys = propertiesObj.keys();
  for (const QString &key : propertyKeys) {
    QJsonArray propertyValuesArray = propertiesObj[key].toArray();
    ScalarPropertyValues propertyValues(propertyValuesArray.size());
    for (int i = 0; i < propertyValuesArray.size(); ++i) {
      propertyValues(i) = propertyValuesArray[i].toDouble();
    }
    pointCloud->setVertexProperty(key, propertyValues);
  }

  return pointCloud;
}

