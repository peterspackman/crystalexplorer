#pragma once
#include "mesh.h"
#include "meshinstancevertex.h"
#include "renderer.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <vector>

class MeshInstanceRenderer : public IndexedRenderer {
public:
  struct IndexTuple {
    GLuint i, j, k;
  };

  MeshInstanceRenderer(Mesh *);
  virtual ~MeshInstanceRenderer();


  void addInstance(const MeshInstanceVertex &);
  void addInstances(const std::vector<MeshInstanceVertex> &instances);

  virtual void draw() override;
  virtual void beginUpdates() override;
  virtual void endUpdates() override;
  virtual void clear() override;

  [[nodiscard]] inline const auto &instances() const { return m_instances; }
  [[nodiscard]] inline auto &instances() { return m_instances; }

private:
  void setMesh(Mesh *);
  void updateBuffers();
  QOpenGLBuffer m_vertex;
  QOpenGLBuffer m_instance;

  QOpenGLShaderProgram *m_program{nullptr};
  std::vector<MeshInstanceVertex> m_instances;

  QOpenGLBuffer m_vertexPropertyBuffer;
  QOpenGLTexture *m_vertexPropertyTexture{nullptr}; 
  int m_numFaces{0};

protected:
  bool m_impostor = false;
};
