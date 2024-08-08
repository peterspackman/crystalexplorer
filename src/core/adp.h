#pragma once
#include <occ/core/linear_algebra.h>

struct AtomicDisplacementParameters {
    double u11{0.0};
    double u22{0.0};
    double u33{0.0};
    double u12{0.0};
    double u13{0.0};
    double u23{0.0};

    AtomicDisplacementParameters();
    AtomicDisplacementParameters(double u11, double u22, double u33,
                                     double u12, double u13, double u23);
    AtomicDisplacementParameters(double u);

    occ::Vec6 toBij() const;
    bool isIsotropic(double tolerance = 1e-6) const;
    double toUeq() const;
};
