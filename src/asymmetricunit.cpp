#include "asymmetricunit.h"

AsymmetricUnit::AsymmetricUnit() {}

AsymmetricUnit::AsymmetricUnit(ConstMatRef3N pos, ConstIVecRef nums)
    : positions(pos), atomicNumbers(nums) {}
