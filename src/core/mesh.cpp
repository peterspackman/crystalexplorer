#include "mesh.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

using VertexList = Mesh::VertexList;
using FaceList = Mesh::FaceList;
using ScalarPropertyValues = Mesh::ScalarPropertyValues;
using ScalarProperties = Mesh::ScalarProperties;

Mesh::Mesh(QObject * parent) : QObject(parent) {}

Mesh::Mesh(Eigen::Ref<const VertexList> vertices, Eigen::Ref<const FaceList> faces, QObject * parent) : QObject(parent), m_vertices(vertices), m_faces(faces) {
    updateVertexFaceMapping();
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

// Vertex Properties
const ScalarProperties &Mesh::vertexProperties() const {
    return m_vertexProperties;
}

void Mesh::setVertexProperty(const QString &name, const ScalarPropertyValues &values) {
    m_vertexProperties[name] = values;
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

MeshInstance::MeshInstance(Mesh *parent, const Eigen::Affine3d &transform)
    : QObject(parent), m_mesh(parent), m_transform(transform) {
}
