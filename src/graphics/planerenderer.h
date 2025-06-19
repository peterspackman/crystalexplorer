#pragma once
#include "planevertex.h"
#include "renderer.h"
#include "plane.h"
#include "planeinstance.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <vector>

using std::vector;

class PlaneRenderer : public IndexedRenderer, protected QOpenGLFunctions {
public:
  PlaneRenderer();
  virtual ~PlaneRenderer();

  // Instance management
  void addPlaneInstance(Plane *plane, PlaneInstance *instance);
  void updatePlaneInstance(Plane *plane, PlaneInstance *instance);
  void removePlaneInstance(PlaneInstance *instance);
  void clear() override;

  inline size_t instanceCount() const { return m_instanceData.size(); }

  void beginUpdates() override;
  void endUpdates() override;
  void draw() override;

  // Texture support for future
  void setTexture(QOpenGLTexture *texture);
  QOpenGLTexture *texture() const { return m_texture; }

private:
  void initializeGeometry();
  void updateBuffers();
  void updateInstanceBuffer();
  
  PlaneInstanceData createInstanceData(Plane *plane, PlaneInstance *instance) const;

  // Vertex data for plane quad (shared by all instances)
  QOpenGLBuffer m_vertexBuffer;
  QOpenGLBuffer m_instanceBuffer;
  vector<PlaneVertex> m_vertices;
  vector<GLuint> m_indices;
  vector<PlaneInstanceData> m_instanceData;
  
  // Track instances for updates
  QMap<PlaneInstance *, int> m_instanceMap;
  
  // Texture support
  QOpenGLTexture *m_texture{nullptr};
  
  bool m_geometryInitialized{false};
};