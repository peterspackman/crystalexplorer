#pragma once
#include "generic_atom_index.h"
#include <occ/core/linear_algebra.h>

using Transform = Eigen::Isometry3d;
struct Fragment {
    std::vector<GenericAtomIndex> atomIndices;
    std::vector<int> _atomOffset;

    occ::IVec atomicNumbers;
    occ::Mat3N positions;

    int asymmetricFragmentIndex{-1};
    Transform asymmetricFragmentTransform = Transform::Identity();

    occ::IVec asymmetricUnitIndices;

    bool sameAsymmetricIndices(const Fragment &) const;
    bool isEquivalentTo(const Fragment &) const;
    bool isComparableTo(const Fragment &) const;
    occ::Vec interatomicDistances() const;
    inline size_t size() const { return atomIndices.size(); }
};

struct FragmentPair {
  Fragment a;
  Fragment b;

  double centroidDistance{0.0};
  double nearestAtomDistance{0.0};
  double centerOfMassDistance{0.0};

  bool sameAsymmetricFragmentIndices(const FragmentPair &) const;
  bool operator==(const FragmentPair &) const;
};

