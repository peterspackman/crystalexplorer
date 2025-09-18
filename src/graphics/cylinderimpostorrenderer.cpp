#include "cylinderimpostorrenderer.h"
#include "shaderloader.h"
#include <QOpenGLShaderProgram>

CylinderImpostorRenderer::CylinderImpostorRenderer()
    : m_vertex(QOpenGLBuffer::VertexBuffer) {
  // Create Shader (Do not release until VAO is created)
  m_program = new QOpenGLShaderProgram();
  m_program->addShaderFromSourceCode(
      QOpenGLShader::Vertex,
      cx::shader::loadShaderFile(":/shaders/cylinder_impostor.vert"));
  m_program->addShaderFromSourceCode(
      QOpenGLShader::Fragment,
      cx::shader::loadShaderFile(":/shaders/cylinder_impostor.frag"));
  m_program->link();
  m_program->bind();

  // Create CylinderImpostorVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  // Create Index Buffer (Do not release until VAO is created)
  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  // Create CylinderImpostorVertex Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  m_program->enableAttributeArray(3);
  m_program->enableAttributeArray(4);
  m_program->enableAttributeArray(5);
  m_program->enableAttributeArray(6);
  m_program->setAttributeBuffer(0, GL_FLOAT,
                                CylinderImpostorVertex::pointAOffset(),
                                CylinderImpostorVertex::PointATupleSize,
                                CylinderImpostorVertex::stride());
  m_program->setAttributeBuffer(1, GL_FLOAT,
                                CylinderImpostorVertex::pointBOffset(),
                                CylinderImpostorVertex::PointBTupleSize,
                                CylinderImpostorVertex::stride());
  m_program->setAttributeBuffer(2, GL_FLOAT,
                                CylinderImpostorVertex::colorAOffset(),
                                CylinderImpostorVertex::ColorATupleSize,
                                CylinderImpostorVertex::stride());
  m_program->setAttributeBuffer(3, GL_FLOAT,
                                CylinderImpostorVertex::colorBOffset(),
                                CylinderImpostorVertex::ColorBTupleSize,
                                CylinderImpostorVertex::stride());
  m_program->setAttributeBuffer(4, GL_FLOAT,
                                CylinderImpostorVertex::mappingOffset(),
                                CylinderImpostorVertex::MappingTupleSize,
                                CylinderImpostorVertex::stride());
  m_program->setAttributeBuffer(5, GL_FLOAT,
                                CylinderImpostorVertex::selectionIdOffset(),
                                CylinderImpostorVertex::SelectionIdTupleSize,
                                CylinderImpostorVertex::stride());
  m_program->setAttributeBuffer(
      6, GL_FLOAT, CylinderImpostorVertex::radiusOffset(),
      CylinderImpostorVertex::RadiusSize, CylinderImpostorVertex::stride());

  // Release (unbind) all
  m_index.release();
  m_object.release();
  m_vertex.release();
  m_program->release();
}

CylinderImpostorRenderer::CylinderImpostorRenderer(
    const vector<CylinderImpostorVertex> &vertices)
    : m_vertex(QOpenGLBuffer::VertexBuffer) {
  // Create Shader (Do not release until VAO is created)
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceFile(
      QOpenGLShader::Vertex, ":/shaders/cylinder_impostor.vert");
  m_program->addCacheableShaderFromSourceFile(
      QOpenGLShader::Fragment, ":/shaders/cylinder_impostor.frag");
  m_program->link();
  m_program->bind();

  // Create CylinderImpostorVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  m_vertex.allocate(
      vertices.data(),
      static_cast<int>(sizeof(CylinderImpostorVertex) * vertices.size()));
  // Create Index Buffer (Do not release until VAO is created)
  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  addVertices(vertices);

  // Create CylinderImpostorVertex Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  m_program->enableAttributeArray(3);
  m_program->enableAttributeArray(4);
  m_program->enableAttributeArray(5);
  m_program->enableAttributeArray(6);
  m_program->setAttributeBuffer(0, GL_FLOAT,
                                CylinderImpostorVertex::pointAOffset(),
                                CylinderImpostorVertex::PointATupleSize,
                                CylinderImpostorVertex::stride());
  m_program->setAttributeBuffer(1, GL_FLOAT,
                                CylinderImpostorVertex::pointBOffset(),
                                CylinderImpostorVertex::PointBTupleSize,
                                CylinderImpostorVertex::stride());
  m_program->setAttributeBuffer(2, GL_FLOAT,
                                CylinderImpostorVertex::colorAOffset(),
                                CylinderImpostorVertex::ColorATupleSize,
                                CylinderImpostorVertex::stride());
  m_program->setAttributeBuffer(3, GL_FLOAT,
                                CylinderImpostorVertex::colorBOffset(),
                                CylinderImpostorVertex::ColorBTupleSize,
                                CylinderImpostorVertex::stride());
  m_program->setAttributeBuffer(4, GL_FLOAT,
                                CylinderImpostorVertex::mappingOffset(),
                                CylinderImpostorVertex::MappingTupleSize,
                                CylinderImpostorVertex::stride());
  m_program->setAttributeBuffer(5, GL_FLOAT,
                                CylinderImpostorVertex::selectionIdOffset(),
                                CylinderImpostorVertex::SelectionIdTupleSize,
                                CylinderImpostorVertex::stride());
  m_program->setAttributeBuffer(
      6, GL_FLOAT, CylinderImpostorVertex::radiusOffset(),
      CylinderImpostorVertex::RadiusSize, CylinderImpostorVertex::stride());

  // Release (unbind) all
  m_index.release();
  m_object.release();
  m_vertex.release();
  m_program->release();
}

void CylinderImpostorRenderer::addVertices(
    const vector<CylinderImpostorVertex> &vertices) {
  auto old_n = m_vertices.size();
  m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
  //    const auto mappingIndices = {
  //      0, 1, 2,
  //      1, 4, 2,
  //      2, 4, 3,
  //      4, 5, 3
  //    };
  // order = 0 1 2 1 4 2 2 4 3 4 5 3
  for (size_t i = old_n / 6; i < (m_vertices.size()) / 6; i++) {
    GLuint idx = static_cast<GLuint>(6 * i);
    m_indices.insert(m_indices.end(),
                     {idx, idx + 1, idx + 2, idx + 1, idx + 4, idx + 2, idx + 2,
                      idx + 4, idx + 3, idx + 4, idx + 5, idx + 3});
  }

  m_numberOfIndices = static_cast<GLsizei>(m_indices.size());
  updateBuffers();
}

void CylinderImpostorRenderer::clear() {
  if (m_vertices.size() > 1) {
    m_indices.clear();
    m_groups.clear();
    m_vertices.clear();
    m_numberOfIndices = 0;
    updateBuffers();
  }
}

void CylinderImpostorRenderer::beginUpdates() { m_updatesDisabled = true; }

void CylinderImpostorRenderer::endUpdates() {
  m_updatesDisabled = false;
  updateBuffers();
}

void CylinderImpostorRenderer::updateBuffers() {
  if (m_updatesDisabled)
    return;
  m_vertex.bind();
  m_index.bind();

  m_vertex.allocate(
      m_vertices.data(),
      static_cast<int>(sizeof(CylinderImpostorVertex) * m_vertices.size()));
  /* NEED TO DEAL WITH MORE THAN 65536 size buffer */
  m_index.allocate(m_indices.data(),
                   static_cast<int>(sizeof(GLuint) * m_indices.size()));
}

// Destructor
CylinderImpostorRenderer::~CylinderImpostorRenderer() {}

void CylinderImpostorRenderer::setRadii(float newRadius) {
  for (auto &vertex : m_vertices) {
    vertex.setRadius(newRadius);
  }
  updateBuffers();
}
