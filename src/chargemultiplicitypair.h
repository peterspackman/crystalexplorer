#pragma once

#include <QDataStream>

struct ChargeMultiplicityPair {
  int charge{0};
  int multiplicity{1};
};

inline QDataStream &operator<<(QDataStream &ds,
                               const ChargeMultiplicityPair &cm) {
  ds << cm.charge << cm.multiplicity;
  return ds;
}

inline QDataStream &operator>>(QDataStream &ds, ChargeMultiplicityPair &cm) {
  ds >> cm.charge;
  ds >> cm.multiplicity;
  return ds;
}
