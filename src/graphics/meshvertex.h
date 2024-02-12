#pragma once
#include <QVector3D>

class MeshVertex {
public:
  // Constructors
  Q_DECL_CONSTEXPR explicit MeshVertex()
      : m_position(), m_normal(), m_color(), m_selection_id() {}

  Q_DECL_CONSTEXPR explicit MeshVertex(const QVector3D &position,
                                       const QVector3D &normal,
                                       const QVector3D &color,
                                       const QVector3D &selection_id)
      : m_position(position), m_normal(normal), m_color(color),
        m_selection_id(selection_id) {}

  // Accessors / Mutators
  Q_DECL_CONSTEXPR const QVector3D &position() const { return m_position; }
  Q_DECL_CONSTEXPR const QVector3D &color() const { return m_color; }
  Q_DECL_CONSTEXPR const QVector3D &normal() const { return m_normal; }
  Q_DECL_CONSTEXPR const QVector3D &selectionId() const {
    return m_selection_id;
  }

  inline void setPosition(const QVector3D &position) { m_position = position; }
  inline void setColor(const QVector3D &color) { m_color = color; }
  inline void setNormal(const QVector3D &normal) { m_normal = normal; }
  inline void setSelectionId(const QVector3D &id) { m_selection_id = id; }

  // OpenGL Helpers
  static const int PositionTupleSize = 3;
  static const int ColorTupleSize = 3;
  static const int NormalTupleSize = 3;
  static const int SelectionIdTupleSize = 3;

  static Q_DECL_CONSTEXPR int positionOffset() {
    return offsetof(MeshVertex, m_position);
  }
  static Q_DECL_CONSTEXPR int colorOffset() {
    return offsetof(MeshVertex, m_color);
  }
  static Q_DECL_CONSTEXPR int normalOffset() {
    return offsetof(MeshVertex, m_normal);
  }
  static Q_DECL_CONSTEXPR int selectionIdOffset() {
    return offsetof(MeshVertex, m_selection_id);
  }

  static Q_DECL_CONSTEXPR int stride() { return sizeof(MeshVertex); }

private:
  QVector3D m_position;
  QVector3D m_normal;
  QVector3D m_color;
  QVector3D m_selection_id;
};

/*******************************************************************************
 * Inline Implementation
 ******************************************************************************/

// Note: Q_MOVABLE_TYPE means it can be memcpy'd.
Q_DECLARE_TYPEINFO(MeshVertex, Q_MOVABLE_TYPE);
