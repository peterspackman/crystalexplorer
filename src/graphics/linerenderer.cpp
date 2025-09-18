#include "linerenderer.h"
#include "shaderloader.h"
#include <QOpenGLShaderProgram>

LineRenderer::LineRenderer() {
  // Create Shader (Do not release until VAO is created)
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Vertex, cx::shader::loadShaderFile(":/shaders/line.vert"));
  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Fragment,
      cx::shader::loadShaderFile(":/shaders/line.frag"));
  m_program->link();
  m_program->bind();

  // Create LineVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  m_index.create();
  m_index.bind();

  m_index.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  // Create LineVertex Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  m_program->enableAttributeArray(3);
  m_program->enableAttributeArray(4);
  m_program->enableAttributeArray(5);
  m_program->enableAttributeArray(6);

  m_program->setAttributeBuffer(0, GL_FLOAT, LineVertex::pointAOffset(),
                                LineVertex::PointATupleSize,
                                LineVertex::stride());
  m_program->setAttributeBuffer(1, GL_FLOAT, LineVertex::pointBOffset(),
                                LineVertex::PointBTupleSize,
                                LineVertex::stride());
  m_program->setAttributeBuffer(2, GL_FLOAT, LineVertex::colorAOffset(),
                                LineVertex::ColorATupleSize,
                                LineVertex::stride());
  m_program->setAttributeBuffer(3, GL_FLOAT, LineVertex::colorBOffset(),
                                LineVertex::ColorBTupleSize,
                                LineVertex::stride());
  m_program->setAttributeBuffer(4, GL_FLOAT, LineVertex::texcoordOffset(),
                                LineVertex::TexcoordTupleSize,
                                LineVertex::stride());
  m_program->setAttributeBuffer(5, GL_FLOAT, LineVertex::lineWidthOffset(),
                                LineVertex::LineWidthSize,
                                LineVertex::stride());
  m_program->setAttributeBuffer(6, GL_FLOAT, LineVertex::selectionColorOffset(),
                                LineVertex::SelectionColorTupleSize,
                                LineVertex::stride());
  // Release (unbind) all
  m_index.release();
  m_object.release();
  m_vertex.release();
  m_program->release();
}

LineRenderer::LineRenderer(const vector<LineVertex> &vertices)
    : m_vertex(QOpenGLBuffer::VertexBuffer) {
  // Create Shader (Do not release until VAO is created)
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Vertex, cx::shader::loadShaderFile(":/shaders/line.vert"));
  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Fragment,
      cx::shader::loadShaderFile(":/shaders/line.frag"));
  m_program->link();
  m_program->bind();

  // Create LineVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  m_vertex.allocate(vertices.data(),
                    static_cast<int>(sizeof(LineVertex) * vertices.size()));

  m_index.create();
  m_index.bind();

  m_index.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  addLines(vertices);

  // Create LineVertex Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  m_program->enableAttributeArray(3);
  m_program->enableAttributeArray(4);
  m_program->enableAttributeArray(5);
  m_program->enableAttributeArray(6);

  m_program->setAttributeBuffer(0, GL_FLOAT, LineVertex::pointAOffset(),
                                LineVertex::PointATupleSize,
                                LineVertex::stride());
  m_program->setAttributeBuffer(1, GL_FLOAT, LineVertex::pointBOffset(),
                                LineVertex::PointBTupleSize,
                                LineVertex::stride());
  m_program->setAttributeBuffer(2, GL_FLOAT, LineVertex::colorAOffset(),
                                LineVertex::ColorATupleSize,
                                LineVertex::stride());
  m_program->setAttributeBuffer(3, GL_FLOAT, LineVertex::colorBOffset(),
                                LineVertex::ColorBTupleSize,
                                LineVertex::stride());
  m_program->setAttributeBuffer(4, GL_FLOAT, LineVertex::texcoordOffset(),
                                LineVertex::TexcoordTupleSize,
                                LineVertex::stride());
  m_program->setAttributeBuffer(5, GL_FLOAT, LineVertex::lineWidthOffset(),
                                LineVertex::LineWidthSize,
                                LineVertex::stride());
  m_program->setAttributeBuffer(6, GL_FLOAT, LineVertex::selectionColorOffset(),
                                LineVertex::SelectionColorTupleSize,
                                LineVertex::stride());

  // Release (unbind) all
  m_index.release();
  m_object.release();
  m_vertex.release();
  m_program->release();
}

void LineRenderer::addLines(const vector<LineVertex> &vertices) {
  auto old_n = m_vertices.size();
  m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
  // vertex ordering = 0 1 2 1 3 2
  for (size_t i = old_n / 4; i < (m_vertices.size()) / 4; i++) {
    GLuint idx = static_cast<GLuint>(4 * i);
    m_indices.insert(m_indices.end(),
                     {idx, idx + 1, idx + 2, idx + 1, idx + 3, idx + 2});
  }

  m_numberOfIndices = static_cast<GLsizei>(m_indices.size());
  updateBuffers();
}

void LineRenderer::clear() {
  if (m_vertices.size() > 1) {
    m_indices.clear();
    m_vertices.clear();
    m_numberOfIndices = 0;
    updateBuffers();
  }
}

void LineRenderer::beginUpdates() { Renderer::beginUpdates(); }

void LineRenderer::endUpdates() {
  Renderer::endUpdates();
  updateBuffers();
}

void LineRenderer::updateBuffers() {
  if (m_updatesDisabled)
    return;
  m_vertex.bind();
  m_index.bind();

  m_vertex.allocate(m_vertices.data(),
                    static_cast<int>(sizeof(LineVertex) * m_vertices.size()));
  /* NEED TO DEAL WITH MORE THAN 65536 size buffer */
  m_index.allocate(m_indices.data(),
                   static_cast<int>(sizeof(GLuint) * m_indices.size()));
}

// Destructor
LineRenderer::~LineRenderer() {}
