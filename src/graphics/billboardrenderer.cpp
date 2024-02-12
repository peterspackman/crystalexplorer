#include "billboardrenderer.h"
#include <QOpenGLShaderProgram>
#include <QPainter>

BillboardRenderer::BillboardRenderer() : m_vertex(QOpenGLBuffer::VertexBuffer) {
  // Create Shader (Do not release until VAO is created)
  m_id = "Billboard-" + Renderer::generateId();

  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex,
                                              ":/shaders/billboard.vert");
  m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment,
                                              ":/shaders/billboard.frag");
  m_program->link();
  m_program->bind();

  // Create BillboardVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  // Create Index Buffer (Do not release until VAO is created)
  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  updateBuffers();

  // Create BillboardVertex Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  m_program->enableAttributeArray(3);
  m_program->setAttributeBuffer(0, GL_FLOAT, BillboardVertex::positionOffset(),
                                BillboardVertex::PositionTupleSize,
                                BillboardVertex::stride());
  m_program->setAttributeBuffer(
      1, GL_FLOAT, BillboardVertex::dimensionsOffset(),
      BillboardVertex::DimensionsTupleSize, BillboardVertex::stride());
  m_program->setAttributeBuffer(2, GL_FLOAT, BillboardVertex::alphaOffset(),
                                BillboardVertex::AlphaSize,
                                BillboardVertex::stride());
  m_program->setAttributeBuffer(3, GL_FLOAT, BillboardVertex::texcoordOffset(),
                                BillboardVertex::TexcoordTupleSize,
                                BillboardVertex::stride());
  // Release (unbind) all
  m_object.release();
  m_vertex.release();
  m_program->release();
}

BillboardRenderer::BillboardRenderer(const vector<BillboardVertex> &vertices,
                                     const QString &label)
    : m_vertex(QOpenGLBuffer::VertexBuffer) {

  // Create Shader (Do not release until VAO is created)
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex,
                                              ":/shaders/billboard.vert");
  m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment,
                                              ":/shaders/billboard.frag");
  m_program->link();
  m_program->bind();

  // Create BillboardVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  // Create Index Buffer (Do not release until VAO is created)
  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  addVertices(vertices, label);

  // Create BillboardVertex Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  m_program->enableAttributeArray(3);
  m_program->setAttributeBuffer(0, GL_FLOAT, BillboardVertex::positionOffset(),
                                BillboardVertex::PositionTupleSize,
                                BillboardVertex::stride());
  m_program->setAttributeBuffer(
      1, GL_FLOAT, BillboardVertex::dimensionsOffset(),
      BillboardVertex::DimensionsTupleSize, BillboardVertex::stride());
  m_program->setAttributeBuffer(2, GL_FLOAT, BillboardVertex::alphaOffset(),
                                BillboardVertex::AlphaSize,
                                BillboardVertex::stride());
  m_program->setAttributeBuffer(3, GL_FLOAT, BillboardVertex::texcoordOffset(),
                                BillboardVertex::TexcoordTupleSize,
                                BillboardVertex::stride());
  // Release (unbind) all
  m_object.release();
  m_vertex.release();
  m_program->release();
}

void BillboardRenderer::addVertices(const vector<BillboardVertex> &vertices,
                                    const QString &label) {
  if (vertices.size() > 0) {
    size_t old_n = m_vertices.size();
    std::vector<BillboardVertex> verts_altered = vertices;
    const auto *texture = m_textures[label];
    for (auto &vert : verts_altered) {
      QVector2D tc = vert.dimensions();
      tc.setX(tc.x() * texture->width());
      tc.setY(tc.y() * texture->height());
      vert.setDimensions(tc);
    }
    m_vertices.insert(m_vertices.end(), verts_altered.begin(),
                      verts_altered.end());

    for (size_t i = old_n / 4; i < (m_vertices.size()) / 4; i++) {
      GLuint idx = static_cast<GLuint>(4 * i);
      m_indices.insert(m_indices.end(),
                       {idx, idx + 1, idx + 2, idx + 2, idx + 1, idx + 3});
      m_labels.push_back(label);
    }

    m_numberOfIndices = static_cast<GLsizei>(m_indices.size());
  }
  updateBuffers();
}

void BillboardRenderer::addVertices(const vector<BillboardVertex> &vertices,
                                    const QString &label,
                                    QOpenGLTexture *texture) {
  m_textures[label] = texture;
  if (vertices.size() > 0) {
    size_t old_n = m_vertices.size();
    m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());

    for (size_t i = old_n / 4; i < (m_vertices.size()) / 4; i++) {
      GLuint idx = static_cast<GLuint>(4 * i);
      m_indices.insert(m_indices.end(),
                       {idx, idx + 1, idx + 2, idx + 2, idx + 1, idx + 3});
      m_labels.push_back(label);
    }

    m_numberOfIndices = static_cast<GLsizei>(m_indices.size());
  }
  updateBuffers();
}

void BillboardRenderer::clear() {
  if (m_vertices.size() > 1) {
    m_indices.clear();
    m_vertices.clear();
    m_labels.clear();
    m_numberOfIndices = 0;
    updateBuffers();
  }
}

void BillboardRenderer::beginUpdates() { m_updatesDisabled = true; }

void BillboardRenderer::endUpdates() {
  m_updatesDisabled = false;
  updateBuffers();
}

void BillboardRenderer::updateBuffers() {
  if (m_updatesDisabled)
    return;
  if (!m_vertex.bind())
    qDebug() << "Failed to bind vertex buffer";
  if (!m_index.bind())
    qDebug() << "Failed to bind index buffer";
  //    qDebug() << "Updating buffers for " << m_id << ": " << m_vertices.size()
  //    << " vertices" << m_indices.size() << " indices";
  m_vertex.allocate(
      m_vertices.data(),
      static_cast<int>(sizeof(BillboardVertex) * m_vertices.size()));
  m_index.allocate(m_indices.data(),
                   static_cast<int>(sizeof(GLuint) * m_indices.size()));
}

void BillboardRenderer::draw() {
  // TODO sort in z-order (i.e. depth) for transparency
  // glDepthFunc(GL_ALWAYS);
  for (size_t i = 0; i < m_vertices.size() / 4; i++) {
    m_textures[m_labels[i]]->bind();
    glDrawElements(DrawType, 6, IndexType, (GLvoid *)(6 * i * sizeof(GLuint)));
    m_textures[m_labels[i]]->release();
  }
  // glDepthFunc(GL_LESS);
}

BillboardRenderer::~BillboardRenderer() {}
