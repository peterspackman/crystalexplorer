#include "pointcloudrenderer.h"
#include "shaderloader.h"
#include <QFile>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>

PointCloudRenderer::PointCloudRenderer()
    : m_vertex(QOpenGLBuffer::VertexBuffer) {
  // Create Shader (Do not release until VAO is created)
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Vertex,
      cx::shader::loadShaderFile(":/shaders/point.vert"));
  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Fragment,
      cx::shader::loadShaderFile(":/shaders/point.frag"));
  m_program->link();
  m_program->bind();

  // Create PointCloudVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  // Create CircleVertex Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->setAttributeBuffer(0, GL_FLOAT, PointCloudVertex::positionOffset(),
                                PointCloudVertex::PositionSize,
                                PointCloudVertex::stride());
  m_program->setAttributeBuffer(1, GL_FLOAT, PointCloudVertex::colorOffset(),
                                PointCloudVertex::ColorSize,
                                PointCloudVertex::stride());
  // Release (unbind) all
  m_object.release();
  m_vertex.release();
  m_program->release();
}

PointCloudRenderer::PointCloudRenderer(const vector<PointCloudVertex> &points)
    : m_vertex(QOpenGLBuffer::VertexBuffer) {
  // Create Shader (Do not release until VAO is created)
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Vertex,
      cx::shader::loadShaderFile(":/shaders/point.vert"));
  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Fragment,
      cx::shader::loadShaderFile(":/shaders/point.frag"));
  m_program->link();
  m_program->bind();

  // Create PointCloudVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  // Create CircleVertex Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->setAttributeBuffer(0, GL_FLOAT, PointCloudVertex::positionOffset(),
                                PointCloudVertex::PositionSize,
                                PointCloudVertex::stride());
  m_program->setAttributeBuffer(1, GL_FLOAT, PointCloudVertex::colorOffset(),
                                PointCloudVertex::ColorSize,
                                PointCloudVertex::stride());
  // Release (unbind) all
  m_object.release();
  m_vertex.release();
  m_program->release();

  addPoints(points);
}

void PointCloudRenderer::addPoints(const vector<PointCloudVertex> &points) {
  m_points.insert(m_points.end(), points.begin(), points.end());
  if (!m_updatesDisabled)
    updateBuffers();
}

void PointCloudRenderer::addPoint(const PointCloudVertex &point) {
  m_points.push_back(point);
  if (!m_updatesDisabled)
    updateBuffers();
}

void PointCloudRenderer::clear() {
  if (m_points.size() > 1) {
    m_points.clear();
    updateBuffers();
  }
}

void PointCloudRenderer::draw() {
  m_program->setUniformValue("u_alpha", m_alpha);
  glEnable(GL_PROGRAM_POINT_SIZE);
  glDrawArrays(GL_POINTS, 0, m_points.size());
}

void PointCloudRenderer::beginUpdates() { Renderer::beginUpdates(); }

void PointCloudRenderer::endUpdates() {
  Renderer::endUpdates();
  updateBuffers();
}

void PointCloudRenderer::updateBuffers() {
  if (m_updatesDisabled)
    return;
  m_vertex.bind();
  m_vertex.allocate(m_points.data(), static_cast<int>(sizeof(PointCloudVertex) *
                                                      m_points.size()));
  qDebug() << "Allocated " << m_points.size() << "points for point cloud";
}
