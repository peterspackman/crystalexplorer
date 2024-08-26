#pragma once
#include <QVector3D>
#include <QMatrix3x3>
#include <QOpenGLFunctions>

class MeshInstanceVertex {
public:
    // Constructors
    explicit MeshInstanceVertex() {}

    explicit MeshInstanceVertex(const QVector3D &translation,
	    const QMatrix3x3 &rotation,
	    const QVector3D &selection_id,
	    int propertyIndex = 0,
	    float alpha = 1.0)
	: m_translation(translation), m_selection_id(selection_id),
	m_propertyIndex(static_cast<GLint>(propertyIndex)), m_alpha(alpha) {
	    setRotation(rotation);
	}

    inline const auto &translation() const { return m_translation; }
    inline const auto &rotation1() const { return m_rotation1; }
    inline const auto &rotation2() const { return m_rotation2; }
    inline const auto &rotation3() const { return m_rotation3; }
    inline const auto &selectionId() const { return m_selection_id; }
    inline float alpha() const { return m_alpha; }
    inline GLint propertyIndex() const { return m_propertyIndex; }


    inline void setTranslation(const QVector3D &t) { m_translation = t;}
    inline void setRotation(const QMatrix3x3 &r) { 
	m_rotation1 = QVector3D(r(0, 0), r(1, 0), r(2, 0));
	m_rotation2 = QVector3D(r(0, 1), r(1, 1), r(2, 1));
	m_rotation3 = QVector3D(r(0, 2), r(1, 2), r(2, 2));
    }
    inline void setSelectionId(const QVector3D &s) { m_selection_id = s; }
    inline void setAlpha(float a) { m_alpha = a; }
    inline void setPropertyIndex(GLint i) { m_propertyIndex = i; }

    // OpenGL Helpers
    static constexpr int TranslationTupleSize = 3;
    static constexpr int RotationTupleSize = 3;
    static constexpr int SelectionIdSize = 3;
    static constexpr int PropertyIndexSize = 1;
    static constexpr int AlphaSize = 1;

    static constexpr int translationOffset() { return offsetof(MeshInstanceVertex, m_translation); }
    static constexpr int rotation1Offset() { return offsetof(MeshInstanceVertex, m_rotation1); }
    static constexpr int rotation2Offset() { return offsetof(MeshInstanceVertex, m_rotation2); }
    static constexpr int rotation3Offset() { return offsetof(MeshInstanceVertex, m_rotation3); }
    static constexpr int selectionIdOffset() { return offsetof(MeshInstanceVertex, m_selection_id); }
    static constexpr int propertyIndexOffset() { return offsetof(MeshInstanceVertex, m_propertyIndex); }
    static constexpr int alphaOffset() { return offsetof(MeshInstanceVertex, m_alpha); }
    static constexpr int stride() { return sizeof(MeshInstanceVertex); }

private:
    QVector3D m_translation;
    QVector3D m_rotation1;
    QVector3D m_rotation2;
    QVector3D m_rotation3;
    QVector3D m_selection_id;
    float m_propertyIndex{0};
    float m_alpha{1.0};
};

Q_DECLARE_TYPEINFO(MeshInstanceVertex, Q_MOVABLE_TYPE);
