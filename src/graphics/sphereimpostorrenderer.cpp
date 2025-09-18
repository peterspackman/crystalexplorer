#include "sphereimpostorrenderer.h"
#include "shaderloader.h"
#include <QOpenGLShaderProgram>
#include <QUuid>

SphereImpostorRenderer::SphereImpostorRenderer()
    : m_vertex(QOpenGLBuffer::VertexBuffer) {
  // Create Shader (Do not release until VAO is created)
  m_program = new QOpenGLShaderProgram();
  m_program->addShaderFromSourceCode(
      QOpenGLShader::Vertex,
      cx::shader::loadShaderFile(":/shaders/sphere_impostor.vert"));
  m_program->addShaderFromSourceCode(
      QOpenGLShader::Fragment,
      cx::shader::loadShaderFile(":/shaders/sphere_impostor.frag"));
  m_program->link();
  m_program->bind();

  // Create SphereImpostorVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  // Create Index Buffer (Do not release until VAO is created)
  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  updateBuffers();

  // Create SphereImpostorVertex Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  m_program->enableAttributeArray(3);
  m_program->enableAttributeArray(4);
  m_program->setAttributeBuffer(
      0, GL_FLOAT, SphereImpostorVertex::positionOffset(),
      SphereImpostorVertex::PositionTupleSize, SphereImpostorVertex::stride());

  m_program->setAttributeBuffer(
      1, GL_FLOAT, SphereImpostorVertex::colorOffset(),
      SphereImpostorVertex::ColorTupleSize, SphereImpostorVertex::stride());
  m_program->setAttributeBuffer(
      2, GL_FLOAT, SphereImpostorVertex::radiusOffset(),
      SphereImpostorVertex::RadiusSize, SphereImpostorVertex::stride());
  m_program->setAttributeBuffer(
      3, GL_FLOAT, SphereImpostorVertex::texcoordOffset(),
      SphereImpostorVertex::TexcoordTupleSize, SphereImpostorVertex::stride());
  m_program->setAttributeBuffer(4, GL_FLOAT,
                                SphereImpostorVertex::selectionIdOffset(),
                                SphereImpostorVertex::SelectionIdTupleSize,
                                SphereImpostorVertex::stride());
  // Release (unbind) all
  m_object.release();
  m_vertex.release();
  m_program->release();
}

SphereImpostorRenderer::SphereImpostorRenderer(
    const vector<SphereImpostorVertex> &vertices)
    : m_vertex(QOpenGLBuffer::VertexBuffer) {
  // Create Shader (Do not release until VAO is created)
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex,
                                              ":/shaders/sphere_impostor.vert");
  m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment,
                                              ":/shaders/sphere_impostor.frag");
  m_program->link();
  m_program->bind();

  // Create SphereImpostorVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  // Create Index Buffer (Do not release until VAO is created)
  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  addVertices(vertices);

  // Create SphereImpostorVertex Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  m_program->enableAttributeArray(3);
  m_program->enableAttributeArray(4);
  m_program->setAttributeBuffer(
      0, GL_FLOAT, SphereImpostorVertex::positionOffset(),
      SphereImpostorVertex::PositionTupleSize, SphereImpostorVertex::stride());
  m_program->setAttributeBuffer(
      1, GL_FLOAT, SphereImpostorVertex::colorOffset(),
      SphereImpostorVertex::ColorTupleSize, SphereImpostorVertex::stride());
  m_program->setAttributeBuffer(
      2, GL_FLOAT, SphereImpostorVertex::radiusOffset(),
      SphereImpostorVertex::RadiusSize, SphereImpostorVertex::stride());
  m_program->setAttributeBuffer(
      3, GL_FLOAT, SphereImpostorVertex::texcoordOffset(),
      SphereImpostorVertex::TexcoordTupleSize, SphereImpostorVertex::stride());
  m_program->setAttributeBuffer(4, GL_FLOAT,
                                SphereImpostorVertex::selectionIdOffset(),
                                SphereImpostorVertex::SelectionIdTupleSize,
                                SphereImpostorVertex::stride());
  // Release (unbind) all
  m_object.release();
  m_vertex.release();
  m_program->release();
}

void SphereImpostorRenderer::addVertices(
    const vector<SphereImpostorVertex> &vertices) {
  if (vertices.size() > 0) {
    size_t old_n = m_vertices.size();
    m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());

    for (size_t i = old_n / 4; i < (m_vertices.size()) / 4; i++) {
      GLuint idx = static_cast<GLuint>(4 * i);
      m_atoms.emplace_back(4 * i, 4 * i + 4, m_vertices);
      m_indices.insert(m_indices.end(),
                       {idx, idx + 1, idx + 2, idx + 2, idx + 1, idx + 3});
    }

    m_numberOfIndices = static_cast<GLsizei>(m_indices.size());
    m_groups.emplace_back(old_n, m_vertices.size(), m_vertices);
  }
  updateBuffers();
}

void SphereImpostorRenderer::clear() {
  if (m_atoms.size() > 1) {
    m_atoms.clear();
    m_indices.clear();
    m_groups.clear();
    m_vertices.clear();
    m_numberOfIndices = 0;
    updateBuffers();
  }
}

void SphereImpostorRenderer::setRadii(float newRadius) {
  for (SphereImpostorVertex &vertex : m_vertices) {
    vertex.setRadius(newRadius);
  }
  updateBuffers();
}

void SphereImpostorRenderer::beginUpdates() { m_updatesDisabled = true; }

void SphereImpostorRenderer::endUpdates() {
  m_updatesDisabled = false;
  updateBuffers();
}

void SphereImpostorRenderer::updateBuffers() {
  if (m_updatesDisabled)
    return;
  if (!m_vertex.bind())
    qDebug() << "Failed to bind vertex buffer";
  if (!m_index.bind())
    qDebug() << "Failed to bind index buffer";
  m_vertex.allocate(
      m_vertices.data(),
      static_cast<int>(sizeof(SphereImpostorVertex) * m_vertices.size()));
  m_index.allocate(m_indices.data(),
                   static_cast<int>(sizeof(GLuint) * m_indices.size()));
}

SphereImpostorRenderer::~SphereImpostorRenderer() {}
