#include "pair_energy_parameters.h"
#include "chemicalstructure.h"
#include "xtb_parameters.h"

namespace pair_energy {

bool EnergyModelParameters::operator==(const EnergyModelParameters &rhs) const {
  if (model != rhs.model)
    return false;
  return true;
}

bool EnergyModelParameters::isXtbModel() const {
  return xtb::isXtbMethod(model);
}

bool Parameters::operator==(const Parameters &rhs) const {
  if (model != rhs.model)
    return false;
  if (atomsA != rhs.atomsA)
    return false;
  if (atomsB != rhs.atomsB)
    return false;
  if (transformA.matrix() != rhs.transformA.matrix())
    return false;
  if (transformB.matrix() != rhs.transformB.matrix())
    return false;
  if (wfnA != rhs.wfnA)
    return false;
  if (wfnB != rhs.wfnB)
    return false;
  return true;
}

QString Parameters::deriveName() const { return fragmentDimer.getName(); }

int Parameters::charge() const {
  // Calculate total charge from both fragments
  return fragmentDimer.a.state.charge + fragmentDimer.b.state.charge;
}

int Parameters::multiplicity() const {
  // For a dimer, multiplicity calculation depends on the individual fragment multiplicities
  // and how their unpaired electrons couple.
  // Number of unpaired electrons = multiplicity - 1
  int unpairedA = fragmentDimer.a.state.multiplicity - 1;
  int unpairedB = fragmentDimer.b.state.multiplicity - 1;

  // Total unpaired electrons (assuming high-spin coupling)
  // For most non-covalent interactions, fragments don't share electrons,
  // so we add unpaired electrons
  int totalUnpaired = unpairedA + unpairedB;

  return totalUnpaired + 1;
}

bool Parameters::isXtbModel() const { return xtb::isXtbMethod(model); }

} // namespace pair_energy

void to_json(nlohmann::json &j, const pair_energy::Parameters &p) {
  j["model"] = p.model;
  j["atomsA"] = p.atomsA;
  j["atomsB"] = p.atomsB;
  j["transformA"] = p.transformA.matrix();
  j["transformB"] = p.transformA.matrix();
  j["fragmentDimer"] = p.fragmentDimer;
}

void from_json(const nlohmann::json &j, pair_energy::Parameters &p) {
  j.at("model").get_to(p.model);
  j.at("atomsA").get_to(p.atomsA);
  j.at("atomsB").get_to(p.atomsB);
  p.transformA = j.at("transformA").get<occ::Mat4>();
  p.transformB = j.at("transformB").get<occ::Mat4>();
  j.at("fragmentDimer").get_to(p.fragmentDimer);
}
