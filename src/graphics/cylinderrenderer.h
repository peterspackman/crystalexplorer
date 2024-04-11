#pragma once

#include "renderer.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <vector>

class CylinderInstance {
public:
  // Constructors
  constexpr explicit CylinderInstance()
      : m_radius(1.0), m_a(), m_b(), m_colorA(), m_colorB(), m_selection_idA(),
        m_selection_idB() {}

  constexpr explicit CylinderInstance(float radius, const QVector3D &a,
                                      const QVector3D &b,
                                      const QVector3D &colorA,
                                      const QVector3D &colorB,
                                      const QVector3D &selectionIdA,
                                      const QVector3D &selectionIdB)
      : m_radius(radius), m_a(a), m_b(b), m_colorA(colorA), m_colorB(colorB),
        m_selection_idA(selectionIdA), m_selection_idB(selectionIdB) {}

  inline void setSelected(bool selectedA, bool selectedB) {
    m_colorA.setX(selectedA ? -std::abs(m_colorA.x()) : std::abs(m_colorA.x()));
    m_colorB.setX(selectedB ? -std::abs(m_colorB.x()) : std::abs(m_colorB.x()));
  }

  constexpr inline float radius() const { return m_radius; }
  constexpr inline const auto &a() const { return m_a; }
  constexpr inline const auto &b() const { return m_b; }
  constexpr inline const auto &colorA() const { return m_colorA; }
  constexpr inline const auto &colorB() const { return m_colorB; }
  constexpr inline const auto &selectionIdA() const { return m_selection_idA; }
  constexpr inline const auto &selectionIdB() const { return m_selection_idB; }

  constexpr inline void setRadius(float radius) { m_radius = radius; }
  constexpr inline void setA(const QVector3D &a) { m_a = a; }
  constexpr inline void setB(const QVector3D &b) { m_b = b; }
  constexpr inline void setColorA(const QVector3D &color) { m_colorA = color; }
  constexpr inline void setColorB(const QVector3D &color) { m_colorB = color; }
  constexpr inline void setSelectionIdA(const QVector3D &selection_id) {
    m_selection_idA = selection_id;
  }
  constexpr inline void setSelectionIdB(const QVector3D &selection_id) {
    m_selection_idB = selection_id;
  }

  // OpenGL Helpers
  static constexpr int RadiusSize = 1;
  static constexpr int ATupleSize = 3;
  static constexpr int BTupleSize = 3;
  static constexpr int ColorATupleSize = 3;
  static constexpr int ColorBTupleSize = 3;
  static constexpr int SelectionIdASize = 3;
  static constexpr int SelectionIdBSize = 3;

  static constexpr int aOffset() { return offsetof(CylinderInstance, m_a); }
  static constexpr int bOffset() { return offsetof(CylinderInstance, m_b); }
  static constexpr int radiusOffset() {
    return offsetof(CylinderInstance, m_radius);
  }
  static constexpr int colorAOffset() {
    return offsetof(CylinderInstance, m_colorA);
  }
  static constexpr int colorBOffset() {
    return offsetof(CylinderInstance, m_colorB);
  }
  static constexpr int selectionIdAOffset() {
    return offsetof(CylinderInstance, m_selection_idA);
  }
  static constexpr int selectionIdBOffset() {
    return offsetof(CylinderInstance, m_selection_idB);
  }

  static constexpr int stride() { return sizeof(CylinderInstance); }

private:
  float m_radius{1.0};
  QVector3D m_a;
  QVector3D m_b;
  QVector3D m_colorA;
  QVector3D m_colorB;
  QVector3D m_selection_idA;
  QVector3D m_selection_idB;
};

Q_DECLARE_TYPEINFO(CylinderInstance, Q_MOVABLE_TYPE);

class CylinderRenderer : public IndexedRenderer {
public:
  CylinderRenderer();
  CylinderRenderer(const vector<CylinderInstance> &instances);
  void addInstance(const CylinderInstance &);
  void addInstances(const vector<CylinderInstance> &instances);

  virtual void beginUpdates() override;
  virtual void endUpdates() override;
  virtual void draw() override;
  virtual void clear() override;

private:
  void loadBaseMesh();
  struct Face {
    GLuint i, j, k;
  };
  void updateBuffers();
  QOpenGLBuffer m_vertex;
  QOpenGLBuffer m_instance;
  vector<QVector3D> m_vertices;
  vector<Face> m_faces;
  vector<CylinderInstance> m_instances;
};
