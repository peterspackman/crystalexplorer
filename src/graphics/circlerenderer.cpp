#include "circlerenderer.h"
#include <QOpenGLShaderProgram>

CircleRenderer::CircleRenderer() : m_vertex(QOpenGLBuffer::VertexBuffer) {
  // Create Shader (Do not release until VAO is created)
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex,
                                              ":/shaders/ellipse.vert");
  m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment,
                                              ":/shaders/ellipse.frag");
  m_program->link();
  m_program->bind();

  // Create CircleVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  // Create Index Buffer (Do not release until VAO is created)
  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  // Create CircleVertex Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  m_program->enableAttributeArray(3);
  m_program->enableAttributeArray(4);
  m_program->enableAttributeArray(5);
  m_program->enableAttributeArray(6);
  m_program->setAttributeBuffer(0, GL_FLOAT, CircleVertex::positionOffset(),
                                CircleVertex::PositionTupleSize,
                                CircleVertex::stride());
  m_program->setAttributeBuffer(1, GL_FLOAT, CircleVertex::rightOffset(),
                                CircleVertex::RightTupleSize,
                                CircleVertex::stride());
  m_program->setAttributeBuffer(2, GL_FLOAT, CircleVertex::upOffset(),
                                CircleVertex::UpTupleSize,
                                CircleVertex::stride());
  m_program->setAttributeBuffer(3, GL_FLOAT, CircleVertex::colorOffset(),
                                CircleVertex::ColorTupleSize,
                                CircleVertex::stride());
  m_program->setAttributeBuffer(4, GL_FLOAT, CircleVertex::texcoordOffset(),
                                CircleVertex::TexcoordTupleSize,
                                CircleVertex::stride());
  m_program->setAttributeBuffer(5, GL_FLOAT, CircleVertex::innerRadiusOffset(),
                                CircleVertex::InnerRadiusSize,
                                CircleVertex::stride());
  m_program->setAttributeBuffer(6, GL_FLOAT, CircleVertex::maxAngleOffset(),
                                CircleVertex::MaxAngleSize,
                                CircleVertex::stride());
  // Release (unbind) all
  m_index.release();
  m_object.release();
  m_vertex.release();
  m_program->release();
}

CircleRenderer::CircleRenderer(const vector<CircleVertex> &vertices)
    : m_vertex(QOpenGLBuffer::VertexBuffer) {
  // Create Shader (Do not release until VAO is created)
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex,
                                              ":/shaders/ellipse.vert");
  m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment,
                                              ":/shaders/ellipse.frag");
  m_program->link();
  m_program->bind();

  // Create CircleVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  // Create Index Buffer (Do not release until VAO is created)
  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  addVertices(vertices);

  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  m_program->enableAttributeArray(3);
  m_program->enableAttributeArray(4);
  m_program->enableAttributeArray(5);
  m_program->enableAttributeArray(6);
  m_program->setAttributeBuffer(0, GL_FLOAT, CircleVertex::positionOffset(),
                                CircleVertex::PositionTupleSize,
                                CircleVertex::stride());
  m_program->setAttributeBuffer(1, GL_FLOAT, CircleVertex::rightOffset(),
                                CircleVertex::RightTupleSize,
                                CircleVertex::stride());
  m_program->setAttributeBuffer(2, GL_FLOAT, CircleVertex::upOffset(),
                                CircleVertex::UpTupleSize,
                                CircleVertex::stride());
  m_program->setAttributeBuffer(3, GL_FLOAT, CircleVertex::colorOffset(),
                                CircleVertex::ColorTupleSize,
                                CircleVertex::stride());
  m_program->setAttributeBuffer(4, GL_FLOAT, CircleVertex::texcoordOffset(),
                                CircleVertex::TexcoordTupleSize,
                                CircleVertex::stride());
  m_program->setAttributeBuffer(5, GL_FLOAT, CircleVertex::innerRadiusOffset(),
                                CircleVertex::InnerRadiusSize,
                                CircleVertex::stride());
  m_program->setAttributeBuffer(6, GL_FLOAT, CircleVertex::maxAngleOffset(),
                                CircleVertex::MaxAngleSize,
                                CircleVertex::stride());
  // Release (unbind) all
  m_index.release();
  m_object.release();
  m_vertex.release();
  m_program->release();
}

void CircleRenderer::addVertices(const vector<CircleVertex> &vertices) {
  if (vertices.size() > 0) {
    size_t old_n = m_vertices.size();
    m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());

    // draw both sides (i.e. clockwise and counter-clockwise)
    for (size_t i = old_n / 4; i < (m_vertices.size()) / 4; i++) {
      GLuint idx = static_cast<GLuint>(4 * i);
      m_indices.insert(m_indices.end(),
                       {idx + 0, idx + 1, idx + 3, idx + 1, idx + 2, idx + 3,
                        idx + 3, idx + 1, idx + 0, idx + 3, idx + 2, idx + 1});
    }

    m_numberOfIndices = static_cast<GLsizei>(m_indices.size());
  }
  updateBuffers();
}

void CircleRenderer::clear() {
  if (m_vertices.size() > 1) {
    m_indices.clear();
    m_vertices.clear();
    m_numberOfIndices = 0;
    updateBuffers();
  }
}

void CircleRenderer::beginUpdates() { m_updatesDisabled = true; }

void CircleRenderer::endUpdates() {
  m_updatesDisabled = false;
  updateBuffers();
}

void CircleRenderer::updateBuffers() {
  if (m_updatesDisabled)
    return;
  if (m_vertices.size() == 0)
    return;
  if (!m_vertex.bind())
    qDebug() << "Failed to bind vertex buffer";
  if (!m_index.bind())
    qDebug() << "Failed to bind index buffer";
  m_vertex.allocate(m_vertices.data(),
                    static_cast<int>(sizeof(CircleVertex) * m_vertices.size()));
  m_index.allocate(m_indices.data(),
                   static_cast<int>(sizeof(GLuint) * m_indices.size()));
}

CircleRenderer::~CircleRenderer() {}
