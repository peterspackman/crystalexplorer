#include "meshrenderer.h"
#include "shaderloader.h"

#include <QOpenGLShaderProgram>
#include <QUuid>

QOpenGLShaderProgram *MeshRenderer::g_program = nullptr;

QOpenGLShaderProgram *MeshRenderer::getShaderProgram() {
  if (g_program == nullptr) {
    g_program = new QOpenGLShaderProgram();
    g_program->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex,
                                                ":/shaders/mesh.vert");
    g_program->addCacheableShaderFromSourceCode(
        QOpenGLShader::Fragment,
        cx::shader::loadShaderFile(":/shaders/mesh.frag"));
    g_program->link();
  }
  return g_program;
}

MeshRenderer::MeshRenderer() : m_vertex(QOpenGLBuffer::VertexBuffer) {
  m_program = getShaderProgram();
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
  m_program->setAttributeBuffer(0, GL_FLOAT, MeshVertex::positionOffset(),
                                MeshVertex::PositionTupleSize,
                                MeshVertex::stride());
  m_program->setAttributeBuffer(1, GL_FLOAT, MeshVertex::normalOffset(),
                                MeshVertex::NormalTupleSize,
                                MeshVertex::stride());
  m_program->setAttributeBuffer(2, GL_FLOAT, MeshVertex::colorOffset(),
                                MeshVertex::ColorTupleSize,
                                MeshVertex::stride());
  m_program->setAttributeBuffer(3, GL_FLOAT, MeshVertex::selectionIdOffset(),
                                MeshVertex::SelectionIdTupleSize,
                                MeshVertex::stride());
  // Release (unbind) all
  m_object.release();
  m_vertex.release();
  m_program->release();
}

MeshRenderer::MeshRenderer(const vector<MeshVertex> &vertices,
                           const vector<IndexTuple> &indices)
    : m_vertex(QOpenGLBuffer::VertexBuffer) {
  // Create Shader (Do not release until VAO is created)
  m_program = getShaderProgram();
  m_program->bind();

  // Create SphereImpostorVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  // Create Index Buffer (Do not release until VAO is created)
  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  addMesh(vertices, indices);

  // Create SphereImpostorVertex Array Object
  m_object.create();
  m_object.bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  m_program->enableAttributeArray(3);
  m_program->setAttributeBuffer(0, GL_FLOAT, MeshVertex::positionOffset(),
                                MeshVertex::PositionTupleSize,
                                MeshVertex::stride());
  m_program->setAttributeBuffer(1, GL_FLOAT, MeshVertex::normalOffset(),
                                MeshVertex::NormalTupleSize,
                                MeshVertex::stride());
  m_program->setAttributeBuffer(2, GL_FLOAT, MeshVertex::colorOffset(),
                                MeshVertex::ColorTupleSize,
                                MeshVertex::stride());
  m_program->setAttributeBuffer(3, GL_FLOAT, MeshVertex::selectionIdOffset(),
                                MeshVertex::SelectionIdTupleSize,
                                MeshVertex::stride());
  // Release (unbind) all
  m_object.release();
  m_vertex.release();
  m_program->release();
}

void MeshRenderer::addMesh(const vector<MeshVertex> &vertices,
                           const vector<IndexTuple> &indices) {
  if (vertices.size() > 0) {
    GLuint num_vertices_prev = m_vertices.size();
    m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
    if (num_vertices_prev > 0) {
      for (const auto &idx : indices) {
        m_indices.push_back({idx.i + num_vertices_prev,
                             idx.j + num_vertices_prev,
                             idx.k + num_vertices_prev});
      }
    } else {
      m_indices.insert(m_indices.end(), indices.begin(), indices.end());
    }
    m_numberOfIndices = static_cast<GLsizei>(m_indices.size() * 3);
  }
  updateBuffers();
}

void MeshRenderer::clear() {
  if (m_vertices.size() > 1) {
    m_indices.clear();
    m_vertices.clear();
    m_numberOfIndices = 0;
    updateBuffers();
  }
}

void MeshRenderer::updateBuffers() {
  if (!m_vertex.bind())
    qDebug() << "Failed to bind vertex buffer";
  if (!m_index.bind())
    qDebug() << "Failed to bind index buffer";
  //    qDebug() << "Updating buffers for " << m_id << ": " << m_vertices.size()
  //    << " vertices" << m_indices.size() << " indices";
  m_vertex.allocate(m_vertices.data(),
                    static_cast<int>(sizeof(MeshVertex) * m_vertices.size()));
  m_index.allocate(m_indices.data(),
                   static_cast<int>(sizeof(IndexTuple) * m_indices.size()));
}

MeshRenderer::~MeshRenderer() {}
