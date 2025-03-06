#include "load_mesh.h"
#include "ply_reader.h"

namespace io {

Mesh *loadMesh(const QString &filename, bool preload) {
  return PlyReader::loadFromFile(filename, preload);
}

QList<Mesh *> loadMeshes(QStringList &filenames, bool preload) {
  QList<Mesh *> result;
  for (const auto &filename : filenames) {
    result.append(loadMesh(filename, preload));
  }
  return result;
}

} // namespace io
