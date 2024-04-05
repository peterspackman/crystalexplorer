#pragma once
#include <QVector3D>
#include <QMatrix3x3>

class MeshInstanceVertex {
public:
  // Constructors
  explicit MeshInstanceVertex() : m_translation(), m_rotation(), m_selection_id() {}

  explicit MeshInstanceVertex(const QVector3D &translation,
                                        const QMatrix3x3 &rotation,
                                        const QVector3D &selection_id,
					float alpha = 1.0)
      : m_translation(translation), m_rotation(rotation), m_selection_id(selection_id), m_alpha(alpha) {}

  inline const auto &translation() const { return m_translation; }
  inline const auto &rotation() const { return m_rotation; }
  inline const auto &selectionId() const { return m_selection_id; }
  float alpha() const { return m_alpha; }

  inline void setTranslation(const QVector3D &t) { m_translation = t;}
  inline void setRotation(const QMatrix3x3 &r) { m_rotation = r; }
  inline void setSelectionId(const QVector3D &s) { m_selection_id = s; }
  inline void setAlpha(float a) { m_alpha = a; }

  // OpenGL Helpers
  static constexpr int TranslationTupleSize = 3;
  static constexpr int RotationTupleSize = 3;
  static constexpr int SelectionIdSize = 3;
  static constexpr int AlphaSize = 1;

  static constexpr int translationOffset() { return offsetof(MeshInstanceVertex, m_translation); }
  static constexpr int rotationOffset() { return offsetof(MeshInstanceVertex, m_rotation); }
  static constexpr int selectionIdOffset() { return offsetof(MeshInstanceVertex, m_selection_id); }
  static constexpr int alphaOffset() { return offsetof(MeshInstanceVertex, m_alpha); }
  static constexpr int stride() { return sizeof(MeshInstanceVertex); }

private:
  QVector3D m_translation;
  QMatrix3x3 m_rotation;
  QVector3D m_selection_id;
  float m_alpha{1.0};
};

Q_DECLARE_TYPEINFO(MeshInstanceVertex, Q_MOVABLE_TYPE);
