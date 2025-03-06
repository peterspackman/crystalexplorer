#pragma once
#include "mesh.h"
#include <QList>
#include <QString>

namespace io {

Mesh *loadMesh(const QString &filename, bool preload = true);
QList<Mesh *> loadMeshes(QStringList &filenames, bool preload = true);

} // namespace io
