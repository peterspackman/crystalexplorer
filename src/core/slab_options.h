#pragma once
#include <QVector3D>
#include <QStringList>

struct SlabGenerationOptions {
  enum class Mode{
    Atoms,
    UnitCellMolecules,
    MoleculesCentroid,
    MoleculesCenterOfMass,
    MoleculesAnyAtom,
  };
  QVector3D lowerBound{0.0, 0.0, 0.0};
  QVector3D upperBound{1.0, 1.0, 1.0};
  Mode mode{Mode::Atoms};
};

QString slabGenerationModeToString(SlabGenerationOptions::Mode mode);
SlabGenerationOptions::Mode slabGenerationModeFromString(const QString &s);
QStringList availableSlabGenerationModeOptions();
