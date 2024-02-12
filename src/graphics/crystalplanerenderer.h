#pragma once
#include "crystalplanevertex.h"
#include "renderer.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <vector>

using std::vector;

class CrystalPlaneRenderer : public IndexedRenderer {
public:
  static constexpr size_t MaxVertices = 65536 / sizeof(CrystalPlaneVertex);

  CrystalPlaneRenderer();
  void addVertices(const vector<CrystalPlaneVertex> &vertices);
  virtual ~CrystalPlaneRenderer();

  inline size_t size() const { return m_vertices.size() / 4; }

  virtual void beginUpdates();
  virtual void endUpdates();
  void clear();

private:
  void updateBuffers();
  QOpenGLBuffer m_vertex;
  vector<CrystalPlaneVertex> m_vertices;
  vector<GLuint> m_indices;
};
