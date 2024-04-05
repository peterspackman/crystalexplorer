#include "meshinstancerenderer.h"
#include "shaderloader.h"

#include <QOpenGLShaderProgram>
#include <QUuid>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>

MeshInstanceRenderer::MeshInstanceRenderer(Mesh * mesh) {

  QOpenGLExtraFunctions f(QOpenGLContext::currentContext());
  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceCode(
	  QOpenGLShader::Vertex,
          cx::shader::loadShaderFile(":/shaders/meshinstance.vert")
  );

  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Fragment,
      cx::shader::loadShaderFile(":/shaders/meshinstance.frag"));
  m_program->link();
  m_program->bind();

  // Create MeshInstanceVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::StaticDraw);

  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::StaticDraw);
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
  m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 2 * sizeof(QVector3D));
  m_program->setAttributeBuffer(1, GL_FLOAT, 3, 3, 2 * sizeof(QVector3D));
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
      1, GL_FLOAT, MeshInstanceVertex::translationOffset(),
      MeshInstanceVertex::TranslationTupleSize, MeshInstanceVertex::stride());
  f.glVertexAttribDivisor(1, 1);
  m_program->setAttributeBuffer(2, GL_FLOAT, MeshInstanceVertex::rotationOffset(),
                                MeshInstanceVertex::RotationTupleSize,
                                MeshInstanceVertex::stride());
  f.glVertexAttribDivisor(2, 1);
  m_program->setAttributeBuffer(3, GL_FLOAT, MeshInstanceVertex::selectionIdOffset(),
                                MeshInstanceVertex::SelectionIdSize,
                                MeshInstanceVertex::stride());
  f.glVertexAttribDivisor(3, 1);
  m_program->setAttributeBuffer(
      4, GL_FLOAT, MeshInstanceVertex::alphaOffset(),
      MeshInstanceVertex::AlphaSize, MeshInstanceVertex::stride());
  f.glVertexAttribDivisor(4, 1);
  // Release (unbind) all
  m_instance.release();
  m_index.release();
  m_object.release();
  m_vertex.release();
  m_program->release();
}

void MeshInstanceRenderer::setMesh(Mesh * mesh) {
  m_vertex.bind();
  m_index.bind();

  const auto &vertices = mesh->vertices();
  const auto &normals = mesh->vertexNormals();
  const auto &faces = mesh->faces();

  std::vector<float> temp;
  temp.reserve(6 * vertices.cols());
  for(int i = 0; i < vertices.cols(); i++) {
      temp.push_back(vertices(0, i));
      temp.push_back(vertices(1, i));
      temp.push_back(vertices(2, i));
      temp.push_back(normals(0, i));
      temp.push_back(normals(1, i));
      temp.push_back(normals(2, i));
  }

  m_vertex.allocate(temp.data(),
                    static_cast<int>(temp.size()));
  m_numFaces = faces.size();
  m_index.allocate(faces.data(), m_numFaces);
}

void MeshInstanceRenderer::addInstances(
    const std::vector<MeshInstanceVertex> &instances) {
  m_instances.insert(m_instances.end(), instances.begin(), instances.end());
  if (!m_updatesDisabled)
    updateBuffers();
}

void MeshInstanceRenderer::addInstance(const MeshInstanceVertex &instance) {
  m_instances.push_back(instance);
  if (!m_updatesDisabled)
    updateBuffers();
}

void MeshInstanceRenderer::clear() {
  if (m_instances.size() > 1) {
    m_instances.clear();
    updateBuffers();
  }
}

void MeshInstanceRenderer::draw() {
  QOpenGLExtraFunctions f(QOpenGLContext::currentContext());
  f.glDrawElementsInstanced(DrawType, m_numFaces, GL_INT, 0,
                            m_instances.size());
}

void MeshInstanceRenderer::beginUpdates() { Renderer::beginUpdates(); }

void MeshInstanceRenderer::endUpdates() {
  Renderer::endUpdates();
  updateBuffers();
}

void MeshInstanceRenderer::updateBuffers() {
  if (m_updatesDisabled)
    return;
  m_instance.bind();
  m_instance.allocate(
      m_instances.data(),
      static_cast<int>(sizeof(MeshInstanceVertex) * m_instances.size()));
}
