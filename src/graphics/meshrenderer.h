#pragma once
#include "meshvertex.h"
#include "renderer.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <vector>

using std::vector;

class MeshRenderer : public IndexedRenderer {
public:
  struct IndexTuple {
    GLuint i, j, k;
  };
  static constexpr size_t MaxVertices = 65536 / sizeof(MeshVertex);

  MeshRenderer();
  MeshRenderer(const vector<MeshVertex> &vertices,
               const vector<IndexTuple> &indices);
  void addMesh(const vector<MeshVertex> &vertices,
               const vector<IndexTuple> &indices);

  inline void setFrontFace(GLenum frontFace) { m_frontFace = frontFace; }
  inline void setCullFace(bool value, GLenum face) {
    m_doCulling = value;
    m_cullFace = face;
  }
  void clear();
  virtual ~MeshRenderer();

  inline size_t size() const { return m_vertices.size(); }

  virtual void draw() {
    if (m_numberOfIndices == 0)
      return;
    // bind the object, draw, then release
    if (!m_doCulling) {
      glDisable(GL_CULL_FACE);
    }
    m_program->setUniformValue("u_alpha", m_alpha);
    glFrontFace(m_frontFace);
    glDrawElements(DrawType, m_numberOfIndices, IndexType, 0);
    // reset
    if (!m_doCulling) {
      glCullFace(GL_CULL_FACE);
      glCullFace(GL_BACK);
    }
    glFrontFace(GL_CCW);
  }

  void setAlpha(float alpha) { m_alpha = alpha; }

private:
  float m_alpha{1.0};
  void updateBuffers();
  QOpenGLBuffer m_vertex;
  vector<MeshVertex> m_vertices;
  vector<IndexTuple> m_indices;
  bool m_doCulling{true};
  GLenum m_cullFace{GL_BACK};
  GLenum m_frontFace{GL_CCW};
  static QOpenGLShaderProgram *getShaderProgram();
  static QOpenGLShaderProgram *g_program;

protected:
  bool m_impostor = false;
};
