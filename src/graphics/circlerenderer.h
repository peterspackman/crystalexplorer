#pragma once
#pragma once
#include "circlevertex.h"
#include "renderer.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <vector>

using std::vector;

class CircleRenderer : public IndexedRenderer {
public:
  static constexpr size_t MaxVertices = 65536 / sizeof(CircleVertex);

  CircleRenderer();
  CircleRenderer(const vector<CircleVertex> &vertices);
  void addVertices(const vector<CircleVertex> &vertices);
  virtual ~CircleRenderer();

  inline size_t size() const { return m_vertices.size() / 4; }

  virtual void beginUpdates();
  virtual void endUpdates();
  void clear();

private:
  void updateBuffers();
  QOpenGLBuffer m_vertex;
  vector<CircleVertex> m_vertices;
  vector<GLuint> m_indices;
};
