#include "pointcloudinstancerenderer.h"
#include "shaderloader.h"

#include <QOpenGLShaderProgram>

PointCloudInstanceRenderer::PointCloudInstanceRenderer(Mesh *mesh)
    : QOpenGLExtraFunctions(QOpenGLContext::currentContext()) {

  m_propertyColorMaps = {
      {"None", ColorMapName::CE_None},   {"dnorm", ColorMapName::CE_bwr},
      {"di", ColorMapName::CE_rgb},      {"de", ColorMapName::CE_rgb},
      {"di_norm", ColorMapName::CE_bwr}, {"de_norm", ColorMapName::CE_bwr},
      {"eeq_esp", ColorMapName::CE_bwr},
  };

  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Vertex,
      cx::shader::loadShaderFile(":/shaders/pointcloudinstance.vert"));

  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Fragment,
      cx::shader::loadShaderFile(":/shaders/pointcloudinstance.frag"));
  if (!m_program->link()) {
    qDebug() << "Shader link error:" << m_program->log();
  }
  m_program->bind();

  // Create MeshInstanceVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::StaticDraw);

  // vertex properties are stored in a buffer
  m_vertexPropertyBuffer.create();

  m_vertexPropertyTexture = new QOpenGLTexture(QOpenGLTexture::TargetBuffer);
  m_vertexPropertyTexture->create();

  setMesh(mesh);

  m_instance.create();
  m_instance.bind();
  m_instance.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  m_vertex.bind();
  // Create MeshInstanceVertex Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * sizeof(float));
  m_program->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3,
                                6 * sizeof(float));
  m_vertex.release();

  m_object.release();
  m_instance.bind();
  m_object.bind();
  m_program->enableAttributeArray(2);
  m_program->enableAttributeArray(3);
  m_program->enableAttributeArray(4);
  m_program->enableAttributeArray(5);
  m_program->enableAttributeArray(6);
  m_program->enableAttributeArray(7);
  m_program->enableAttributeArray(8);

  m_program->setAttributeBuffer(
      2, GL_FLOAT, MeshInstanceVertex::translationOffset(),
      MeshInstanceVertex::TranslationTupleSize, MeshInstanceVertex::stride());
  this->glVertexAttribDivisor(2, 1);
  m_program->setAttributeBuffer(
      3, GL_FLOAT, MeshInstanceVertex::rotation1Offset(),
      MeshInstanceVertex::RotationTupleSize, MeshInstanceVertex::stride());
  this->glVertexAttribDivisor(3, 1);
  m_program->setAttributeBuffer(
      4, GL_FLOAT, MeshInstanceVertex::rotation2Offset(),
      MeshInstanceVertex::RotationTupleSize, MeshInstanceVertex::stride());
  this->glVertexAttribDivisor(4, 1);
  m_program->setAttributeBuffer(
      5, GL_FLOAT, MeshInstanceVertex::rotation3Offset(),
      MeshInstanceVertex::RotationTupleSize, MeshInstanceVertex::stride());
  this->glVertexAttribDivisor(5, 1);

  m_program->setAttributeBuffer(
      6, GL_FLOAT, MeshInstanceVertex::selectionIdOffset(),
      MeshInstanceVertex::SelectionIdSize, MeshInstanceVertex::stride());
  this->glVertexAttribDivisor(6, 1);

  m_program->setAttributeBuffer(
      7, GL_FLOAT, MeshInstanceVertex::propertyIndexOffset(),
      MeshInstanceVertex::PropertyIndexSize, MeshInstanceVertex::stride());
  this->glVertexAttribDivisor(7, 1);

  m_program->setAttributeBuffer(8, GL_FLOAT, MeshInstanceVertex::alphaOffset(),
                                MeshInstanceVertex::AlphaSize,
                                MeshInstanceVertex::stride());
  this->glVertexAttribDivisor(8, 1);

  // Release (unbind) all
  m_instance.release();
  m_object.release();
  m_vertex.release();
  m_program->release();
}

void PointCloudInstanceRenderer::setMesh(Mesh *mesh) {
  m_vertex.bind();
  if (!mesh)
    return;

  const auto &vertices = mesh->vertices();
  const auto &faces = mesh->faces();
  const auto &vertexMask = mesh->vertexMask();

  std::vector<float> temp;
  temp.reserve(6 * vertices.cols());
  for (int i = 0; i < vertices.cols(); i++) {
    temp.push_back(vertices(0, i));
    temp.push_back(vertices(1, i));
    temp.push_back(vertices(2, i));
    temp.push_back(0.0);
    temp.push_back(0.0);
    temp.push_back(0.0);
  }
  m_numVertices = vertices.cols();

  m_vertex.allocate(temp.data(), static_cast<int>(sizeof(float) * temp.size()));

  {
    m_vertexPropertyBuffer.bind();
    std::vector<float> propertyData;
    m_availableProperties = mesh->availableVertexProperties();
    for (const auto &prop : m_availableProperties) {

      const auto &vals = mesh->vertexProperty(prop);
      auto range = mesh->vertexPropertyRange(prop);

      ColorMapFunc cmap(m_propertyColorMaps.value(prop, ColorMapName::Viridis),
                        range.lower, range.upper);

      for (size_t i = 0; i < vals.rows(); i++) {
        QColor color = cmap(vals(i));
        if(!vertexMask(i)) color = color.darker();
        propertyData.push_back(color.redF());
        propertyData.push_back(color.greenF());
        propertyData.push_back(color.blueF());
        propertyData.push_back(color.alphaF());
      }
    }
    m_vertexPropertyBuffer.allocate(
        propertyData.data(),
        static_cast<int>(sizeof(float) * propertyData.size()));
    m_vertexPropertyBuffer.release();

    m_vertexPropertyTexture->bind();
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F,
                m_vertexPropertyBuffer.bufferId());
    m_vertexPropertyTexture->release();
  }

  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    // Log or handle the error
    qDebug() << "OpenGL error (setMesh):" << err;
  }
}

void PointCloudInstanceRenderer::addInstances(
    const std::vector<MeshInstanceVertex> &instances) {
  m_instances.insert(m_instances.end(), instances.begin(), instances.end());
  if (!m_updatesDisabled)
    updateBuffers();
}

void PointCloudInstanceRenderer::addInstance(const MeshInstanceVertex &instance) {
  m_instances.push_back(instance);
  if (!m_updatesDisabled)
    updateBuffers();
}

void PointCloudInstanceRenderer::clear() {
  if (m_instances.size() > 1) {
    m_instances.clear();
    updateBuffers();
  }
}

void PointCloudInstanceRenderer::draw() {
  // After linking the program and before rendering
  m_program->setUniformValue("u_propertyBuffer", 0);
  m_program->setUniformValue("u_numVertices", m_numVertices);
  m_vertexPropertyTexture->bind();

  this->glEnable(GL_PROGRAM_POINT_SIZE);
  this->glDrawArraysInstanced(GL_POINTS, 0, m_numVertices,
                                static_cast<int>(m_instances.size()));
  this->glDisable(GL_PROGRAM_POINT_SIZE);

  m_vertexPropertyTexture->release();
}

void PointCloudInstanceRenderer::beginUpdates() { Renderer::beginUpdates(); }

void PointCloudInstanceRenderer::endUpdates() {
  Renderer::endUpdates();
  updateBuffers();
}

void PointCloudInstanceRenderer::updateBuffers() {
  if (m_updatesDisabled)
    return;
  m_instance.bind();
  m_instance.allocate(
      m_instances.data(),
      static_cast<int>(sizeof(MeshInstanceVertex) * m_instances.size()));
}
