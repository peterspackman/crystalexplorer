#pragma once
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

class CrystalPlaneVertex {
public:
  Q_DECL_CONSTEXPR explicit CrystalPlaneVertex(const QVector3D &position,
                                               const QVector3D &right,
                                               const QVector3D &up,
                                               const QVector4D &color,
                                               const QVector2D &texcoord)
      : m_position(position), m_right(right), m_up(up), m_color(color),
        m_texcoord(texcoord) {}

  Q_DECL_CONSTEXPR const QVector3D &position() const { return m_position; }
  Q_DECL_CONSTEXPR const QVector3D &right() const { return m_right; }
  Q_DECL_CONSTEXPR const QVector3D &up() const { return m_up; }
  Q_DECL_CONSTEXPR const QVector4D &color() const { return m_color; }
  Q_DECL_CONSTEXPR const QVector2D &texcoord() const { return m_texcoord; }

  inline void setPosition(const QVector3D &position) { m_position = position; }
  inline void setRight(const QVector3D &right) { m_right = right; }
  inline void setUp(const QVector3D &up) { m_up = up; }
  inline void setColor(const QVector4D &color) { m_color = color; }
  inline void setTexCoord(const QVector2D &texcoord) { m_texcoord = texcoord; }

  // OpenGL Helpers
  static const int PositionTupleSize = 3;
  static const int RightTupleSize = 3;
  static const int UpTupleSize = 3;
  static const int ColorTupleSize = 4;
  static const int TexcoordTupleSize = 2;

  static Q_DECL_CONSTEXPR int positionOffset() {
    return offsetof(CrystalPlaneVertex, m_position);
  }
  static Q_DECL_CONSTEXPR int rightOffset() {
    return offsetof(CrystalPlaneVertex, m_right);
  }
  static Q_DECL_CONSTEXPR int upOffset() {
    return offsetof(CrystalPlaneVertex, m_up);
  }
  static Q_DECL_CONSTEXPR int colorOffset() {
    return offsetof(CrystalPlaneVertex, m_color);
  }
  static Q_DECL_CONSTEXPR int texcoordOffset() {
    return offsetof(CrystalPlaneVertex, m_texcoord);
  }

  static Q_DECL_CONSTEXPR int stride() { return sizeof(CrystalPlaneVertex); }

private:
  QVector3D m_position;
  QVector3D m_right;
  QVector3D m_up;
  QVector4D m_color;
  QVector2D m_texcoord;
};

// Note: Q_MOVABLE_TYPE means it can be memcpy'd.
Q_DECLARE_TYPEINFO(CrystalPlaneVertex, Q_MOVABLE_TYPE);
