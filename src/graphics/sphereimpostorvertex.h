#pragma once
#include <QDebug>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <vector>

using std::vector;

class SphereImpostorVertex {
public:
  // Constructors
  Q_DECL_CONSTEXPR explicit SphereImpostorVertex()
      : m_position(), m_color(), m_radius(0), m_texcoord(), m_selection_id() {}

  Q_DECL_CONSTEXPR explicit SphereImpostorVertex(const QVector3D &position,
                                                 const QVector4D &color,
                                                 float radius,
                                                 const QVector2D &texcoord,
                                                 const QVector3D &id)
      : m_position(position), m_color(color), m_radius(radius),
        m_texcoord(texcoord), m_selection_id(id) {}

  // Accessors / Mutators
  Q_DECL_CONSTEXPR const auto &position() const { return m_position; }
  Q_DECL_CONSTEXPR const auto &color() const { return m_color; }
  Q_DECL_CONSTEXPR const auto &texcoord() const { return m_texcoord; }
  Q_DECL_CONSTEXPR const auto &radius() const { return m_radius; }
  Q_DECL_CONSTEXPR const auto &selectionId() const { return m_selection_id; }
  inline void setPosition(const QVector3D &position) { m_position = position; }
  inline void setColor(const QVector4D &color) { m_color = color; }
  inline void setRadius(float radius) { m_radius = radius; }
  inline void setTexcoord(const QVector2D &texcoord) { m_texcoord = texcoord; }
  inline void setSelectionId(const QVector3D &id) { m_selection_id = id; }

  inline void toggleSelected() { m_color[0] = -m_color[0]; }
  inline void setSelected(bool selected) {
    m_color[0] = selected ? -std::abs(m_color[0]) : std::abs(m_color[0]);
  }
  // OpenGL Helpers
  static const int PositionTupleSize = 3;
  static const int ColorTupleSize = 4;
  static const int RadiusSize = 1;
  static const int TexcoordTupleSize = 2;
  static const int SelectionIdTupleSize = 3;

  static Q_DECL_CONSTEXPR int positionOffset() {
    return offsetof(SphereImpostorVertex, m_position);
  }
  static Q_DECL_CONSTEXPR int radiusOffset() {
    return offsetof(SphereImpostorVertex, m_radius);
  }
  static Q_DECL_CONSTEXPR int colorOffset() {
    return offsetof(SphereImpostorVertex, m_color);
  }
  static Q_DECL_CONSTEXPR int texcoordOffset() {
    return offsetof(SphereImpostorVertex, m_texcoord);
  }
  static Q_DECL_CONSTEXPR int selectionIdOffset() {
    return offsetof(SphereImpostorVertex, m_selection_id);
  }

  static Q_DECL_CONSTEXPR int stride() { return sizeof(SphereImpostorVertex); }

private:
  QVector3D m_position;
  QVector4D m_color;
  float m_radius;
  QVector2D m_texcoord;
  QVector3D m_selection_id;
};

// Note: Q_MOVABLE_TYPE means it can be memcpy'd.
Q_DECLARE_TYPEINFO(SphereImpostorVertex, Q_MOVABLE_TYPE);
