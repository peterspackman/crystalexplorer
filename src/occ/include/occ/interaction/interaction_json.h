#pragma once
#include <nlohmann/json.hpp>
#include <occ/crystal/crystal.h>
#include <occ/core/dimer.h>
#include <string>
#include <vector>

namespace occ::interaction {

// Energy components structure
struct CEEnergyComponents {
  double coulomb{0.0};
  double exchange{0.0};
  double repulsion{0.0};
  double dispersion{0.0};
  double polarization{0.0};
  double total{0.0};
  bool is_computed{false};
};

// Lattice energy result structure
struct LatticeEnergyResult {
  double lattice_energy{0.0};
  occ::crystal::CrystalDimers dimers;
  std::vector<CEEnergyComponents> energy_components;
};

// Complete elat results structure
struct ElatResults {
    occ::crystal::Crystal crystal;
    LatticeEnergyResult lattice_energy_result;
    std::string title;
    std::string model;
};

/**
 * \brief Write elat JSON results in compact format
 *
 * Writes crystal, dimers, and energy data to JSON file
 *
 * \param filename Output filename
 * \param results ElatResults to write
 */
void write_elat_json(const std::string& filename, const ElatResults& results);

/**
 * \brief Read elat JSON results and reconstruct data structures
 *
 * Loads crystal and reconstructs LatticeEnergyResult with energies
 * mapped from the compact JSON format produced by write_elat_json()
 *
 * \param filename Path to elat JSON results file
 * \return ElatResults containing crystal and lattice energy data
 */
ElatResults read_elat_json(const std::string& filename);

} // namespace occ::interaction
