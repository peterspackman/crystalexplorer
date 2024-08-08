#include "adp.h"

AtomicDisplacementParameters::AtomicDisplacementParameters() {}

AtomicDisplacementParameters::AtomicDisplacementParameters(double U11, double U22, double U33, 
                                     double U12, double U13, double U23)
        : u11(U11), u22(U22), u33(U33), u12(U12), u13(U13), u23(U23) {}

AtomicDisplacementParameters::AtomicDisplacementParameters(double u) : u11(u), u22(u), u33(u) {}

occ::Vec6 AtomicDisplacementParameters::toBij() const {
    constexpr double factor = 8.0 * M_PI * M_PI;
    return {u11 * factor, u22 * factor, u33 * factor, 
            u12 * factor, u13 * factor, u23 * factor};
}

bool AtomicDisplacementParameters::isIsotropic(double tolerance) const {
    return std::abs(u11 - u22) < tolerance &&
           std::abs(u11 - u33) < tolerance &&
           std::abs(u12) < tolerance &&
           std::abs(u13) < tolerance &&
           std::abs(u23) < tolerance;
}

double AtomicDisplacementParameters::toUeq() const {
    return (u11 + u22 + u33) / 3.0;
}

