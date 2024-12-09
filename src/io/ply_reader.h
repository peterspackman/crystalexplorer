#pragma once

#include "mesh.h"
#include "tinyply.h"
#include <QString>
#include <istream>
#include <memory>
#include <optional>
#include <vector>

class PlyReader {
public:
  explicit PlyReader(std::unique_ptr<std::istream> stream);
  explicit PlyReader(const QString &filepath);

  std::unique_ptr<Mesh> read();
  static std::unique_ptr<Mesh> loadFromFile(const QString &filepath);

private:
  virtual void parseHeader();
  virtual void requestProperties();
  virtual void readFileData();
  virtual std::unique_ptr<Mesh> constructMesh();

  void processVertexProperties();
  void setMeshProperty(Mesh *mesh, const QString &displayName,
                       const std::shared_ptr<tinyply::PlyData> &prop);

  static std::vector<uint8_t> readFileBinary(const QString &filepath);

  std::unique_ptr<std::istream> m_stream;
  std::unique_ptr<tinyply::PlyFile> m_plyFile;

  std::shared_ptr<tinyply::PlyData> m_vertices;
  std::shared_ptr<tinyply::PlyData> m_faces;
  std::shared_ptr<tinyply::PlyData> m_normals;
  std::map<std::string, std::shared_ptr<tinyply::PlyData>> m_properties;
  std::vector<uint8_t> m_fileBuffer;
};
