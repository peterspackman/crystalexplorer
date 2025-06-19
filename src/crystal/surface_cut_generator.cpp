#include "surface_cut_generator.h"
#include <occ/crystal/surface.h>
#include <QDebug>

namespace cx::crystal {

SlabStructure* generateSurfaceCut(
  CrystalStructure *crystalStructure,
  int h, int k, int l, 
  double cutOffset,
  double thickness
) {
  if (!crystalStructure) {
    qDebug() << "Invalid crystal structure provided";
    return nullptr;
  }

  try {
    // Create surface cut options
    CrystalSurfaceCutOptions options;
    options.millerPlane = occ::crystal::HKL{h, k, l};
    options.cutOffset = cutOffset;
    options.thickness = thickness;
    options.preserveMolecules = true;
    options.termination = "auto";

    // Validate Miller indices
    if (h == 0 && k == 0 && l == 0) {
      qDebug() << "Invalid Miller indices (0,0,0)";
      return nullptr;
    }

    // Create new slab structure
    SlabStructure *slab = new SlabStructure();
    slab->buildFromCrystal(*crystalStructure, options);

    return slab;

  } catch (const std::exception &e) {
    qDebug() << "Error generating surface cut:" << e.what();
    return nullptr;
  }
}

std::vector<double> getSuggestedCuts(
  CrystalStructure *crystalStructure,
  int h, int k, int l
) {
  std::vector<double> cuts;
  if (!crystalStructure) {
    return cuts;
  }

  try {
    occ::crystal::HKL hkl{h, k, l};
    if (hkl.h == 0 && hkl.k == 0 && hkl.l == 0) {
      return cuts;
    }

    occ::crystal::Surface surface(hkl, crystalStructure->occCrystal());
    const auto &positions = crystalStructure->atomicPositions();
    cuts = surface.possible_cuts(positions);

  } catch (const std::exception &e) {
    qDebug() << "Error getting suggested cuts:" << e.what();
  }

  return cuts;
}

QList<SlabStructure*> generateSuggestedSurfaceCuts(
  CrystalStructure *crystalStructure,
  int h, int k, int l,
  double thickness
) {
  QList<SlabStructure*> results;
  
  std::vector<double> cuts = getSuggestedCuts(crystalStructure, h, k, l);
  
  for (double cut : cuts) {
    SlabStructure* cutStructure = generateSurfaceCut(
      crystalStructure, h, k, l, cut, thickness);
    if (cutStructure) {
      results.append(cutStructure);
    }
  }
  
  return results;
}

// SlabStructure handles all the conversion internally

} // namespace cx::crystal