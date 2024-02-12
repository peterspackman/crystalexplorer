#include "orientation.h"

Orientation::Orientation() { m_transformationMatrix.setToIdentity(); }

Orientation::Orientation(const Orientation &o) {
  setTransformationMatrix(o.transformationMatrix());
}

Orientation::Orientation(float scale, float x, float y, float z) {
  m_transformationMatrix.setToIdentity();
  m_transformationMatrix.rotate(x, y, z);
  m_transformationMatrix.scale(scale);
}

Orientation::Orientation(const QMatrix4x4 &mat) {
  setTransformationMatrix(mat);
}

void Orientation::setTransformationMatrix(const QMatrix4x4 &mat) {
  m_transformationMatrix = mat;
}

float Orientation::scale() const {
  float s =
      QVector3D(m_transformationMatrix(0, 0), m_transformationMatrix(0, 1),
                m_transformationMatrix(0, 2))
          .length();
  return s;
}

Orientation::EulerAngles Orientation::eulerAngles() const {
  EulerAngles result;

  auto q = QQuaternion::fromRotationMatrix(rotationMatrix());
  q.getEulerAngles(&result.x, &result.y, &result.z);
  return result;
}

QMatrix4x4 Orientation::viewMatrix() const {
  QMatrix4x4 view(m_transformationMatrix);
  view *= 1.0f / scale();
  view.setColumn(3, {0, 0, 0, 1});
  return view;
}

QMatrix4x4 Orientation::modelMatrix() const {
  float s = scale();
  QMatrix4x4 model(s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0, 0, 0, 0, 1);
  return model;
}

QMatrix3x3 Orientation::rotationMatrix() const {
  QMatrix3x3 rot(m_transformationMatrix.toGenericMatrix<3, 3>());
  rot *= 1.0f / scale();
  return rot;
}

float Orientation::xRotation() const { return eulerAngles().x; }

float Orientation::yRotation() const { return eulerAngles().y; }

float Orientation::zRotation() const { return eulerAngles().z; }

void Orientation::setXRotation(float x) {
  auto q = QQuaternion::fromAxisAndAngle({1.0, 0.0, 0.0}, x - xRotation());
  m_transformationMatrix.rotate(q);
}

void Orientation::setYRotation(float y) {
  auto q = QQuaternion::fromAxisAndAngle({0.0, 1.0, 0.0}, y - yRotation());
  m_transformationMatrix.rotate(q);
}

void Orientation::setZRotation(float z) {
  auto q = QQuaternion::fromAxisAndAngle({0.0, 0.0, 1.0}, z - zRotation());
  m_transformationMatrix.rotate(q);
}

void Orientation::setScale(float s) {
  m_transformationMatrix.scale(s / scale());
}

void Orientation::rotate(const QQuaternion &q) {
  m_transformationMatrix.rotate(q);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &ds, const Orientation &orient) {
  ds << orient.transformationMatrix();
  return ds;
}

QDataStream &operator>>(QDataStream &ds, Orientation &orient) {
  QMatrix4x4 dest;
  ds >> dest;
  orient.setTransformationMatrix(dest);
  return ds;
}
