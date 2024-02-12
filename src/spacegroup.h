#pragma once
#include <QVector>

#include "qeigen.h"

typedef int SymopId;
const SymopId NOSYMOP = -1;
const double SYMOP_STRING_TOL = 0.0001;

class SpaceGroup {
  friend QDataStream &operator<<(QDataStream &, const SpaceGroup &);
  friend QDataStream &operator>>(QDataStream &, SpaceGroup &);

public:
  SpaceGroup() { _HM_Symbol = QString(); }
  SpaceGroup(const QString &hms) { _HM_Symbol = hms; }
  const QString &symbol() const { return _HM_Symbol; }
  int numberOfSymops() const { return _seitzMatrices.size(); }
  void addSeitzMatrix(const Matrix4q &m) { _seitzMatrices.append(m); }
  void addInverseSymops(const QVector<int> &is) { _inverseSymops = is; }
  void addSymopProducts(const QVector<QVector<SymopId>> &sp) {
    _symopProducts = sp;
  }
  int symopProduct(const SymopId &s1, SymopId s2) const {
    return _symopProducts[s1][s2];
  }
  int inverseSymop(const SymopId &s) const { return _inverseSymops[s]; }
  Matrix3q rotationMatrixForSymop(const SymopId &) const;
  Vector3q translationForSymop(const SymopId &) const;
  QString symopAsString(const SymopId &) const;

protected:
private:
  QString rotationString(const QString &, double, const QString &) const;
  QString translationString(const QString &, double) const;
  bool equal(double, double) const;

  QString _HM_Symbol;
  QVector<Matrix4q> _seitzMatrices;
  QVector<int> _inverseSymops;
  QVector<QVector<int>> _symopProducts;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &, const SpaceGroup &);
QDataStream &operator>>(QDataStream &, SpaceGroup &);
