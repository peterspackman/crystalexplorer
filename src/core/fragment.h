#pragma once
#include "fragment_index.h"
#include "generic_atom_index.h"
#include <QColor>
#include <QDebug>
#include <QVector3D>
#include <occ/core/linear_algebra.h>

using Transform = Eigen::Isometry3d;

struct DimerAtoms {
  std::vector<GenericAtomIndex> a;
  std::vector<GenericAtomIndex> b;
};

struct FragmentColorSettings {
  enum class Method {
    UnitCellFragment,
    SymmetryUniqueFragment,
    Constant
  };

  Method method{Method::SymmetryUniqueFragment};
  QColor color{Qt::gray};
};

struct Fragment {

  struct NearestAtomResult {
    size_t idx_this;
    size_t idx_other;
    double distance;
  };

  struct State {
    int charge{0};
    int multiplicity{1};
  };

  std::vector<GenericAtomIndex> atomIndices;

  occ::IVec atomicNumbers;
  occ::Mat3N positions;

  FragmentIndex asymmetricFragmentIndex{-1};
  Transform asymmetricFragmentTransform = Transform::Identity();

  FragmentIndex index;
  State state;

  occ::IVec asymmetricUnitIndices;
  QColor color{Qt::gray};

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
  explicit FragmentDimer() {};
  explicit FragmentDimer(const Fragment &, const Fragment &);

  Fragment a;
  Fragment b;
  double centroidDistance{0.0};
  double nearestAtomDistance{0.0};
  double centerOfMassDistance{0.0};
  int nearestAtomIndexA{-1};
  int nearestAtomIndexB{-1};
  QString symmetry{"-"};
  FragmentIndexPair index;

  bool sameAsymmetricFragmentIndices(const FragmentDimer &) const;
  bool operator==(const FragmentDimer &) const;
};

QDebug operator<<(QDebug debug, const Fragment &fragment);
QDebug operator<<(QDebug debug, const FragmentDimer &dimer);
