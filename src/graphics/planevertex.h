#pragma once
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

class PlaneVertex {
public:
  Q_DECL_CONSTEXPR explicit PlaneVertex(const QVector3D &position,
                                        const QVector2D &texcoord)
      : m_position(position), m_texcoord(texcoord) {}

  Q_DECL_CONSTEXPR const QVector3D &position() const { return m_position; }
  Q_DECL_CONSTEXPR const QVector2D &texcoord() const { return m_texcoord; }

  inline void setPosition(const QVector3D &position) { m_position = position; }
  inline void setTexCoord(const QVector2D &texcoord) { m_texcoord = texcoord; }

  // OpenGL Helpers
  static const int PositionTupleSize = 3;
  static const int TexcoordTupleSize = 2;

  static Q_DECL_CONSTEXPR int positionOffset() {
    return offsetof(PlaneVertex, m_position);
  }
  static Q_DECL_CONSTEXPR int texcoordOffset() {
    return offsetof(PlaneVertex, m_texcoord);
  }

  static Q_DECL_CONSTEXPR int stride() { return sizeof(PlaneVertex); }

private:
  QVector3D m_position;
  QVector2D m_texcoord;
};

// Instance data for each plane instance
class PlaneInstanceData {
public:
  Q_DECL_CONSTEXPR explicit PlaneInstanceData(const QVector3D &origin,
                                               const QVector3D &axisA,
                                               const QVector3D &axisB,
                                               const QVector4D &color,
                                               const QVector4D &gridParams,
                                               const QVector4D &boundsA,
                                               const QVector4D &boundsB)
      : m_origin(origin), m_axisA(axisA), m_axisB(axisB), m_color(color),
        m_gridParams(gridParams), m_boundsA(boundsA), m_boundsB(boundsB) {}

  Q_DECL_CONSTEXPR const QVector3D &origin() const { return m_origin; }
  Q_DECL_CONSTEXPR const QVector3D &axisA() const { return m_axisA; }
  Q_DECL_CONSTEXPR const QVector3D &axisB() const { return m_axisB; }
  Q_DECL_CONSTEXPR const QVector4D &color() const { return m_color; }
  Q_DECL_CONSTEXPR const QVector4D &gridParams() const { return m_gridParams; }
  Q_DECL_CONSTEXPR const QVector4D &boundsA() const { return m_boundsA; }
  Q_DECL_CONSTEXPR const QVector4D &boundsB() const { return m_boundsB; }

  // OpenGL Helpers
  static const int OriginTupleSize = 3;
  static const int AxisATupleSize = 3;
  static const int AxisBTupleSize = 3;
  static const int ColorTupleSize = 4;
  static const int GridParamsTupleSize = 4;
  static const int BoundsATupleSize = 4;
  static const int BoundsBTupleSize = 4;

  static Q_DECL_CONSTEXPR int originOffset() {
    return offsetof(PlaneInstanceData, m_origin);
  }
  static Q_DECL_CONSTEXPR int axisAOffset() {
    return offsetof(PlaneInstanceData, m_axisA);
  }
  static Q_DECL_CONSTEXPR int axisBOffset() {
    return offsetof(PlaneInstanceData, m_axisB);
  }
  static Q_DECL_CONSTEXPR int colorOffset() {
    return offsetof(PlaneInstanceData, m_color);
  }
  static Q_DECL_CONSTEXPR int gridParamsOffset() {
    return offsetof(PlaneInstanceData, m_gridParams);
  }
  static Q_DECL_CONSTEXPR int boundsAOffset() {
    return offsetof(PlaneInstanceData, m_boundsA);
  }
  static Q_DECL_CONSTEXPR int boundsBOffset() {
    return offsetof(PlaneInstanceData, m_boundsB);
  }

  static Q_DECL_CONSTEXPR int stride() { return sizeof(PlaneInstanceData); }

private:
  QVector3D m_origin;     // Plane origin
  QVector3D m_axisA;      // First axis vector
  QVector3D m_axisB;      // Second axis vector
  QVector4D m_color;      // RGBA color
  QVector4D m_gridParams; // [showGrid, gridSpacing, showAxes, showBounds]
  QVector4D m_boundsA;    // [minA, maxA, unused, unused]
  QVector4D m_boundsB;    // [minB, maxB, unused, unused]
};

Q_DECLARE_TYPEINFO(PlaneVertex, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(PlaneInstanceData, Q_MOVABLE_TYPE);