#include "pair_energy_parameters.h"

namespace pair_energy {
bool Parameters::operator==(const Parameters& rhs) const {
    if (model != rhs.model) return false;
    if (atomsA != rhs.atomsA) return false;
    if (atomsB != rhs.atomsB) return false;
    if (transformA.matrix() != rhs.transformA.matrix()) return false;
    if (transformB.matrix() != rhs.transformB.matrix()) return false;
    if (wfnA != rhs.wfnA) return false;
    if (wfnB != rhs.wfnB) return false;
    return true;
}

}
