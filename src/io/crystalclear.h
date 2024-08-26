#pragma once
#include "crystalstructure.h"

namespace io {

CrystalStructure *loadCrystalClearJson(const QString &filename);
void loadCrystalClearSurfaceJson(const QString &filename, CrystalStructure *);

}
