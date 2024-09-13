#pragma once
#include "molecular_wavefunction.h"

namespace io {

MolecularWavefunction *loadWavefunction(const QString &filename);
bool populateWavefunctionFromJsonContents(MolecularWavefunction *, const QByteArray &contents);
bool populateWavefunctionFromMoldenContents(MolecularWavefunction *, const QByteArray &contents);
bool populateWavefunctionFromXtbPropertiesContents(MolecularWavefunction *, const QByteArray &contents);
bool populateWavefunctionFromXtbStdoutContents(MolecularWavefunction *, const QByteArray &contents);

}
