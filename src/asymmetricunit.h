#pragma once
#include "qeigen.h"
#include <QList>
#include <QString>

struct AsymmetricUnit {
  AsymmetricUnit();

  AsymmetricUnit(ConstMatRef3N positions, ConstIVecRef atomicNumbers);

  Mat3N positions;
  IVec atomicNumbers;
  Vec occupations;
  Vec charges;
  QList<QString> labels;

  size_t size() const { return atomicNumbers.rows(); }
};
