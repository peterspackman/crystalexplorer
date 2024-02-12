#include "cylinderrenderer.h"
#include "shaderloader.h"
#include <QFile>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>

CylinderRenderer::CylinderRenderer() {
  QOpenGLExtraFunctions f(QOpenGLContext::currentContext());

  // Create Shader (Do not release until VAO is created)
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex,
                                              cx::shader::loadShaderFile(":/shaders/cylinder.vert"));
  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Fragment,
      cx::shader::loadShaderFile(":/shaders/cylinder.frag"));
  m_program->link();
  m_program->bind();

  m_shader_programs.insert(ShadingMode::PBR, m_program);

  // Create CylinderInstance Buffer (Do not release until VAO is created)
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
  // Create CylinderInstance Array Object
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
  m_program->enableAttributeArray(7);

  m_program->setAttributeBuffer(1, GL_FLOAT, CylinderInstance::radiusOffset(),
                                CylinderInstance::RadiusSize,
                                CylinderInstance::stride());
  f.glVertexAttribDivisor(1, 1);
  m_program->setAttributeBuffer(2, GL_FLOAT, CylinderInstance::aOffset(),
                                CylinderInstance::ATupleSize,
                                CylinderInstance::stride());
  f.glVertexAttribDivisor(2, 1);
  m_program->setAttributeBuffer(3, GL_FLOAT, CylinderInstance::bOffset(),
                                CylinderInstance::BTupleSize,
                                CylinderInstance::stride());
  f.glVertexAttribDivisor(3, 1);
  m_program->setAttributeBuffer(4, GL_FLOAT, CylinderInstance::colorAOffset(),
                                CylinderInstance::ColorATupleSize,
                                CylinderInstance::stride());
  f.glVertexAttribDivisor(4, 1);
  m_program->setAttributeBuffer(5, GL_FLOAT, CylinderInstance::colorBOffset(),
                                CylinderInstance::ColorBTupleSize,
                                CylinderInstance::stride());
  f.glVertexAttribDivisor(5, 1);
  m_program->setAttributeBuffer(
      6, GL_FLOAT, CylinderInstance::selectionIdAOffset(),
      CylinderInstance::SelectionIdASize, CylinderInstance::stride());
  f.glVertexAttribDivisor(6, 1);
  m_program->setAttributeBuffer(
      7, GL_FLOAT, CylinderInstance::selectionIdBOffset(),
      CylinderInstance::SelectionIdBSize, CylinderInstance::stride());
  f.glVertexAttribDivisor(7, 1);
  // Release (unbind) all
  m_instance.release();
  m_index.release();
  m_object.release();
  m_vertex.release();
  m_program->release();
}

CylinderRenderer::CylinderRenderer(const vector<CylinderInstance> &instances) {
  QOpenGLExtraFunctions f(QOpenGLContext::currentContext());
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex,
                                              cx::shader::loadShaderFile(":/shaders/cylinder.vert"));
  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Fragment,
      cx::shader::loadShaderFile(":/shaders/cylinder.frag"));
  m_program->link();
  m_program->bind();

  // Create CylinderInstance Buffer (Do not release until VAO is created)
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
  // Create CylinderInstance Array Object
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
  m_program->enableAttributeArray(7);

  m_program->setAttributeBuffer(1, GL_FLOAT, CylinderInstance::radiusOffset(),
                                CylinderInstance::RadiusSize,
                                CylinderInstance::stride());
  f.glVertexAttribDivisor(1, 1);
  m_program->setAttributeBuffer(2, GL_FLOAT, CylinderInstance::aOffset(),
                                CylinderInstance::ATupleSize,
                                CylinderInstance::stride());
  f.glVertexAttribDivisor(2, 1);
  m_program->setAttributeBuffer(3, GL_FLOAT, CylinderInstance::bOffset(),
                                CylinderInstance::BTupleSize,
                                CylinderInstance::stride());
  f.glVertexAttribDivisor(3, 1);
  m_program->setAttributeBuffer(4, GL_FLOAT, CylinderInstance::colorAOffset(),
                                CylinderInstance::ColorATupleSize,
                                CylinderInstance::stride());
  f.glVertexAttribDivisor(4, 1);
  m_program->setAttributeBuffer(5, GL_FLOAT, CylinderInstance::colorBOffset(),
                                CylinderInstance::ColorBTupleSize,
                                CylinderInstance::stride());
  f.glVertexAttribDivisor(5, 1);
  m_program->setAttributeBuffer(
      6, GL_FLOAT, CylinderInstance::selectionIdAOffset(),
      CylinderInstance::SelectionIdASize, CylinderInstance::stride());
  f.glVertexAttribDivisor(6, 1);
  m_program->setAttributeBuffer(
      7, GL_FLOAT, CylinderInstance::selectionIdBOffset(),
      CylinderInstance::SelectionIdBSize, CylinderInstance::stride());
  f.glVertexAttribDivisor(7, 1);
  // Release (unbind) all
  m_instance.release();
  m_index.release();
  m_object.release();
  m_vertex.release();
  m_program->release();

  addInstances(instances);
}

void CylinderRenderer::loadBaseMesh() {
  QFile obj(":/mesh/cylinder.obj");
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

void CylinderRenderer::addInstances(const vector<CylinderInstance> &instances) {
  m_instances.insert(m_instances.end(), instances.begin(), instances.end());
  if (!m_updatesDisabled)
    updateBuffers();
}

void CylinderRenderer::addInstance(const CylinderInstance &instance) {
  m_instances.push_back(instance);
  if (!m_updatesDisabled)
    updateBuffers();
}

void CylinderRenderer::clear() {
  if (m_instances.size() > 1) {
    m_instances.clear();
    updateBuffers();
  }
}

void CylinderRenderer::draw() {
  QOpenGLExtraFunctions f(QOpenGLContext::currentContext());
  f.glDrawElementsInstanced(DrawType, m_faces.size() * 3, GL_UNSIGNED_INT, 0,
                            m_instances.size());
}

void CylinderRenderer::beginUpdates() { Renderer::beginUpdates(); }

void CylinderRenderer::endUpdates() {
  Renderer::endUpdates();
  updateBuffers();
}

void CylinderRenderer::updateBuffers() {
  if (m_updatesDisabled)
    return;
  m_instance.bind();
  m_instance.allocate(
      m_instances.data(),
      static_cast<int>(sizeof(CylinderInstance) * m_instances.size()));
}
