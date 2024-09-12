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

QString Parameters::deriveName() const {
  return fragmentDimer.getName();
}

int Parameters::charge() const {
  // TODO
  return 0;
}

int Parameters::multiplicity() const {
  // TODO
  return 1;
}

bool Parameters::isXtbModel() const { return xtb::isXtbMethod(model); }

} // namespace pair_energy
