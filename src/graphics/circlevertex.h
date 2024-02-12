#pragma once
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <vector>

using std::vector;

class CircleVertex {
public:
  // Constructors
  Q_DECL_CONSTEXPR explicit CircleVertex(
      const QVector3D &position, const QVector3D &right, const QVector3D &up,
      const QVector4D &color, const QVector2D &texcoord, float r1squared = 0.0,
      float max_angle = 0.0)
      : m_position(position), m_right(right), m_up(up), m_color(color),
        m_texcoord(texcoord), m_r1squared(r1squared), m_max_angle(max_angle) {}

  // Accessors / Mutators
  Q_DECL_CONSTEXPR const QVector3D &position() const { return m_position; }
  Q_DECL_CONSTEXPR const QVector3D &right() const { return m_right; }
  Q_DECL_CONSTEXPR const QVector3D &up() const { return m_up; }
  Q_DECL_CONSTEXPR const QVector4D &color() const { return m_color; }
  Q_DECL_CONSTEXPR const QVector2D &texcoord() const { return m_texcoord; }
  Q_DECL_CONSTEXPR float innerRadiusSquared() const { return m_r1squared; }
  Q_DECL_CONSTEXPR float maxAngle() const { return m_max_angle; }

  inline void setPosition(const QVector3D &position) { m_position = position; }
  inline void setRight(const QVector3D &right) { m_right = right; }
  inline void setUp(const QVector3D &up) { m_up = up; }
  inline void setColor(const QVector4D &color) { m_color = color; }
  inline void setTexCoord(const QVector2D &texcoord) { m_texcoord = texcoord; }
  inline void setInnerRadiusSquared(float r) { m_r1squared = r; }
  inline void setMaxAngle(float a) { m_max_angle = a; }

  // OpenGL Helpers
  static const int PositionTupleSize = 3;
  static const int RightTupleSize = 3;
  static const int UpTupleSize = 3;
  static const int ColorTupleSize = 4;
  static const int TexcoordTupleSize = 2;
  static const int InnerRadiusSize = 1;
  static const int MaxAngleSize = 1;

  static Q_DECL_CONSTEXPR int positionOffset() {
    return offsetof(CircleVertex, m_position);
  }
  static Q_DECL_CONSTEXPR int rightOffset() {
    return offsetof(CircleVertex, m_right);
  }
  static Q_DECL_CONSTEXPR int upOffset() {
    return offsetof(CircleVertex, m_up);
  }
  static Q_DECL_CONSTEXPR int colorOffset() {
    return offsetof(CircleVertex, m_color);
  }
  static Q_DECL_CONSTEXPR int texcoordOffset() {
    return offsetof(CircleVertex, m_texcoord);
  }
  static Q_DECL_CONSTEXPR int innerRadiusOffset() {
    return offsetof(CircleVertex, m_r1squared);
  }
  static Q_DECL_CONSTEXPR int maxAngleOffset() {
    return offsetof(CircleVertex, m_max_angle);
  }

  static Q_DECL_CONSTEXPR int stride() { return sizeof(CircleVertex); }

private:
  QVector3D m_position;
  QVector3D m_right;
  QVector3D m_up;
  QVector4D m_color;
  QVector2D m_texcoord;
  float m_r1squared;
  float m_max_angle;
};

// Note: Q_MOVABLE_TYPE means it can be memcpy'd.
Q_DECLARE_TYPEINFO(CircleVertex, Q_MOVABLE_TYPE);
