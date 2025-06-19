#pragma once
#include <QColor>
#include <QHashFunctions>
#include <QVector2D>

struct MillerIndex {
  int h{0};
  int k{0};
  int l{0};

  inline bool operator==(const MillerIndex &other) const {
    return h == other.h && k == other.k && l == other.l;
  }
  inline bool operator!=(const MillerIndex &other) const {
    return !(*this == other);
  }

  inline bool isZero() const { return h == 0 && k == 0 && l == 0; }
};

struct PlaneVisualizationOptions {
  bool useInfinitePlanes{false};
  bool showGrid{true};
  bool showUnitCellIntersection{true};
  double gridSpacing{1.0};
  QVector2D repeatRangeA{-2, 2};  // Repeat range in first plane direction
  QVector2D repeatRangeB{-2, 2};  // Repeat range in second plane direction
};

struct CrystalPlane {
  MillerIndex hkl;
  double offset{0.0};
  QColor color;
  inline bool operator==(const CrystalPlane &other) const {
    return hkl == other.hkl && qFuzzyCompare(offset, other.offset);
  }

  inline bool operator!=(const CrystalPlane &other) const {
    return !(*this == other);
  }
};

inline uint qHash(const CrystalPlane &plane, uint seed = 0) {
  // Qt 5.15 and later
  return qHash(plane.hkl.h, seed) ^ qHash(plane.hkl.k, seed) ^
         qHash(plane.hkl.l, seed) ^ qHash(plane.offset, seed);
}
