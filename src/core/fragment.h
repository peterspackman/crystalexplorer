#pragma once
#include "generic_atom_index.h"
#include <QDebug>
#include <occ/core/linear_algebra.h>
#include <QVector3D>

using Transform = Eigen::Isometry3d;
struct Fragment {

  struct NearestAtomResult {
    size_t idx_this;
    size_t idx_other;
    double distance;
  };

  std::vector<GenericAtomIndex> atomIndices;
  std::vector<int> _atomOffset;

  occ::IVec atomicNumbers;
  occ::Mat3N positions;

  int asymmetricFragmentIndex{-1};
  Transform asymmetricFragmentTransform = Transform::Identity();

  occ::IVec asymmetricUnitIndices;

  NearestAtomResult nearestAtom(const Fragment &other) const;
  NearestAtomResult nearestAtomToPoint(const occ::Vec3 &point) const;

  QVector3D posVector3D(int index) const;

  occ::Vec atomicMasses() const;
  occ::Vec3 centroid() const;
  occ::Vec3 centerOfMass() const;

  bool sameAsymmetricIndices(const Fragment &) const;
  bool isEquivalentTo(const Fragment &) const;
  bool isComparableTo(const Fragment &) const;
  occ::Vec interatomicDistances() const;
  inline size_t size() const { return atomIndices.size(); }
};

struct FragmentDimer {
  FragmentDimer(const Fragment &, const Fragment &);

  Fragment a;
  Fragment b;

  double centroidDistance{0.0};
  double nearestAtomDistance{0.0};
  double centerOfMassDistance{0.0};

  bool sameAsymmetricFragmentIndices(const FragmentDimer &) const;
  bool operator==(const FragmentDimer &) const;
};

QDebug operator<<(QDebug debug, const Fragment &fragment);
