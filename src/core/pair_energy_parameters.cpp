#include "pair_energy_parameters.h"
#include "chemicalstructure.h"
#include <functional>

// Helper function to combine hash values
template <typename T> void hash_combine(std::size_t &seed, const T &value) {
  std::hash<T> hasher;
  seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Hash function for Eigen::Isometry3d
struct Isometry3dHash {
  std::size_t operator()(const Eigen::Isometry3d &isometry) const {
    std::size_t seed = 0;

    // Hash the translation vector
    const Eigen::Vector3d &translation = isometry.translation();
    hash_combine(seed, translation.x());
    hash_combine(seed, translation.y());
    hash_combine(seed, translation.z());

    // Hash the rotation matrix
    const Eigen::Matrix3d &rotation = isometry.rotation();
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        hash_combine(seed, rotation(i, j));
      }
    }

    return seed;
  }
};

namespace pair_energy {

bool EnergyModelParameters::operator==(const EnergyModelParameters &rhs) const {
  if (model != rhs.model)
    return false;
  return true;
}

bool EnergyModelParameters::isXtbModel() const {
    // TODO proper implementation
    return model == "GFN2-xTB";
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

  Isometry3dHash hasher;

  QString formulaA = "A";
  QString formulaB = "B";

  auto hashA = hasher(transformA);
  auto hashB = hasher(transformB);

  return QString("%1_%2_%3_%4")
      .arg(formulaA)
      .arg(hashA)
      .arg(formulaB)
      .arg(hashB);
}

int Parameters::charge() const {
    // TODO
    return 0;
}

int Parameters::multiplicity() const {
    // TODO
    return 1;
}

bool Parameters::isXtbModel() const {
    // TODO proper implementation
    return model == "GFN2-xTB";
}


} // namespace pair_energy
