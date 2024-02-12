#pragma once
#include "renderer.h"
#include "sphereimpostorvertex.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <vector>

using std::vector;

class SphereImpostorRenderer : public IndexedRenderer {
public:
  static constexpr size_t MaxVertices = 65536 / sizeof(SphereImpostorVertex);

  SphereImpostorRenderer();
  SphereImpostorRenderer(const vector<SphereImpostorVertex> &vertices);
  void addVertices(const vector<SphereImpostorVertex> &vertices);
  virtual ~SphereImpostorRenderer();
  const GroupIndex<SphereImpostorVertex> &group(size_t i) {
    return m_groups.at(i);
  }
  inline size_t size() const { return m_vertices.size() / 4; }
  inline float sphereRadius(size_t idx) const {
    return m_vertices[idx * 4].radius();
  }

  virtual void beginUpdates() override;
  virtual void endUpdates() override;
  virtual void clear() override;

  void setRadii(float newRadius);

private:
  void updateBuffers();
  QOpenGLBuffer m_vertex;
  vector<SphereImpostorVertex> m_vertices;
  vector<GLuint> m_indices;
  // TODO convert this from a vector to a map with IDs
  vector<GroupIndex<SphereImpostorVertex>> m_atoms;
  vector<GroupIndex<SphereImpostorVertex>> m_groups;

protected:
  bool m_impostor = true;
};
