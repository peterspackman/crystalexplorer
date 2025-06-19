#pragma once

#include "crystalstructure.h"
#include "slabstructure.h"
#include <occ/crystal/surface.h>
#include <vector>

namespace cx::crystal {

/**
 * \brief Generate a surface cut from a crystal structure
 * \param crystalStructure The source crystal structure
 * \param h Miller index h
 * \param k Miller index k  
 * \param l Miller index l
 * \param cutOffset Offset along the surface normal (0.0 to 1.0)
 * \param thickness Slab thickness in Angstroms (0 = auto from depth scale)
 * \return New SlabStructure containing the surface cut, or nullptr on failure
 */
SlabStructure* generateSurfaceCut(
  CrystalStructure *crystalStructure,
  int h, int k, int l, 
  double cutOffset = 0.0,
  double thickness = 5.0
);

/**
 * \brief Get suggested cut positions for a given Miller plane
 * \param crystalStructure The source crystal structure
 * \param h Miller index h
 * \param k Miller index k
 * \param l Miller index l
 * \return List of suggested cut offsets
 */
std::vector<double> getSuggestedCuts(
  CrystalStructure *crystalStructure,
  int h, int k, int l
);

/**
 * \brief Generate multiple surface cuts at suggested positions
 * \param crystalStructure The source crystal structure
 * \param h Miller index h
 * \param k Miller index k
 * \param l Miller index l
 * \param thickness Slab thickness in Angstroms (0 = auto)
 * \return List of new SlabStructures containing surface cuts
 */
QList<SlabStructure*> generateSuggestedSurfaceCuts(
  CrystalStructure *crystalStructure,
  int h, int k, int l,
  double thickness = 5.0
);

// No detail namespace needed - SlabStructure handles the conversion internally

} // namespace cx::crystal