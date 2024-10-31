#pragma once
#include "mesh.h"
#include <QList>
#include <QString>

namespace io {

Mesh * loadMesh(const QString &filename);
QList<Mesh*> loadMeshes(QStringList& filenames);

}
