#include "ply_writer.h"
#include "isosurface_parameters.h"
#include "tinyply.h"
#include <QDebug>
#include <fstream>
#include <occ/core/units.h>

namespace cx::io {

PlyWriter::PlyWriter(const QString &filepath) : m_filepath(filepath) {}

bool PlyWriter::write(const Mesh *mesh,
                      const std::vector<float> &vertexColors,
                      const nlohmann::json &metadata) {
  if (!mesh) {
    qWarning() << "PlyWriter: null mesh";
    return false;
  }

  if (mesh->numberOfVertices() == 0 || mesh->numberOfFaces() == 0) {
    qWarning() << "PlyWriter: empty mesh";
    return false;
  }

  try {
    tinyply::PlyFile plyFile;

    // Vertices
    const auto &vertices = mesh->vertices();
    int numVertices = vertices.cols();
    std::vector<float> vertexData(numVertices * 3);
    for (int i = 0; i < numVertices; ++i) {
      vertexData[i * 3 + 0] = static_cast<float>(vertices(0, i));
      vertexData[i * 3 + 1] = static_cast<float>(vertices(1, i));
      vertexData[i * 3 + 2] = static_cast<float>(vertices(2, i));
    }
    plyFile.add_properties_to_element(
        "vertex", {"x", "y", "z"}, tinyply::Type::FLOAT32, numVertices,
        reinterpret_cast<uint8_t *>(vertexData.data()),
        tinyply::Type::INVALID, 0);

    // Normals
    if (mesh->haveVertexNormals()) {
      const auto &normals = mesh->vertexNormals();
      std::vector<float> normalData(numVertices * 3);
      for (int i = 0; i < numVertices; ++i) {
        normalData[i * 3 + 0] = static_cast<float>(normals(0, i));
        normalData[i * 3 + 1] = static_cast<float>(normals(1, i));
        normalData[i * 3 + 2] = static_cast<float>(normals(2, i));
      }
      plyFile.add_properties_to_element(
          "vertex", {"nx", "ny", "nz"}, tinyply::Type::FLOAT32, numVertices,
          reinterpret_cast<uint8_t *>(normalData.data()),
          tinyply::Type::INVALID, 0);
    }

    // Vertex colors (if provided from renderer)
    if (!vertexColors.empty() &&
        vertexColors.size() == static_cast<size_t>(numVertices * 3)) {
      std::vector<uint8_t> colorData(numVertices * 3);
      for (size_t i = 0; i < vertexColors.size(); ++i) {
        colorData[i] = static_cast<uint8_t>(
            std::clamp(vertexColors[i] * 255.0f, 0.0f, 255.0f));
      }
      plyFile.add_properties_to_element(
          "vertex", {"red", "green", "blue"}, tinyply::Type::UINT8,
          numVertices, colorData.data(), tinyply::Type::INVALID, 0);
    }

    // Vertex properties (raw property values)
    QString selectedProperty = mesh->getSelectedProperty();
    if (!selectedProperty.isEmpty() &&
        mesh->haveVertexProperty(selectedProperty)) {
      const auto &propData = mesh->vertexProperty(selectedProperty);
      std::vector<float> floatData(propData.rows());
      for (int i = 0; i < propData.rows(); ++i) {
        floatData[i] = propData(i);
      }

      std::string propKey = selectedProperty.toStdString();
      plyFile.add_properties_to_element(
          "vertex", {propKey}, tinyply::Type::FLOAT32, numVertices,
          reinterpret_cast<uint8_t *>(floatData.data()),
          tinyply::Type::INVALID, 0);
    }

    // Faces
    const auto &faces = mesh->faces();
    int numFaces = faces.cols();
    std::vector<uint32_t> faceData(numFaces * 3);
    for (int i = 0; i < numFaces; ++i) {
      faceData[i * 3 + 0] = static_cast<uint32_t>(faces(0, i));
      faceData[i * 3 + 1] = static_cast<uint32_t>(faces(1, i));
      faceData[i * 3 + 2] = static_cast<uint32_t>(faces(2, i));
    }
    plyFile.add_properties_to_element(
        "face", {"vertex_indices"}, tinyply::Type::UINT32, numFaces,
        reinterpret_cast<uint8_t *>(faceData.data()), tinyply::Type::UINT8, 3);

    // Metadata
    if (!metadata.empty()) {
      std::string metaComment = "metajson" + metadata.dump();
      plyFile.get_comments().push_back(metaComment);
    }

    // Write to file
    std::ofstream file(m_filepath.toStdString(), std::ios::binary);
    if (!file.is_open()) {
      qWarning() << "PlyWriter: could not open file:" << m_filepath;
      return false;
    }

    plyFile.write(file, false); // ASCII format for compatibility
    qDebug() << "PlyWriter: wrote" << numVertices << "vertices," << numFaces
             << "faces to" << m_filepath;
    return true;

  } catch (const std::exception &e) {
    qWarning() << "PlyWriter: exception:" << e.what();
    return false;
  }
}

bool PlyWriter::writeToFile(const Mesh *mesh, const QString &filepath,
                            const std::vector<float> &vertexColors,
                            const nlohmann::json &metadata) {
  PlyWriter writer(filepath);
  return writer.write(mesh, vertexColors, metadata);
}

} // namespace cx::io
