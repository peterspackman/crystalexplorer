#pragma once
#include "linevertex.h"
#include "renderer.h"
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <vector>

using std::vector;

class LineRenderer : public IndexedRenderer {
public:
  static constexpr size_t MaxVertices = 65536 / sizeof(LineVertex);

  LineRenderer();
  LineRenderer(const vector<LineVertex> &vertices);
  void addLines(const vector<LineVertex> &vertices);
  void clear();
  virtual ~LineRenderer();
  inline size_t size() const { return m_vertices.size(); }

  virtual void beginUpdates();
  virtual void endUpdates();
  virtual void draw() {
    // bind the object, draw, then release
    m_program->setUniformValue("u_alpha", m_alpha);
    m_program->setUniformValue("u_lineScale", m_lineScale);
    glDrawElements(DrawType, m_numberOfIndices, IndexType, 0);
    // reset
  }
  inline void setLineScale(float linewidth) { m_lineScale = linewidth; }
  inline float lineScale() const { return m_lineScale; }

private:
  float m_alpha{1.0};
  float m_lineScale{10.0};
  void updateBuffers();
  QOpenGLBuffer m_vertex;
  vector<GLuint> m_indices;
  vector<LineVertex> m_vertices;
};
