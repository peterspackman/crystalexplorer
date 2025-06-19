#include "planerenderer.h"
#include "shaderloader.h"
#include <QOpenGLShaderProgram>
#include <QDebug>

PlaneRenderer::PlaneRenderer()
    : m_vertexBuffer(QOpenGLBuffer::VertexBuffer),
      m_instanceBuffer(QOpenGLBuffer::VertexBuffer) {
  
  initializeOpenGLFunctions();
  
  // Create Shader Program
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex,
                                              cx::shader::loadShaderFile(":/shaders/plane.vert"));
  m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment,
                                              cx::shader::loadShaderFile(":/shaders/plane.frag"));
  m_program->link();
  m_program->bind();

  // Create buffers
  m_vertexBuffer.create();
  m_vertexBuffer.bind();
  m_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);

  m_instanceBuffer.create();
  m_instanceBuffer.bind();
  m_instanceBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::StaticDraw);

  // Create VAO
  m_object.create();
  m_object.bind();

  // Vertex attributes (per-vertex data)
  m_program->enableAttributeArray(0); // position
  m_program->enableAttributeArray(1); // texcoord
  
  m_vertexBuffer.bind();
  m_program->setAttributeBuffer(0, GL_FLOAT, PlaneVertex::positionOffset(),
                                PlaneVertex::PositionTupleSize, PlaneVertex::stride());
  m_program->setAttributeBuffer(1, GL_FLOAT, PlaneVertex::texcoordOffset(),
                                PlaneVertex::TexcoordTupleSize, PlaneVertex::stride());

  // Instance attributes (per-instance data)
  m_instanceBuffer.bind();
  
  // origin (attribute 2)
  m_program->enableAttributeArray(2);
  m_program->setAttributeBuffer(2, GL_FLOAT, PlaneInstanceData::originOffset(),
                                PlaneInstanceData::OriginTupleSize, PlaneInstanceData::stride());
  this->glVertexAttribDivisor(2, 1); // advance once per instance
  
  // axisA (attribute 3)
  m_program->enableAttributeArray(3);
  m_program->setAttributeBuffer(3, GL_FLOAT, PlaneInstanceData::axisAOffset(),
                                PlaneInstanceData::AxisATupleSize, PlaneInstanceData::stride());
  this->glVertexAttribDivisor(3, 1);
  
  // axisB (attribute 4)
  m_program->enableAttributeArray(4);
  m_program->setAttributeBuffer(4, GL_FLOAT, PlaneInstanceData::axisBOffset(),
                                PlaneInstanceData::AxisBTupleSize, PlaneInstanceData::stride());
  this->glVertexAttribDivisor(4, 1);
  
  // color (attribute 5)
  m_program->enableAttributeArray(5);
  m_program->setAttributeBuffer(5, GL_FLOAT, PlaneInstanceData::colorOffset(),
                                PlaneInstanceData::ColorTupleSize, PlaneInstanceData::stride());
  this->glVertexAttribDivisor(5, 1);
  
  // gridParams (attribute 6)
  m_program->enableAttributeArray(6);
  m_program->setAttributeBuffer(6, GL_FLOAT, PlaneInstanceData::gridParamsOffset(),
                                PlaneInstanceData::GridParamsTupleSize, PlaneInstanceData::stride());
  this->glVertexAttribDivisor(6, 1);
  
  // boundsA (attribute 7)
  m_program->enableAttributeArray(7);
  m_program->setAttributeBuffer(7, GL_FLOAT, PlaneInstanceData::boundsAOffset(),
                                PlaneInstanceData::BoundsATupleSize, PlaneInstanceData::stride());
  this->glVertexAttribDivisor(7, 1);
  
  // boundsB (attribute 8)
  m_program->enableAttributeArray(8);
  m_program->setAttributeBuffer(8, GL_FLOAT, PlaneInstanceData::boundsBOffset(),
                                PlaneInstanceData::BoundsBTupleSize, PlaneInstanceData::stride());
  this->glVertexAttribDivisor(8, 1);

  // Release all
  m_index.release();
  m_instanceBuffer.release();
  m_vertexBuffer.release();
  m_object.release();
  m_program->release();

  initializeGeometry();
}

PlaneRenderer::~PlaneRenderer() {
  delete m_texture;
}

void PlaneRenderer::initializeGeometry() {
  if (m_geometryInitialized)
    return;

  // Create a unit quad geometry that will be transformed by instances
  m_vertices = {
    PlaneVertex(QVector3D(0.0f, 0.0f, 0.0f), QVector2D(0.0f, 0.0f)), // bottom-left
    PlaneVertex(QVector3D(1.0f, 0.0f, 0.0f), QVector2D(1.0f, 0.0f)), // bottom-right
    PlaneVertex(QVector3D(1.0f, 1.0f, 0.0f), QVector2D(1.0f, 1.0f)), // top-right
    PlaneVertex(QVector3D(0.0f, 1.0f, 0.0f), QVector2D(0.0f, 1.0f))  // top-left
  };

  // Two triangles to form a quad (draw both sides)
  m_indices = {
    0, 1, 2,  0, 2, 3,  // front face
    2, 1, 0,  3, 2, 0   // back face  
  };

  m_numberOfIndices = static_cast<GLsizei>(m_indices.size());

  // Update vertex buffer
  m_vertexBuffer.bind();
  m_vertexBuffer.allocate(m_vertices.data(), 
                         static_cast<int>(sizeof(PlaneVertex) * m_vertices.size()));
  m_vertexBuffer.release();

  // Update index buffer
  m_index.bind();
  m_index.allocate(m_indices.data(),
                   static_cast<int>(sizeof(GLuint) * m_indices.size()));
  m_index.release();

  m_geometryInitialized = true;
}

void PlaneRenderer::addPlaneInstance(Plane *plane, PlaneInstance *instance) {
  if (!plane || !instance || m_instanceMap.contains(instance))
    return;

  qDebug() << "PlaneRenderer: Adding plane instance" << instance->name() 
           << "for plane" << plane->name();

  PlaneInstanceData data = createInstanceData(plane, instance);
  m_instanceData.push_back(data);
  m_instanceMap[instance] = m_instanceData.size() - 1;
  
  qDebug() << "PlaneRenderer: Total instances:" << m_instanceData.size();
  
  updateInstanceBuffer();
}

void PlaneRenderer::updatePlaneInstance(Plane *plane, PlaneInstance *instance) {
  if (!plane || !instance || !m_instanceMap.contains(instance))
    return;

  int index = m_instanceMap[instance];
  if (index >= 0 && index < static_cast<int>(m_instanceData.size())) {
    m_instanceData[index] = createInstanceData(plane, instance);
    updateInstanceBuffer();
  }
}

void PlaneRenderer::removePlaneInstance(PlaneInstance *instance) {
  if (!instance || !m_instanceMap.contains(instance))
    return;

  int index = m_instanceMap[instance];
  
  // Remove from data vector
  m_instanceData.erase(m_instanceData.begin() + index);
  
  // Update indices in map for elements after removed one
  for (auto it = m_instanceMap.begin(); it != m_instanceMap.end(); ++it) {
    if (it.value() > index) {
      it.value()--;
    }
  }
  
  // Remove from map
  m_instanceMap.remove(instance);
  
  updateInstanceBuffer();
}

void PlaneRenderer::clear() {
  m_instanceData.clear();
  m_instanceMap.clear();
  updateInstanceBuffer();
}

PlaneInstanceData PlaneRenderer::createInstanceData(Plane *plane, PlaneInstance *instance) const {
  QVector3D origin = instance->origin();
  QVector3D axisA = plane->axisA();
  QVector3D axisB = plane->axisB();
  QVector4D color(plane->color().redF(), plane->color().greenF(), 
                  plane->color().blueF(), plane->isVisible() ? 0.5f : 0.0f);
  
  // Pack grid parameters: [showGrid, gridSpacing, showAxes, showBounds]
  QVector4D gridParams(
    plane->showGrid() ? 1.0f : 0.0f,
    static_cast<float>(plane->gridSpacing()),
    plane->showAxes() ? 1.0f : 0.0f,
    plane->showBounds() ? 1.0f : 0.0f
  );
  
  QVector2D boundsA = plane->boundsA();
  QVector2D boundsB = plane->boundsB();
  
  QVector4D boundsAVec(boundsA.x(), boundsA.y(), 0.0f, 0.0f);
  QVector4D boundsBVec(boundsB.x(), boundsB.y(), 0.0f, 0.0f);
  
  return PlaneInstanceData(origin, axisA, axisB, color, gridParams, boundsAVec, boundsBVec);
}

void PlaneRenderer::beginUpdates() {
  m_updatesDisabled = true;
}

void PlaneRenderer::endUpdates() {
  m_updatesDisabled = false;
  updateInstanceBuffer();
}

void PlaneRenderer::updateBuffers() {
  updateInstanceBuffer();
}

void PlaneRenderer::updateInstanceBuffer() {
  if (m_updatesDisabled || m_instanceData.empty())
    return;

  m_instanceBuffer.bind();
  m_instanceBuffer.allocate(m_instanceData.data(),
                           static_cast<int>(sizeof(PlaneInstanceData) * m_instanceData.size()));
  m_instanceBuffer.release();
}

void PlaneRenderer::draw() {
  qDebug() << "PlaneRenderer::draw() called with" << m_instanceData.size() << "instances, indices:" << m_numberOfIndices;
  
  if (m_numberOfIndices == 0 || m_instanceData.empty())
    return;
    
  // Use instanced drawing
  this->glDrawElementsInstanced(DrawType, m_numberOfIndices, IndexType, 0, 
                               static_cast<GLsizei>(m_instanceData.size()));
  
  qDebug() << "PlaneRenderer: Drew" << m_instanceData.size() << "plane instances";
}

void PlaneRenderer::setTexture(QOpenGLTexture *texture) {
  if (m_texture != texture) {
    delete m_texture;
    m_texture = texture;
  }
}