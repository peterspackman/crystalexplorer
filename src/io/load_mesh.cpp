#include "load_mesh.h"
#include "ply_reader.h"

namespace io {

Mesh* loadMesh(const QString& filename) {
    auto mesh = PlyReader::loadFromFile(filename);
    return mesh ? mesh.release() : nullptr;
}

} // namespace io
