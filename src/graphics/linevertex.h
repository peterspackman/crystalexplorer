#pragma once
#include <QVector2D>
#include <QVector3D>

class LineVertex {
public:
  Q_DECL_CONSTEXPR explicit LineVertex()
      : m_pointA(), m_pointB(), m_colorA(), m_colorB(), m_texcoord() {}

  Q_DECL_CONSTEXPR explicit LineVertex(const QVector3D &pointA,
                                       const QVector3D &pointB,
                                       const QVector3D &colorA,
                                       const QVector3D &colorB,
                                       const QVector2D &texcoord, float width)
      : m_pointA(pointA), m_pointB(pointB), m_colorA(colorA), m_colorB(colorB),
        m_texcoord(texcoord), m_lineWidth{width} {}

  Q_DECL_CONSTEXPR inline const QVector3D &pointA() const { return m_pointA; }
  Q_DECL_CONSTEXPR inline const QVector3D &pointB() const { return m_pointB; }
  Q_DECL_CONSTEXPR inline const QVector3D &colorA() const { return m_colorA; }
  Q_DECL_CONSTEXPR inline const QVector3D &colorB() const { return m_colorB; }
  Q_DECL_CONSTEXPR inline const QVector2D &texcoord() const {
    return m_texcoord;
  }
  Q_DECL_CONSTEXPR inline float lineWidth() const { return m_lineWidth; }

  void inline setPointA(const QVector3D &point) { m_pointA = point; }
  void inline setPointB(const QVector3D &point) { m_pointB = point; }
  void inline setColorA(const QVector3D &color) { m_colorA = color; }
  void inline setColorB(const QVector3D &color) { m_colorB = color; }
  void inline setTexCoord(const QVector2D &texcoord) { m_texcoord = texcoord; }
  void inline setLineWidth(const float width) { m_lineWidth = width; }

  static const int PointATupleSize = 3;
  static const int PointBTupleSize = 3;
  static const int ColorATupleSize = 3;
  static const int ColorBTupleSize = 3;
  static const int TexcoordTupleSize = 2;
  static const int LineWidthSize = 1;

  static Q_DECL_CONSTEXPR inline int pointAOffset() {
    return offsetof(LineVertex, m_pointA);
  }
  static Q_DECL_CONSTEXPR inline int pointBOffset() {
    return offsetof(LineVertex, m_pointB);
  }
  static Q_DECL_CONSTEXPR inline int colorAOffset() {
    return offsetof(LineVertex, m_colorA);
  }
  static Q_DECL_CONSTEXPR inline int colorBOffset() {
    return offsetof(LineVertex, m_colorB);
  }
  static Q_DECL_CONSTEXPR inline int texcoordOffset() {
    return offsetof(LineVertex, m_texcoord);
  }
  static Q_DECL_CONSTEXPR inline int lineWidthOffset() {
    return offsetof(LineVertex, m_lineWidth);
  }
  static Q_DECL_CONSTEXPR inline int stride() { return sizeof(LineVertex); }

private:
  QVector3D m_pointA;
  QVector3D m_pointB;
  QVector3D m_colorA;
  QVector3D m_colorB;
  QVector2D m_texcoord;
  float m_lineWidth{1};
};

// Note: Q_MOVABLE_TYPE facilitates memcpy.
Q_DECLARE_TYPEINFO(LineVertex, Q_MOVABLE_TYPE);
