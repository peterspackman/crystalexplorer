#include "load_mesh.h"
#include "ply_reader.h"

namespace io {

Mesh* loadMesh(const QString& filename) {
    auto mesh = PlyReader::loadFromFile(filename);
    return mesh ? mesh.release() : nullptr;
}

QList<Mesh*> loadMeshes(QStringList& filenames) {
    QList<Mesh*> result;
    for(const auto &filename: filenames) {
        result.append(loadMesh(filename));
    }
    return result;
}

} // namespace io
