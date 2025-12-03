#include <occ/interaction/interaction_json.h>
#include <fstream>
#include <stdexcept>

namespace occ::interaction {

ElatResults read_elat_json(const std::string& filename) {
  // Load JSON file
  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open JSON file: " + filename);
  }

  nlohmann::json j;
  file >> j;

  // Validate JSON format
  if (j["result_type"] != "elat") {
    throw std::runtime_error("Invalid JSON: not an elat result file");
  }

  if (!j.contains("pairs")) {
    throw std::runtime_error("Need 'pairs' in JSON output.");
  }

  if (!j.contains("crystal")) {
    throw std::runtime_error("Need 'crystal' in JSON output.");
  }

  // Extract basic info
  occ::crystal::Crystal crystal = j["crystal"];
  std::string title = j.value("title", "");
  std::string model = j.value("model", "");

  // Extract pairs data and build energy mapping
  const auto& pairs_json = j["pairs"];

  ankerl::unordered_dense::map<int, CEEnergyComponents> energy_components;
  ankerl::unordered_dense::map<int, double> total_energies;

  for (size_t mol_idx = 0; mol_idx < pairs_json.size(); mol_idx++) {
    for (const auto& pair_data : pairs_json[mol_idx]) {
      int unique_idx = pair_data["Unique Index"];

      if (pair_data.contains("energies")) {
        const auto& energies_json = pair_data["energies"];

        CEEnergyComponents components;
        if (energies_json.contains("Coulomb")) components.coulomb = energies_json["Coulomb"];
        if (energies_json.contains("Exchange")) components.exchange = energies_json["Exchange"];
        if (energies_json.contains("Repulsion")) components.repulsion = energies_json["Repulsion"];
        if (energies_json.contains("Dispersion")) components.dispersion = energies_json["Dispersion"];
        if (energies_json.contains("Polarization")) components.polarization = energies_json["Polarization"];
        if (energies_json.contains("Total")) components.total = energies_json["Total"];

        energy_components[unique_idx] = components;
        total_energies[unique_idx] = components.total;
      }
    }
  }

  // Generate CrystalDimers from crystal using appropriate radius
  double radius = 15.0; // Default radius
  if (j.contains("radius")) {
    radius = j["radius"];
  }

  occ::crystal::CrystalDimers dimers = crystal.symmetry_unique_dimers(radius);

  // Map energies onto the computed dimers
  for (size_t i = 0; i < dimers.unique_dimers.size(); i++) {
    auto it = total_energies.find(i);
    if (it != total_energies.end()) {
      dimers.unique_dimers[i].set_interaction_energy(it->second);

      // Set detailed energy components if available
      auto comp_it = energy_components.find(i);
      if (comp_it != energy_components.end()) {
        ankerl::unordered_dense::map<std::string, double> energy_map;
        energy_map["Coulomb"] = comp_it->second.coulomb;
        energy_map["Exchange"] = comp_it->second.exchange;
        energy_map["Repulsion"] = comp_it->second.repulsion;
        energy_map["Dispersion"] = comp_it->second.dispersion;
        energy_map["Polarization"] = comp_it->second.polarization;
        energy_map["Total"] = comp_it->second.total;
        dimers.unique_dimers[i].set_interaction_energies(energy_map);
      }
    }
  }

  // Build LatticeEnergyResult
  LatticeEnergyResult lattice_result;
  lattice_result.dimers = std::move(dimers);
  lattice_result.energy_components.resize(lattice_result.dimers.unique_dimers.size());

  for (size_t i = 0; i < lattice_result.energy_components.size(); i++) {
    auto comp_it = energy_components.find(i);
    if (comp_it != energy_components.end()) {
      lattice_result.energy_components[i] = comp_it->second;
      lattice_result.energy_components[i].is_computed = true;
    }
  }

  // Calculate total lattice energy if available
  if (j.contains("lattice_energy")) {
    lattice_result.lattice_energy = j["lattice_energy"];
  }

  return ElatResults{
    std::move(crystal),
    std::move(lattice_result),
    title,
    model
  };
}

} // namespace occ::interaction
