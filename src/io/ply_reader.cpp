#include "ply_reader.h"
#include "io_utilities.h"
#include "isosurface_parameters.h"
#include <fmt/core.h>
#include <QDebug>
#include <fstream>
#include <sstream>
#include <vector>

PlyReader::PlyReader(const QString &filepath, bool preloadIntoMemory)
    : m_filepath(filepath), m_preloadIntoMemory(preloadIntoMemory),
      m_plyFile(std::make_unique<tinyply::PlyFile>()) {}

PlyReader::~PlyReader() = default;

bool PlyReader::parseFile() {
  if (m_preloadIntoMemory) {
    qDebug() << "Reading PLY file into memory:" << m_filepath;
    return parseFileFromBuffer(
        io::readFileBytes(m_filepath, QIODevice::ReadOnly));
  } else {
    qDebug() << "Reading PLY file directly from disk:" << m_filepath;
    return parseFileFromDisk();
  }
}

bool PlyReader::parseFileFromBuffer(const QByteArray &buffer) {
  if (buffer.isEmpty()) {
    qDebug() << "Could not read file or file is empty:" << m_filepath;
    return false;
  }

  try {
    std::istringstream data_stream(
        std::string(buffer.constData(), buffer.size()));

    m_plyFile->parse_header(data_stream);

    requestProperties();
    m_plyFile->read(data_stream);
    maybeReadMetaData();

    return true;
  } catch (const std::exception &e) {
    qDebug() << "Error parsing PLY file from buffer:" << e.what();
    return false;
  }
}

bool PlyReader::parseFileFromDisk() {
  try {
    std::ifstream file(m_filepath.toStdString(), std::ios::binary);
    if (!file.is_open()) {
      qDebug() << "Could not open file:" << m_filepath;
      return false;
    }

    m_plyFile->parse_header(file);

    requestProperties();
    m_plyFile->read(file);
    maybeReadMetaData();

    return true;
  } catch (const std::exception &e) {
    qDebug() << "Error parsing PLY file from disk:" << e.what();
    return false;
  }
}

void PlyReader::requestProperties() {
  m_vertices =
      m_plyFile->request_properties_from_element("vertex", {"x", "y", "z"});
  m_faces =
      m_plyFile->request_properties_from_element("face", {"vertex_indices"}, 3);
  m_normals =
      m_plyFile->request_properties_from_element("vertex", {"nx", "ny", "nz"});

  processVertexProperties();
}

void PlyReader::processVertexProperties() {
  const std::vector<std::string> skip_properties = {"x",  "y",  "z",
                                                    "nx", "ny", "nz"};

  for (const auto &element : m_plyFile->get_elements()) {
    if (element.name != "vertex")
      continue;

    qDebug() << "Processing vertex element with" << element.properties.size()
             << "properties";

    for (const auto &property : element.properties) {
      if (std::find(skip_properties.begin(), skip_properties.end(),
                    property.name) != skip_properties.end()) {
        continue;
      }

      try {
        qDebug() << "Requesting property:"
                 << QString::fromStdString(property.name);
        m_properties[property.name] =
            m_plyFile->request_properties_from_element("vertex",
                                                       {property.name});
      } catch (const std::exception &e) {
        qDebug() << "Failed to request property"
                 << QString::fromStdString(property.name)
                 << "Error:" << e.what();
      }
    }
    break;
  }
}

void PlyReader::maybeReadMetaData() {
  const auto &comments = m_plyFile->get_comments();
  qDebug() << "Found num comments:" << comments.size();
  for (const auto &comment : m_plyFile->get_comments()) {
    qDebug() << "Comment" << QString::fromStdString(comment);
    auto loc = comment.rfind("metajson", 0);
    if (loc == 0) {
      std::string jsonStr =
          comment.substr(loc + 8); // 8 is length of "metajson"
      try {
        m_metaData = nlohmann::json::parse(jsonStr);
      } catch (const nlohmann::json::parse_error &e) {
        qWarning() << "JSON parsing error: "
                   << QString::fromStdString(e.what());
      }
      break;
    }
  }
}

void PlyReader::setMeshProperty(Mesh *mesh, const QString &displayName,
                                const std::shared_ptr<tinyply::PlyData> &prop) {
  if (!prop || !prop->buffer.get())
    return;

  try {
    switch (prop->t) {
    case tinyply::Type::FLOAT32: {
      auto buf = reinterpret_cast<const float *>(prop->buffer.get());
      mesh->setVertexProperty(
          displayName, Eigen::Map<const Eigen::VectorXf>(buf, prop->count));
      break;
    }
    case tinyply::Type::INT32: {
      auto buf = reinterpret_cast<const int *>(prop->buffer.get());
      mesh->setVertexProperty(
          displayName,
          Eigen::Map<const Eigen::VectorXi>(buf, prop->count).cast<float>());
      break;
    }
    case tinyply::Type::UINT32: {
      auto buf = reinterpret_cast<const uint32_t *>(prop->buffer.get());
      mesh->setVertexProperty(
          displayName,
          Eigen::Map<const Eigen::Matrix<uint32_t, Eigen::Dynamic, 1>>(
              buf, prop->count)
              .cast<float>());
      break;
    }
    case tinyply::Type::FLOAT64: {
      auto buf = reinterpret_cast<const double *>(prop->buffer.get());
      mesh->setVertexProperty(
          displayName,
          Eigen::Map<const Eigen::VectorXd>(buf, prop->count).cast<float>());
      break;
    }
    default:
      qDebug() << "Unsupported property type" << static_cast<int>(prop->t)
               << "for property" << displayName;
    }
  } catch (const std::exception &e) {
    qDebug() << "Error processing property" << displayName << ":" << e.what();
  }
}

void PlyReader::processMetaData(Mesh * mesh) {
  if(!mesh) return;

  Mesh::Attributes attr;

  const auto &j = metaData();
  qDebug() << j.size();

  if(j.contains("kind")) {
    attr.kind = isosurface::stringToKind(j.at("kind").get<QString>());
  }

  if(j.contains("isovalue")) {
    attr.isovalue = j.at("isovalue").get<float>();
  }

  if(j.contains("separation")) {
    attr.separation = j.at("separation").get<float>() * occ::units::BOHR_TO_ANGSTROM;
  }

  if(j.contains("description")) {
    QString name = j.at("description").get<QString>();
    mesh->setObjectName(QString("%1 [sep=%2,iso=%3]").arg(name).arg(attr.separation).arg(attr.isovalue));
  }


  mesh->setAttributes(attr);
}

Mesh *PlyReader::constructMesh() {
  if (!m_vertices || !m_faces) {
    qDebug() << "Required mesh data not loaded";
    return nullptr;
  }

  Mesh::VertexList vertexMatrix(3, m_vertices->count);
  Mesh::FaceList faceMatrix(3, m_faces->count);

  vertexMatrix = Eigen::Map<const Eigen::Matrix3Xf>(
                     reinterpret_cast<const float *>(m_vertices->buffer.get()),
                     3, m_vertices->count)
                     .cast<double>();

  faceMatrix = Eigen::Map<const Eigen::Matrix<uint32_t, 3, Eigen::Dynamic>>(
                   reinterpret_cast<const uint32_t *>(m_faces->buffer.get()), 3,
                   m_faces->count)
                   .cast<int>();

  Mesh *mesh = new Mesh(vertexMatrix, faceMatrix);

  if (m_normals && m_normals->count == m_vertices->count) {
    Mesh::VertexList normalMatrix(3, m_normals->count);
    normalMatrix = Eigen::Map<const Eigen::Matrix3Xf>(
                       reinterpret_cast<const float *>(m_normals->buffer.get()),
                       3, m_normals->count)
                       .cast<double>();
    mesh->setVertexNormals(normalMatrix);
  }

  mesh->setVertexProperty("None", Eigen::VectorXf::Zero(vertexMatrix.cols()));

  for (const auto &[prop_name, prop] : m_properties) {
    QString displayName = isosurface::getSurfacePropertyDisplayName(
        QString::fromStdString(prop_name));
    setMeshProperty(mesh, displayName, prop);
  }

  processMetaData(mesh);

  return mesh;
}

Mesh *PlyReader::read() {
  if (!parseFile()) {
    return nullptr;
  }

  return constructMesh();
}

Mesh *PlyReader::loadFromFile(const QString &filepath, bool preloadIntoMemory) {
  PlyReader reader(filepath, preloadIntoMemory);
  return reader.read();
}
