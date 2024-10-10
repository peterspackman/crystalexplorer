#pragma once
#include "json.h"
#include <QMatrix4x4>

class Orientation {
public:
  struct EulerAngles {
    float x{0.0};
    float y{0.0};
    float z{0.0};
  };
  Orientation();
  Orientation(const Orientation &);
  Orientation(const QMatrix4x4 &);
  Orientation(float, float, float, float);
  void setTransformationMatrix(const QMatrix4x4 &);
  void setXRotation(float);
  void setYRotation(float);
  void setZRotation(float);
  void setScale(float);
  void setRotation(float, float, float);
  void rotate(const QQuaternion &);
  EulerAngles eulerAngles() const;
  float xRotation() const;
  float yRotation() const;
  float zRotation() const;
  float scale() const;
  QMatrix3x3 rotationMatrix() const;
  QMatrix4x4 viewMatrix() const;
  QMatrix4x4 modelMatrix() const;
  const QMatrix4x4 &modelViewMatrix() const { return m_transformationMatrix; }
  const QMatrix4x4 modelViewMatrixInverse() const {
    return m_transformationMatrix.inverted();
  }
  const QMatrix4x4 &transformationMatrix() const {
    return m_transformationMatrix;
  }

private:
  QMatrix4x4 m_transformationMatrix;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &, const Orientation &);
QDataStream &operator>>(QDataStream &, Orientation &);

void to_json(nlohmann::json &, const Orientation &);
void from_json(const nlohmann::json &, Orientation &);
