#pragma once
#include <QString>
#include <QVector2D>
#include <QVector3D>

class CylinderImpostorVertex {
public:
  // Constructors
  Q_DECL_CONSTEXPR explicit CylinderImpostorVertex()
      : m_pointA(), m_pointB(), m_colorA(), m_colorB(), m_mapping(),
        m_selection_id(), m_radius() {}

  Q_DECL_CONSTEXPR inline CylinderImpostorVertex(
      const QVector3D &pa, const QVector3D &pb, const QVector3D &ca,
      const QVector3D &cb, const QVector3D &mapping, const QVector3D &id,
      float radius)
      : m_pointA(pa), m_pointB(pb), m_colorA(ca), m_colorB(cb),
        m_mapping(mapping), m_selection_id(id), m_radius(radius) {}

  // Accessors / Mutators
  Q_DECL_CONSTEXPR const QVector3D &pointA() const { return m_pointA; }
  Q_DECL_CONSTEXPR const QVector3D &pointB() const { return m_pointB; }
  Q_DECL_CONSTEXPR const QVector3D &colorA() const { return m_colorA; }
  Q_DECL_CONSTEXPR const QVector3D &colorB() const { return m_colorB; }
  Q_DECL_CONSTEXPR const QVector3D &mapping() const { return m_mapping; }
  Q_DECL_CONSTEXPR const QVector3D &selectionId() const {
    return m_selection_id;
  }
  Q_DECL_CONSTEXPR float radius() const { return m_radius; }

  void inline setPointA(const QVector3D &p) { m_pointA = p; }
  void inline setPointB(const QVector3D &p) { m_pointB = p; }
  void inline setColorA(const QVector3D &c) { m_colorA = c; }
  void inline setColorB(const QVector3D &c) { m_colorB = c; }
  void inline setMapping(const QVector3D &m) { m_mapping = m; }
  void inline setSelectionId(const QVector3D &id) { m_selection_id = id; }
  void inline setRadius(float radius) { m_radius = radius; }

  // OpenGL Helpers
  static const int PointATupleSize = 3;
  static const int PointBTupleSize = 3;
  static const int ColorATupleSize = 3;
  static const int ColorBTupleSize = 3;
  static const int MappingTupleSize = 3;
  static const int SelectionIdTupleSize = 3;
  static const int RadiusSize = 1;

  static Q_DECL_CONSTEXPR int pointAOffset() {
    return offsetof(CylinderImpostorVertex, m_pointA);
  }
  static Q_DECL_CONSTEXPR int pointBOffset() {
    return offsetof(CylinderImpostorVertex, m_pointB);
  }
  static Q_DECL_CONSTEXPR int colorAOffset() {
    return offsetof(CylinderImpostorVertex, m_colorA);
  }
  static Q_DECL_CONSTEXPR int colorBOffset() {
    return offsetof(CylinderImpostorVertex, m_colorB);
  }
  static Q_DECL_CONSTEXPR int mappingOffset() {
    return offsetof(CylinderImpostorVertex, m_mapping);
  }
  static Q_DECL_CONSTEXPR int selectionIdOffset() {
    return offsetof(CylinderImpostorVertex, m_selection_id);
  }
  static Q_DECL_CONSTEXPR int radiusOffset() {
    return offsetof(CylinderImpostorVertex, m_radius);
  }

  static Q_DECL_CONSTEXPR int stride() {
    return sizeof(CylinderImpostorVertex);
  }

private:
  QVector3D m_pointA;
  QVector3D m_pointB;
  QVector3D m_colorA;
  QVector3D m_colorB;
  QVector3D m_mapping;
  QVector3D m_selection_id;
  float m_radius;
};

/*******************************************************************************
 * Inline Implementation
 ******************************************************************************/

// Note: Q_MOVABLE_TYPE means it can be memcpy'd.
Q_DECLARE_TYPEINFO(CylinderImpostorVertex, Q_MOVABLE_TYPE);
