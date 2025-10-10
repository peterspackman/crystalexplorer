#pragma once
#include <ankerl/unordered_dense.h>

namespace occ::crystal {

/**
 * \brief Standard hydrogen bond lengths for various elements
 *
 * This class provides standard X-H bond lengths commonly used for
 * normalizing hydrogen positions in crystal structures, particularly
 * those determined by X-ray diffraction where hydrogen positions are
 * less accurate.
 *
 * Bond lengths from: Allen, F. H. Acta Cryst. (2010). B66, 380–386
 */
class StandardBondLengths {
public:
  // Standard bond lengths in Angstroms from Allen (2010)
  static constexpr double C_H = 1.083;   // C-H bond
  static constexpr double N_H = 1.009;   // N-H bond
  static constexpr double O_H = 0.983;   // O-H bond
  static constexpr double B_H = 1.180;   // B-H bond

  /**
   * \brief Get standard hydrogen bond length for a given element
   *
   * \param atomic_number The atomic number of the heavy atom bonded to hydrogen
   * \return The standard bond length in Angstroms, or -1.0 if not available
   */
  static double get_hydrogen_bond_length(int atomic_number) {
    switch (atomic_number) {
      case 5:  return B_H;  // Boron
      case 6:  return C_H;  // Carbon
      case 7:  return N_H;  // Nitrogen
      case 8:  return O_H;  // Oxygen
      default: return -1.0;
    }
  }

  /**
   * \brief Check if a standard bond length is available for an element
   *
   * \param atomic_number The atomic number of the heavy atom
   * \return True if a standard bond length is defined
   */
  static bool has_standard_length(int atomic_number) {
    return get_hydrogen_bond_length(atomic_number) > 0;
  }
};

} // namespace occ::crystal
