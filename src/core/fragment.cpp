#include "fragment.h"
#include <cmath>
#include <occ/core/element.h>
#include <occ/core/util.h>

occ::Vec Fragment::interatomicDistances() const {
  // upper triangle of distance matrix
  size_t N = atomIndices.size();
  size_t num_idxs = N * (N - 1) / 2;
  occ::Vec result(num_idxs);
  size_t idx = 0;
  for (size_t i = 0; i < N; i++) {
    for (size_t j = i + 1; j < N; j++) {
      result(idx++) = (positions.col(i) - positions.col(j)).norm();
    }
  }
  return result;
}

bool Fragment::isComparableTo(const Fragment &other) const {
  if (atomIndices.size() != other.atomIndices.size())
    return false;
  return (atomicNumbers.array() == other.atomicNumbers.array()).all();
}

bool Fragment::isEquivalentTo(const Fragment &rhs) const {
  if (!isComparableTo(rhs))
    return false;
  auto dists_a = interatomicDistances();
  auto dists_b = rhs.interatomicDistances();
  return occ::util::all_close(dists_a, dists_b, 1e-8, 1e-8);
}

bool FragmentDimer::sameAsymmetricFragmentIndices(
    const FragmentDimer &rhs) const {
  bool same_idxs = false;
  const int a1_idx = a.asymmetricFragmentIndex;
  const int b1_idx = b.asymmetricFragmentIndex;
  const int a2_idx = rhs.a.asymmetricFragmentIndex;
  const int b2_idx = rhs.b.asymmetricFragmentIndex;

  if ((a1_idx < 0) || (b1_idx < 0) || (a2_idx < 0) || (b2_idx < 0))
    same_idxs = true;
  else {
    if ((a1_idx == a2_idx) && (b1_idx == b2_idx))
      same_idxs = true;
    else if ((a1_idx == b2_idx) && (a2_idx == b1_idx))
      same_idxs = true;
  }
  return same_idxs;
}

bool FragmentDimer::operator==(const FragmentDimer &rhs) const {
  if (!sameAsymmetricFragmentIndices(rhs)) {
    return false;
  }
  constexpr double eps = 1e-7;
  double centroid_diff = std::abs(centroidDistance - rhs.centroidDistance);
  if (centroid_diff > eps) {
    return false;
  }
  double com_diff = std::abs(centerOfMassDistance - rhs.centerOfMassDistance);
  if (com_diff > eps) {
    return false;
  }
  double nearest_diff = std::abs(nearestAtomDistance - rhs.nearestAtomDistance);
  if (nearest_diff > eps) {
    return false;
  }
  bool aa_eq = a.isEquivalentTo(rhs.a);
  bool bb_eq = b.isEquivalentTo(rhs.b);
  return (aa_eq && bb_eq);
  bool ba_eq = b.isEquivalentTo(rhs.a);
  bool ab_eq = a.isEquivalentTo(rhs.b);
  return ab_eq && ba_eq;
}

occ::Vec Fragment::atomicMasses() const {
  occ::Vec result(size());
  for (int i = 0; i < size(); i++) {
    result(i) =
        static_cast<double>(occ::core::Element(atomicNumbers(i)).mass());
  }
  return result;
}

occ::Vec3 Fragment::centroid() const { return positions.rowwise().mean(); }

occ::Vec3 Fragment::centerOfMass() const {
  occ::RowVec masses = atomicMasses();
  masses.array() /= masses.sum();
  return (positions.array().rowwise() * masses.array()).rowwise().sum();
}

QVector3D Fragment::posVector3D(int index) const {
  if (index < 0 || index >= positions.cols())
    return {};
  return QVector3D(positions(0, index), positions(1, index),
                   positions(2, index));
}

Fragment::NearestAtomResult Fragment::nearestAtom(const Fragment &other) const {
  Fragment::NearestAtomResult result{0, 0, std::numeric_limits<double>::max()};
  for (size_t i = 0; i < size(); i++) {
    const occ::Vec3 &p1 = positions.col(i);
    for (size_t j = 0; j < other.size(); j++) {
      const occ::Vec3 &p2 = other.positions.col(j);
      double d = (p2 - p1).norm();
      if (d < result.distance) {
        result = Fragment::NearestAtomResult{i, j, d};
      }
    }
  }
  return result;
}

Fragment::NearestAtomResult
Fragment::nearestAtomToPoint(const occ::Vec3 &point) const {
  Fragment::NearestAtomResult result{0, 0, std::numeric_limits<double>::max()};
  for (size_t i = 0; i < size(); i++) {
    const occ::Vec3 &p1 = positions.col(i);
    double d = (point - p1).norm();
    if (d < result.distance) {
      result = Fragment::NearestAtomResult{i, 0, d};
    }
  }
  return result;
}

FragmentDimer::FragmentDimer(const Fragment &fa, const Fragment &fb)
    : a(fa), b(fb) {
  nearestAtomDistance = fa.nearestAtom(fb).distance;
  centerOfMassDistance = (fb.centerOfMass() - fa.centerOfMass()).norm();
  centroidDistance = (fa.centroid() - fb.centroid()).norm();
}

QDebug operator<<(QDebug debug, const Fragment &fragment) {
  debug.nospace() << "Fragment {\n";
  debug.nospace() << "  atomIndices: [";
  for (const auto &index : fragment.atomIndices) {
    debug.nospace() << index << ", ";
  }
  debug.nospace() << "]\n";

  debug.nospace() << "  _atomOffset: [";
  for (int offset : fragment._atomOffset) {
    debug.nospace() << offset << ", ";
  }
  debug.nospace() << "]\n";

  debug.nospace() << "  atomicNumbers: [";
  for (int i = 0; i < fragment.atomicNumbers.rows(); i++) {
    debug.nospace() << fragment.atomicNumbers(i) << ", ";
  }
  debug.nospace() << "]\n";

  debug.nospace() << "  positions: [\n";
  for (int i = 0; i < fragment.positions.cols(); i++) {
    debug.nospace() << "[" << fragment.positions(0, i) << ", "
                    << fragment.positions(1, i) << ", "
                    << fragment.positions(2, i) << "]\n";
  }
  debug.nospace() << "]\n";
  debug.nospace() << "  asymmetricFragmentIndex: "
                  << fragment.asymmetricFragmentIndex << "\n";
  debug.nospace() << "  size: " << fragment.size() << "\n";
  debug.nospace() << "}";

  return debug;
}

QDebug operator<<(QDebug debug, const FragmentDimer &dimer) {
  debug.nospace() << "FragmentDimer {n=" << dimer.nearestAtomDistance
                  << ",c=" << dimer.centroidDistance
                  << ",m=" << dimer.centerOfMassDistance << "}";
  return debug;
}
