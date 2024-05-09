#include "fragment.h"
#include <occ/core/util.h>
#include <cmath>

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

bool FragmentDimer::sameAsymmetricFragmentIndices(const FragmentDimer &rhs) const {
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
    if (!sameAsymmetricFragmentIndices(rhs))
	return false;
    constexpr double eps = 1e-7;
    double centroid_diff = std::abs(centroidDistance - rhs.centroidDistance);
    if (centroid_diff > eps)
	return false;
    double com_diff = std::abs(centerOfMassDistance - rhs.centerOfMassDistance);
    if (com_diff > eps)
	return false;
    double nearest_diff = std::abs(nearestAtomDistance - rhs.nearestAtomDistance);
    if (nearest_diff > eps)
	return false;
    bool aa_eq = a.isEquivalentTo(rhs.a);
    bool bb_eq = b.isEquivalentTo(rhs.b);
    if (aa_eq && bb_eq)
	return true;
    bool ba_eq = b.isEquivalentTo(rhs.a);
    bool ab_eq = a.isEquivalentTo(rhs.b);
    return ab_eq && ba_eq;
}


QDebug operator<<(QDebug debug, const Fragment& fragment) {
    debug.nospace() << "Fragment {\n";
    debug.nospace() << "  atomIndices: [";
    for (const auto& index : fragment.atomIndices) {
        debug.nospace() << index << ", ";
    }
    debug.nospace() << "]\n";
    
    debug.nospace() << "  _atomOffset: [";
    for (int offset : fragment._atomOffset) {
        debug.nospace() << offset << ", ";
    }
    debug.nospace() << "]\n";
    
    debug.nospace() << "  atomicNumbers: [";
    for(int i = 0; i < fragment.atomicNumbers.rows(); i++) {
	debug.nospace() << fragment.atomicNumbers(i) << ", ";
    }
    debug.nospace() << "]\n";

    debug.nospace() << "  positions: [\n";
    for(int i = 0; i < fragment.positions.cols(); i++) {
	debug.nospace() << "[" << fragment.positions(0, i) << ", "
			       << fragment.positions(1, i) << ", "
			       << fragment.positions(2, i) << "]\n";
    }
    debug.nospace() << "]\n";
    debug.nospace() << "  asymmetricFragmentIndex: " << fragment.asymmetricFragmentIndex << "\n";
    debug.nospace() << "  size: " << fragment.size() << "\n";
    debug.nospace() << "}";
    
    return debug;
}
