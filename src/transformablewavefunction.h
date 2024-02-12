#pragma once
#include "qeigen.h"
#include <QPair>

class Wavefunction; // forward declaration

typedef QPair<Matrix3q, Vector3q> WavefunctionTransform;
typedef QPair<Wavefunction, WavefunctionTransform> TransformableWavefunction;

inline QDataStream &operator<<(QDataStream &ds,
                               const WavefunctionTransform &wt) {
  ds << wt.first;
  ds << wt.second;
  return ds;
}

inline QDataStream &operator>>(QDataStream &ds, WavefunctionTransform &wt) {
  ds >> wt.first;
  ds >> wt.second;
  return ds;
}
