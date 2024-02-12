#pragma once
#include <QDataStream>

struct VanDerWaalsContact {
  int from{-1}, to{-1};
  float distance{0.0};
  float vdw_sum{0.0};
  bool is_intramolecular{false};
};

inline QDataStream &operator<<(QDataStream &ds, const VanDerWaalsContact &vdw) {
  ds << vdw.from << vdw.to << vdw.distance << vdw.vdw_sum
     << vdw.is_intramolecular;
  return ds;
}

inline QDataStream &operator>>(QDataStream &ds, VanDerWaalsContact &vdw) {
  ds >> vdw.from >> vdw.to >> vdw.distance >> vdw.vdw_sum >>
      vdw.is_intramolecular;
  return ds;
}
