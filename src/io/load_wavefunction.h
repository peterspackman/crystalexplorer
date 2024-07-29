#pragma once
#include "molecular_wavefunction.h"

namespace io {

MolecularWavefunction *loadWavefunction(const QString &filename);
bool populateWavefunctionFromJsonContents(MolecularWavefunction *, const QByteArray &contents);

}
