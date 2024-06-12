#pragma once

#include "renderer.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <vector>

class EllipsoidInstance {
public:
  // Constructors
  constexpr explicit EllipsoidInstance()
      : m_position(), m_a(), m_b(), m_c(), m_color(), m_selection_id() {}

  constexpr explicit EllipsoidInstance(const QVector3D &position,
                                       const QVector3D &a, const QVector3D &b,
                                       const QVector3D &c,
                                       const QVector3D &color,
                                       const QVector3D &selection_id)
      : m_position(position), m_a(a), m_b(b), m_c(c), m_color(color),
        m_selection_id(selection_id) {}

  inline void setSelected(bool selected) {
    m_color.setX(selected ? -std::abs(m_color.x()) : std::abs(m_color.x()));
  }

  constexpr inline const auto &position() const { return m_position; }
  constexpr inline const auto &a() const { return m_a; }
  constexpr inline const auto &b() const { return m_b; }
  constexpr inline const auto &c() const { return m_c; }
  constexpr inline const auto &color() const { return m_color; }
  constexpr inline const auto &selectionId() const { return m_selection_id; }

  constexpr inline void setPosition(const QVector3D &position) {
    m_position = position;
  }
  constexpr inline void setA(const QVector3D &a) { m_a = a; }
  constexpr inline void setB(const QVector3D &b) { m_b = b; }
  constexpr inline void setC(const QVector3D &c) { m_c = c; }
  constexpr inline void setColor(const QVector3D &color) { m_color = color; }
  constexpr inline void setSelectionId(const QVector3D &selection_id) {
    m_selection_id = selection_id;
  }
  // OpenGL Helpers
  static constexpr int PositionTupleSize = 3;
  static constexpr int ATupleSize = 3;
  static constexpr int BTupleSize = 3;
  static constexpr int CTupleSize = 3;
  static constexpr int ColorTupleSize = 3;
  static constexpr int SelectionIdSize = 3;

  static constexpr int positionOffset() {
    return offsetof(EllipsoidInstance, m_position);
  }
  static constexpr int aOffset() { return offsetof(EllipsoidInstance, m_a); }
  static constexpr int bOffset() { return offsetof(EllipsoidInstance, m_b); }
  static constexpr int cOffset() { return offsetof(EllipsoidInstance, m_c); }
  static constexpr int colorOffset() {
    return offsetof(EllipsoidInstance, m_color);
  }
  static constexpr int selectionIdOffset() {
    return offsetof(EllipsoidInstance, m_selection_id);
  }

  static constexpr int stride() { return sizeof(EllipsoidInstance); }

private:
  QVector3D m_position;
  QVector3D m_a;
  QVector3D m_b;
  QVector3D m_c;
  QVector3D m_color;
  QVector3D m_selection_id;
};

Q_DECLARE_TYPEINFO(EllipsoidInstance, Q_MOVABLE_TYPE);

class EllipsoidRenderer : public IndexedRenderer {
public:
  EllipsoidRenderer();
  EllipsoidRenderer(const vector<EllipsoidInstance> &instances);
  void addInstance(const EllipsoidInstance &);
  void addInstances(const vector<EllipsoidInstance> &instances);

  inline size_t size() const { return m_instances.size(); }

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
  vector<EllipsoidInstance> m_instances;
};
