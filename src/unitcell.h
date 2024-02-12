#pragma once
#include "qeigen.h"
#include <QGenericMatrix>
#include <QVector3D>

class UnitCell {
  friend QDataStream &operator<<(QDataStream &, const UnitCell &);
  friend QDataStream &operator>>(QDataStream &, UnitCell &);

public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  UnitCell();
  UnitCell(float, float, float, float, float, float);
  float volume() const { return _volume; }
  float a() const { return _a; }
  float b() const { return _b; }
  float c() const { return _c; }
  float alpha() const { return _alpha; }
  float beta() const { return _beta; }
  float gamma() const { return _gamma; }
  const Vector3q &aAxis() const { return _aAxis; }
  const Vector3q &bAxis() const { return _bAxis; }
  const Vector3q &cAxis() const { return _cAxis; }
  QVector3D aVector() const {
    return QVector3D(_aAxis(0), _aAxis(1), _aAxis(2));
  }
  QVector3D bVector() const {
    return QVector3D(_bAxis(0), _bAxis(1), _bAxis(2));
  }
  QVector3D cVector() const {
    return QVector3D(_cAxis(0), _cAxis(1), _cAxis(2));
  }
  const Matrix3q &directCellMatrix() const { return _directCellMatrix; }
  const Matrix3q &inverseCellMatrix() const { return _inverseCellMatrix; }
  inline const auto reciprocalMatrix() const {
    return _inverseCellMatrix.transpose();
  }

private:
  void init();
  float _a, _b, _c, _alpha, _beta, _gamma;
  float _volume;

  Vector3q _aAxis, _bAxis, _cAxis;
  Matrix3q _directCellMatrix, _inverseCellMatrix;
};

QDataStream &operator<<(QDataStream &, const UnitCell &);
QDataStream &operator>>(QDataStream &, UnitCell &);
