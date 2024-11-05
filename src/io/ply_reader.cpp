#include "ply_reader.h"
#include "isosurface_parameters.h"
#include <QDebug>
#include <fstream>

namespace {
class MemoryBuffer : public std::streambuf {
public:
  MemoryBuffer(char const *first_elem, size_t size)
      : p_start(const_cast<char *>(first_elem)), p_end(p_start + size) {
    setg(p_start, p_start, p_end);
  }

private:
  pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                   std::ios_base::openmode) override {
    if (dir == std::ios_base::cur)
      gbump(static_cast<int>(off));
    else
      setg(p_start, (dir == std::ios_base::beg ? p_start : p_end) + off, p_end);
    return gptr() - p_start;
  }

  pos_type seekpos(pos_type pos, std::ios_base::openmode which) override {
    return seekoff(pos, std::ios_base::beg, which);
  }

  char *p_start{nullptr};
  char *p_end{nullptr};
};

class MemoryStream : virtual MemoryBuffer, public std::istream {
public:
  MemoryStream(char const *first_elem, size_t size)
      : MemoryBuffer(first_elem, size),
        std::istream(static_cast<std::streambuf *>(this)) {}
};
} // namespace

PlyReader::PlyReader(std::unique_ptr<std::istream> stream)
    : m_stream(std::move(stream)),
      m_plyFile(std::make_unique<tinyply::PlyFile>()) {}

PlyReader::PlyReader(const QString &filepath) {
  auto buffer = readFileBinary(filepath);
  m_stream = std::make_unique<MemoryStream>(
      reinterpret_cast<char *>(buffer.data()), buffer.size());
  m_plyFile = std::make_unique<tinyply::PlyFile>();
}

std::vector<uint8_t> PlyReader::readFileBinary(const QString &filepath) {
  std::ifstream file(filepath.toStdString(), std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file: " + filepath.toStdString());
  }

  file.seekg(0, std::ios::end);
  size_t sizeBytes = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> buffer(sizeBytes);
  if (!file.read(reinterpret_cast<char *>(buffer.data()), sizeBytes)) {
    throw std::runtime_error("Failed to read file: " + filepath.toStdString());
  }

  return buffer;
}

void PlyReader::parseHeader() {
  if (!m_stream || m_stream->fail()) {
    throw std::runtime_error("Invalid stream");
  }
  m_plyFile->parse_header(*m_stream);
}

void PlyReader::requestProperties() {
  // Request core geometric properties
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

void PlyReader::readFileData() { m_plyFile->read(*m_stream); }

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

std::unique_ptr<Mesh> PlyReader::constructMesh() {
  if (!m_vertices || !m_faces) {
    throw std::runtime_error("Required mesh data not loaded");
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

  auto mesh = std::make_unique<Mesh>(vertexMatrix, faceMatrix);

  // Set normals if available
  if (m_normals && m_normals->count == m_vertices->count) {
    Mesh::VertexList normalMatrix(3, m_normals->count);
    normalMatrix = Eigen::Map<const Eigen::Matrix3Xf>(
                       reinterpret_cast<const float *>(m_normals->buffer.get()),
                       3, m_normals->count)
                       .cast<double>();
    mesh->setVertexNormals(normalMatrix);
  }

  // Set default property
  mesh->setVertexProperty("None", Eigen::VectorXf::Zero(vertexMatrix.cols()));

  // Process additional properties
  for (const auto &[prop_name, prop] : m_properties) {
    QString displayName =
        isosurface::getSurfacePropertyDisplayName(QString::fromStdString(prop_name));
    setMeshProperty(mesh.get(), displayName, prop);
  }

  return mesh;
}

std::unique_ptr<Mesh> PlyReader::read() {
  try {
    parseHeader();
    requestProperties();
    readFileData();
    return constructMesh();
  } catch (const std::exception &e) {
    qDebug() << "Error reading PLY file:" << e.what();
    return nullptr;
  }
}

std::unique_ptr<Mesh> PlyReader::loadFromFile(const QString &filepath) {
  PlyReader reader(filepath);
  return reader.read();
}
