#include "adp.h"

// Owen's Handbook formula
inline double sphericalCDF(double B) {
  // univariate normal pdf and cdf
  auto pdf = [](double x) {
    return std::exp(-0.5 * x * x) / std::sqrt(2.0 * M_PI);
  };
  auto cdf = [](double x) {
    return 0.5 * (1.0 + std::erf(x / std::sqrt(2.0)));
  };
  return 2.0 * cdf(B) - 2.0 * B * pdf(B) - 1.0;
}

inline double findScaleFactorForProbability(double target,
                                            double tolerance = 1e-8) {
  double low = 0.0, high = 10.0;
  while (high - low > tolerance) {
    double mid = (low + high) / 2.0;
    double prob = sphericalCDF(mid);
    if (prob < target) {
      low = mid;
    } else {
      high = mid;
    }
  }
  return (low + high) / 2.0;
}

AtomicDisplacementParameters::AtomicDisplacementParameters() { initialize(); }

AtomicDisplacementParameters::AtomicDisplacementParameters(
    double U11, double U22, double U33, double U12, double U13, double U23)
    : u11(U11), u22(U22), u33(U33), u12(U12), u13(U13), u23(U23) {
  initialize();
}

AtomicDisplacementParameters::AtomicDisplacementParameters(double u)
    : u11(u), u22(u), u33(u) {
  initialize();
}

occ::Vec6 AtomicDisplacementParameters::toBij() const {
  constexpr double factor = 8.0 * M_PI * M_PI;
  return {u11 * factor, u22 * factor, u33 * factor,
          u12 * factor, u13 * factor, u23 * factor};
}

bool AtomicDisplacementParameters::isIsotropic(double tolerance) const {
  return std::abs(u11 - u22) < tolerance && std::abs(u11 - u33) < tolerance &&
         std::abs(u12) < tolerance && std::abs(u13) < tolerance &&
         std::abs(u23) < tolerance;
}

double AtomicDisplacementParameters::toUeq() const {
  return (u11 + u22 + u33) / 3.0;
}

QMatrix3x3
AtomicDisplacementParameters::thermalEllipsoidMatrix(double scaleFactor) const {
  occ::Vec3 a = scaleFactor * amplitudes;
  QMatrix3x3 result;
  result(0, 0) = a(0) * rotations(0, 0);
  result(1, 0) = a(0) * rotations(0, 1);
  result(2, 0) = a(0) * rotations(0, 2);

  result(0, 1) = a(1) * rotations(1, 0);
  result(1, 1) = a(1) * rotations(1, 1);
  result(2, 1) = a(1) * rotations(1, 2);

  result(0, 2) = a(2) * rotations(2, 0);
  result(1, 2) = a(2) * rotations(2, 1);
  result(2, 2) = a(2) * rotations(2, 2);
  return result;
}

QMatrix3x3 AtomicDisplacementParameters::thermalEllipsoidMatrixForProbability(
    double p) const {
  double scaleFactor = findScaleFactorForProbability(p);
  return thermalEllipsoidMatrix(scaleFactor);
}

bool AtomicDisplacementParameters::isZero() const {
  return occ::Vec6(u11, u22, u33, u12, u13, u23).isZero(1e-10);
}

void AtomicDisplacementParameters::initialize() {
  occ::Mat3 m;
  // clang-format off
  m << u11, u12, u13,
       u12, u22, u23,
       u13, u23, u33;
  // clang-format on

  if (m.isZero(1e-10)) {
    // Handle all-zero case
    amplitudes.setZero();
    rotations.setIdentity();
    return;
  }

  // Calculate eigenvalues.  Include "true" to calculate eigenvectors as well.
  Eigen::SelfAdjointEigenSolver<occ::Mat3> solver(m,
                                                  Eigen::ComputeEigenvectors);

  // Eigenvalues are proportional to square of vibrational amplitudes
  occ::Vec3 evals = solver.eigenvalues();

  // Eigenvectors are the rotation matrices.
  // It seems the correct rotation is the transpose.
  occ::Mat3 evecs = solver.eigenvectors().transpose();

  // Ensure we have a pure rotation
  float det = evecs.determinant();
  if (det < 0) {
    evecs.array() *= -1.0;
  }

  // Vibrational amplitudes = sqrt(evals);
  amplitudes = evals.array().abs().sqrt();
  rotations = evecs;
}
