#pragma once

#include "mesh.h"
#include "json.h"
#include <QString>
#include <vector>

namespace cx::io {

/**
 * @brief PLY file writer using tinyply library
 *
 * Writes Mesh objects to PLY format with:
 * - Vertex positions (x,y,z)
 * - Vertex normals (nx,ny,nz)
 * - Vertex colors (red,green,blue) if provided
 * - Vertex properties (custom float properties)
 * - Face indices
 * - Metadata in PLY comments
 */
class PlyWriter {
public:
  explicit PlyWriter(const QString &filepath);
  ~PlyWriter() = default;

  /**
   * @brief Write mesh to PLY file
   * @param mesh Mesh object to export
   * @param vertexColors Optional RGB vertex colors (3 floats per vertex: r,g,b in 0-1 range)
   * @param metadata Optional metadata to store in PLY comment
   * @return true if successful
   */
  bool write(const Mesh *mesh,
             const std::vector<float> &vertexColors = {},
             const nlohmann::json &metadata = {});

  /**
   * @brief Static helper to write mesh in one call
   */
  static bool writeToFile(const Mesh *mesh,
                          const QString &filepath,
                          const std::vector<float> &vertexColors = {},
                          const nlohmann::json &metadata = {});

private:
  QString m_filepath;
};

} // namespace cx::io
