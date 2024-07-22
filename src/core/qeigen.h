#pragma once
#include <Eigen/Core>
#include <Eigen/Dense>
#include <QDataStream>
#include <QtGlobal>
//// Import most common Eigen types

// Define Eigen types compatible with Qt

typedef Eigen::Matrix<double, 2, 1> Vector2q;
typedef Eigen::Matrix<double, 3, 1> Vector3q;
typedef Eigen::Matrix<double, 3, 3> Matrix3q;
typedef Eigen::Matrix<double, 4, 4> Matrix4q;
typedef Eigen::Matrix<double, Eigen::Dynamic, 1> VectorXq;
typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> MatrixXq;

using Mat3Xd = Eigen::Matrix<double, 3, Eigen::Dynamic>;
using Mat3N = Eigen::Matrix<double, 3, Eigen::Dynamic>;

using Mat3Xf = Eigen::Matrix<float, 3, Eigen::Dynamic>;

using Vecf = Eigen::Vector<float, Eigen::Dynamic>;
using IVec = Eigen::Vector<int, Eigen::Dynamic>;
using Vec = Eigen::Vector<double, Eigen::Dynamic>;
using Mat3Xi = Eigen::Matrix<int, 3, Eigen::Dynamic>;

using MatRef3N = Eigen::Ref<Eigen::Matrix<double, 3, Eigen::Dynamic>>;
using IVecRef = Eigen::Ref<Eigen::Vector<int, Eigen::Dynamic>>;
using ConstMatRef3N =
    Eigen::Ref<const Eigen::Matrix<double, 3, Eigen::Dynamic>>;
using ConstIVecRef = Eigen::Ref<const Eigen::Vector<int, Eigen::Dynamic>>;

// Taken from
// https://github.com/biometrics/openbr/blob/master/openbr/core/eigenutils.h
template <typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows,
          int _MaxCols>
inline QDataStream &
operator<<(QDataStream &stream,
           const Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows,
                               _MaxCols> &mat) {
  int r = mat.rows();
  int c = mat.cols();
  stream << r << c;

  _Scalar *data = new _Scalar[r * c];
  for (int i = 0; i < r; i++)
    for (int j = 0; j < c; j++)
      data[i * c + j] = mat(i, j);
  int bytes = r * c * sizeof(_Scalar);
  int bytes_written = stream.writeRawData((const char *)data, bytes);
  if (bytes != bytes_written)
    qFatal("EigenUtils.h operator<< failure.");

  delete[] data;
  return stream;
}

template <typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows,
          int _MaxCols>
inline QDataStream &operator>>(
    QDataStream &stream,
    Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols> &mat) {
  int r, c;
  stream >> r >> c;
  mat.resize(r, c);

  _Scalar *data = new _Scalar[r * c];
  int bytes = r * c * sizeof(_Scalar);
  int bytes_read = stream.readRawData((char *)data, bytes);
  if (bytes != bytes_read)
    qFatal("EigenUtils.h operator>> failure.");
  for (int i = 0; i < r; i++)
    for (int j = 0; j < c; j++)
      mat(i, j) = data[i * c + j];

  delete[] data;
  return stream;
}

template <typename DerivedA, typename DerivedB>
bool all_close(const Eigen::DenseBase<DerivedA> &a,
               const Eigen::DenseBase<DerivedB> &b,
               const typename DerivedA::RealScalar &eps =
                   Eigen::NumTraits<typename DerivedA::RealScalar>::epsilon()) {
  return ((a.derived() - b.derived()).array().abs() <= eps).all();
}
