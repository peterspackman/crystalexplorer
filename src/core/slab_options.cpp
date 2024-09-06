#include "slab_options.h"

inline constexpr std::array<SlabGenerationOptions::Mode, 5> availableModes = {
    SlabGenerationOptions::Mode::Atoms,
    SlabGenerationOptions::Mode::UnitCellMolecules,
    SlabGenerationOptions::Mode::MoleculesCentroid,
    SlabGenerationOptions::Mode::MoleculesCenterOfMass,
    SlabGenerationOptions::Mode::MoleculesAnyAtom};

QStringList availableSlabGenerationModeOptions() {
  QStringList result;
  for (const auto &c : availableModes) {
    result.append(slabGenerationModeToString(c));
  }
  return result;
}

QString slabGenerationModeToString(SlabGenerationOptions::Mode mode) {
  switch (mode) {
  case SlabGenerationOptions::Mode::Atoms:
    return "Atoms inside";
  case SlabGenerationOptions::Mode::UnitCellMolecules:
    return "Unit cell molecules";
  case SlabGenerationOptions::Mode::MoleculesAnyAtom:
    return "Molecules with any atom inside";
  case SlabGenerationOptions::Mode::MoleculesCentroid:
    return "Molecules with centroid inside";
  case SlabGenerationOptions::Mode::MoleculesCenterOfMass:
    return "Molecules with center of mass inside";
  }
  return "Atoms";
}

SlabGenerationOptions::Mode slabGenerationModeFromString(const QString &s) {
  for (const auto &c : availableModes) {
    if (s == slabGenerationModeToString(c)) {
      return c;
    }
  }
  return SlabGenerationOptions::Mode::Atoms;
}
