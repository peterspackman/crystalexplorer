#pragma once
#pragma once
#include <QDebug>
#include <QVector2D>
#include <QVector3D>
#include <vector>

using std::vector;

class BillboardVertex {
public:
  // Constructors
  Q_DECL_CONSTEXPR explicit BillboardVertex()
      : m_position(), m_dimensions(), m_texcoord() {}

  Q_DECL_CONSTEXPR explicit BillboardVertex(const QVector3D &position,
                                            const QVector2D &dimensions,
                                            const QVector2D &texcoord)
      : m_position(position), m_dimensions(dimensions), m_texcoord(texcoord) {}

  // Accessors / Mutators
  Q_DECL_CONSTEXPR inline const QVector3D &position() const {
    return m_position;
  }
  Q_DECL_CONSTEXPR inline const QVector2D &dimensions() const {
    return m_dimensions;
  }
  Q_DECL_CONSTEXPR inline const QVector2D &texcoord() const {
    return m_texcoord;
  }
  Q_DECL_CONSTEXPR inline float alpha() const { return m_alpha; }

  inline void setPosition(const QVector3D &position) { m_position = position; }
  inline void setDimensions(const QVector2D &dimensions) {
    m_dimensions = dimensions;
  }
  inline void setTexcoord(const QVector2D &texcoord) { m_texcoord = texcoord; }
  inline void setAlpha(float alpha) { m_alpha = alpha; };

  // OpenGL Helpers
  static const int PositionTupleSize = 3;
  static const int DimensionsTupleSize = 2;
  static const int TexcoordTupleSize = 2;
  static const int AlphaSize = 1;

  static Q_DECL_CONSTEXPR int positionOffset() {
    return offsetof(BillboardVertex, m_position);
  }
  static Q_DECL_CONSTEXPR int dimensionsOffset() {
    return offsetof(BillboardVertex, m_dimensions);
  }
  static Q_DECL_CONSTEXPR int texcoordOffset() {
    return offsetof(BillboardVertex, m_texcoord);
  }
  static Q_DECL_CONSTEXPR int alphaOffset() {
    return offsetof(BillboardVertex, m_alpha);
  }

  static Q_DECL_CONSTEXPR int stride() { return sizeof(BillboardVertex); }

private:
  QVector3D m_position;
  QVector2D m_dimensions;
  float m_alpha{1.0f};
  QVector2D m_texcoord;
};

/*******************************************************************************
 * Inline Implementation
 ******************************************************************************/

// Note: Q_MOVABLE_TYPE means it can be memcpy'd.
Q_DECLARE_TYPEINFO(BillboardVertex, Q_MOVABLE_TYPE);
