#pragma once
#include "cylinderimpostorvertex.h"
#include "renderer.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <vector>

using std::vector;

class CylinderImpostorRenderer : public IndexedRenderer {
public:
  static constexpr size_t MaxVertices = 65536 / sizeof(CylinderImpostorVertex);
  CylinderImpostorRenderer();
  CylinderImpostorRenderer(const vector<CylinderImpostorVertex> &vertices);
  void addVertices(const vector<CylinderImpostorVertex> &vertices);
  virtual ~CylinderImpostorRenderer();
  void setRadii(float newRadius);

  virtual void beginUpdates() override;
  virtual void endUpdates() override;
  virtual void clear() override;

private:
  void updateBuffers();
  QOpenGLBuffer m_vertex;
  vector<CylinderImpostorVertex> m_vertices;
  vector<GLuint> m_indices;
  vector<GroupIndex<CylinderImpostorVertex>> m_groups;

protected:
  bool m_impostor = true;
};
