#include "ellipsoidrenderer.h"
#include "shaderloader.h"
#include <QFile>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>

EllipsoidRenderer::EllipsoidRenderer() {
  QOpenGLExtraFunctions f(QOpenGLContext::currentContext());
  // Create Shader (Do not release until VAO is created)
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceCode(
	  QOpenGLShader::Vertex,
          cx::shader::loadShaderFile(":/shaders/ellipsoid.vert")
  );
  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Fragment,
      cx::shader::loadShaderFile(":/shaders/ellipsoid.frag")
  );
  m_program->link();
  m_program->bind();

  // Create EllipsoidInstance Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::StaticDraw);

  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::StaticDraw);
  loadBaseMesh();

  m_instance.create();
  m_instance.bind();
  m_instance.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  m_vertex.bind();
  // Create EllipsoidInstance Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(QVector3D));
  m_vertex.release();

  m_object.release();
  m_instance.bind();
  m_object.bind();
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  m_program->enableAttributeArray(3);
  m_program->enableAttributeArray(4);
  m_program->enableAttributeArray(5);
  m_program->enableAttributeArray(6);

  m_program->setAttributeBuffer(
      1, GL_FLOAT, EllipsoidInstance::positionOffset(),
      EllipsoidInstance::PositionTupleSize, EllipsoidInstance::stride());
  f.glVertexAttribDivisor(1, 1);
  m_program->setAttributeBuffer(2, GL_FLOAT, EllipsoidInstance::aOffset(),
                                EllipsoidInstance::ATupleSize,
                                EllipsoidInstance::stride());
  f.glVertexAttribDivisor(2, 1);
  m_program->setAttributeBuffer(3, GL_FLOAT, EllipsoidInstance::bOffset(),
                                EllipsoidInstance::BTupleSize,
                                EllipsoidInstance::stride());
  f.glVertexAttribDivisor(3, 1);
  m_program->setAttributeBuffer(4, GL_FLOAT, EllipsoidInstance::cOffset(),
                                EllipsoidInstance::CTupleSize,
                                EllipsoidInstance::stride());
  f.glVertexAttribDivisor(4, 1);
  m_program->setAttributeBuffer(5, GL_FLOAT, EllipsoidInstance::colorOffset(),
                                EllipsoidInstance::ColorTupleSize,
                                EllipsoidInstance::stride());
  f.glVertexAttribDivisor(5, 1);
  m_program->setAttributeBuffer(
      6, GL_FLOAT, EllipsoidInstance::selectionIdOffset(),
      EllipsoidInstance::SelectionIdSize, EllipsoidInstance::stride());
  f.glVertexAttribDivisor(6, 1);
  // Release (unbind) all
  m_instance.release();
  m_index.release();
  m_object.release();
  m_vertex.release();
  m_program->release();
}

EllipsoidRenderer::EllipsoidRenderer(
    const vector<EllipsoidInstance> &instances) {
  QOpenGLExtraFunctions f(QOpenGLContext::currentContext());
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceCode(
	  QOpenGLShader::Vertex,
          cx::shader::loadShaderFile(":/shaders/ellipsoid.vert")
  );

  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Fragment,
      cx::shader::loadShaderFile(":/shaders/ellipsoid.frag"));
  m_program->link();
  m_program->bind();

  // Create EllipsoidInstance Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::StaticDraw);

  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::StaticDraw);
  loadBaseMesh();

  m_instance.create();
  m_instance.bind();
  m_instance.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  m_vertex.bind();
  // Create EllipsoidInstance Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(QVector3D));
  m_vertex.release();

  m_object.release();
  m_instance.bind();
  m_object.bind();
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  m_program->enableAttributeArray(3);
  m_program->enableAttributeArray(4);
  m_program->enableAttributeArray(5);
  m_program->enableAttributeArray(6);

  m_program->setAttributeBuffer(
      1, GL_FLOAT, EllipsoidInstance::positionOffset(),
      EllipsoidInstance::PositionTupleSize, EllipsoidInstance::stride());
  f.glVertexAttribDivisor(1, 1);
  m_program->setAttributeBuffer(2, GL_FLOAT, EllipsoidInstance::aOffset(),
                                EllipsoidInstance::ATupleSize,
                                EllipsoidInstance::stride());
  f.glVertexAttribDivisor(2, 1);
  m_program->setAttributeBuffer(3, GL_FLOAT, EllipsoidInstance::bOffset(),
                                EllipsoidInstance::BTupleSize,
                                EllipsoidInstance::stride());
  f.glVertexAttribDivisor(3, 1);
  m_program->setAttributeBuffer(4, GL_FLOAT, EllipsoidInstance::cOffset(),
                                EllipsoidInstance::CTupleSize,
                                EllipsoidInstance::stride());
  f.glVertexAttribDivisor(4, 1);
  m_program->setAttributeBuffer(5, GL_FLOAT, EllipsoidInstance::colorOffset(),
                                EllipsoidInstance::ColorTupleSize,
                                EllipsoidInstance::stride());
  f.glVertexAttribDivisor(5, 1);
  m_program->setAttributeBuffer(
      6, GL_FLOAT, EllipsoidInstance::selectionIdOffset(),
      EllipsoidInstance::SelectionIdSize, EllipsoidInstance::stride());
  f.glVertexAttribDivisor(6, 1);
  // Release (unbind) all
  m_instance.release();
  m_index.release();
  m_object.release();
  m_vertex.release();
  m_program->release();

  addInstances(instances);
}

void EllipsoidRenderer::loadBaseMesh() {
  QFile obj(":/mesh/icosphere.obj");
  obj.open(QIODevice::ReadOnly);
  if (!obj.isOpen())
    return;

  QTextStream stream(&obj);
  QStringList tokens;
  QString line;
  while (stream.readLineInto(&line)) {
    if (line.startsWith('v')) {
      tokens = line.split(' ');
      QVector3D pos(tokens[1].toFloat(), tokens[2].toFloat(),
                    tokens[3].toFloat());
      m_vertices.push_back(pos);
    } else if (line.startsWith('f')) {
      tokens = line.split(' ');
      m_faces.push_back({tokens[1].toUInt() - 1, tokens[2].toUInt() - 1,
                         tokens[3].toUInt() - 1});
    }
  }
  m_vertex.bind();
  m_index.bind();

  m_vertex.allocate(m_vertices.data(),
                    static_cast<int>(sizeof(QVector3D) * m_vertices.size()));
  m_index.allocate(m_faces.data(),
                   static_cast<int>(sizeof(Face) * m_faces.size()));
}

void EllipsoidRenderer::addInstances(
    const vector<EllipsoidInstance> &instances) {
  m_instances.insert(m_instances.end(), instances.begin(), instances.end());
  if (!m_updatesDisabled)
    updateBuffers();
}

void EllipsoidRenderer::addInstance(const EllipsoidInstance &instance) {
  m_instances.push_back(instance);
  if (!m_updatesDisabled)
    updateBuffers();
}

void EllipsoidRenderer::clear() {
  if (m_instances.size() > 1) {
    m_instances.clear();
    updateBuffers();
  }
}

void EllipsoidRenderer::draw() {
  QOpenGLExtraFunctions f(QOpenGLContext::currentContext());
  f.glDrawElementsInstanced(DrawType, m_faces.size() * 3, GL_UNSIGNED_INT, 0,
                            m_instances.size());
}

void EllipsoidRenderer::beginUpdates() { Renderer::beginUpdates(); }

void EllipsoidRenderer::endUpdates() {
  Renderer::endUpdates();
  updateBuffers();
}

void EllipsoidRenderer::updateBuffers() {
  if (m_updatesDisabled)
    return;
  m_instance.bind();
  m_instance.allocate(
      m_instances.data(),
      static_cast<int>(sizeof(EllipsoidInstance) * m_instances.size()));
}
