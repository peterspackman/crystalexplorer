#include "crystalplanerenderer.h"
#include <QOpenGLShaderProgram>

CrystalPlaneRenderer::CrystalPlaneRenderer()
    : m_vertex(QOpenGLBuffer::VertexBuffer) {
  // Create Shader (Do not release until VAO is created)
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex,
                                              ":/shaders/crystalplane.vert");
  m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment,
                                              ":/shaders/crystalplane.frag");
  m_program->link();
  m_program->bind();

  // Create CrystalPlaneVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  // Create Index Buffer (Do not release until VAO is created)
  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  // Create CrystalPlaneVertex Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  m_program->enableAttributeArray(3);
  m_program->enableAttributeArray(4);
  m_program->setAttributeBuffer(
      0, GL_FLOAT, CrystalPlaneVertex::positionOffset(),
      CrystalPlaneVertex::PositionTupleSize, CrystalPlaneVertex::stride());
  m_program->setAttributeBuffer(1, GL_FLOAT, CrystalPlaneVertex::rightOffset(),
                                CrystalPlaneVertex::RightTupleSize,
                                CrystalPlaneVertex::stride());
  m_program->setAttributeBuffer(2, GL_FLOAT, CrystalPlaneVertex::upOffset(),
                                CrystalPlaneVertex::UpTupleSize,
                                CrystalPlaneVertex::stride());
  m_program->setAttributeBuffer(3, GL_FLOAT, CrystalPlaneVertex::colorOffset(),
                                CrystalPlaneVertex::ColorTupleSize,
                                CrystalPlaneVertex::stride());
  m_program->setAttributeBuffer(
      4, GL_FLOAT, CrystalPlaneVertex::texcoordOffset(),
      CrystalPlaneVertex::TexcoordTupleSize, CrystalPlaneVertex::stride());

  // Release (unbind) all
  m_index.release();
  m_object.release();
  m_vertex.release();
  m_program->release();
}

void CrystalPlaneRenderer::addVertices(
    const vector<CrystalPlaneVertex> &vertices) {
  if (vertices.size() > 0) {
    size_t old_n = m_vertices.size();
    m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());

    // draw both sides (i.e. clockwise and counter-clockwise)
    for (size_t i = old_n / 4; i < (m_vertices.size()) / 4; i++) {
      GLuint idx = static_cast<GLuint>(4 * i);
      m_indices.insert(m_indices.end(),
                       {idx + 0, idx + 1, idx + 2, idx + 0, idx + 2, idx + 3,
                        idx + 2, idx + 1, idx + 0, idx + 3, idx + 2, idx + 0});
    }

    m_numberOfIndices = static_cast<GLsizei>(m_indices.size());
  }
  updateBuffers();
}

void CrystalPlaneRenderer::clear() {
  if (m_vertices.size() > 1) {
    m_indices.clear();
    m_vertices.clear();
    m_numberOfIndices = 0;
    updateBuffers();
  }
}

void CrystalPlaneRenderer::beginUpdates() { m_updatesDisabled = true; }

void CrystalPlaneRenderer::endUpdates() {
  m_updatesDisabled = false;
  updateBuffers();
}

void CrystalPlaneRenderer::updateBuffers() {
  if (m_updatesDisabled)
    return;
  if (m_vertices.size() == 0)
    return;
  if (!m_vertex.bind())
    qDebug() << "Failed to bind vertex buffer";
  if (!m_index.bind())
    qDebug() << "Failed to bind index buffer";
  m_vertex.allocate(
      m_vertices.data(),
      static_cast<int>(sizeof(CrystalPlaneVertex) * m_vertices.size()));
  m_index.allocate(m_indices.data(),
                   static_cast<int>(sizeof(GLuint) * m_indices.size()));
}

CrystalPlaneRenderer::~CrystalPlaneRenderer() {}
