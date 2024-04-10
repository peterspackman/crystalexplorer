#include "meshinstancerenderer.h"
#include "shaderloader.h"

#include <QOpenGLShaderProgram>

MeshInstanceRenderer::MeshInstanceRenderer(Mesh * mesh) : QOpenGLExtraFunctions(QOpenGLContext::currentContext()) {

  m_propertyColorMaps = {
    {"None", ColorMapName::CE_None},
    {"dnorm", ColorMapName::CE_bwr},
    {"di", ColorMapName::CE_rgb},
    {"de", ColorMapName::CE_rgb},
    {"di_norm", ColorMapName::CE_bwr},
    {"de_norm", ColorMapName::CE_bwr},
  };

  m_program = new QOpenGLShaderProgram();
  m_program->addCacheableShaderFromSourceCode(
	  QOpenGLShader::Vertex,
          cx::shader::loadShaderFile(":/shaders/meshinstance.vert")
  );

  m_program->addCacheableShaderFromSourceCode(
      QOpenGLShader::Fragment,
      cx::shader::loadShaderFile(":/shaders/meshinstance.frag"));
  if (!m_program->link()) {
    qDebug() << "Shader link error:" << m_program->log();
  }
  m_program->bind();

  // Create MeshInstanceVertex Buffer (Do not release until VAO is created)
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::StaticDraw);

  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::StaticDraw);

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
  m_program->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));
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

  m_program->setAttributeBuffer(
      2, GL_FLOAT, MeshInstanceVertex::translationOffset(),
      MeshInstanceVertex::TranslationTupleSize, MeshInstanceVertex::stride());
  this->glVertexAttribDivisor(2, 1);
  m_program->setAttributeBuffer(3, GL_FLOAT, MeshInstanceVertex::rotation1Offset(),
                                MeshInstanceVertex::RotationTupleSize,
                                MeshInstanceVertex::stride());
  this->glVertexAttribDivisor(3, 1);
  m_program->setAttributeBuffer(4, GL_FLOAT, MeshInstanceVertex::rotation2Offset(),
                                MeshInstanceVertex::RotationTupleSize,
                                MeshInstanceVertex::stride());
  this->glVertexAttribDivisor(4, 1);
  m_program->setAttributeBuffer(5, GL_FLOAT, MeshInstanceVertex::rotation3Offset(),
                                MeshInstanceVertex::RotationTupleSize,
                                MeshInstanceVertex::stride());
  this->glVertexAttribDivisor(5, 1);

  // matrix takes up 3 attribute locations
  m_program->setAttributeBuffer(6, GL_FLOAT, MeshInstanceVertex::selectionIdOffset(),
                                MeshInstanceVertex::SelectionIdSize,
                                MeshInstanceVertex::stride());
  this->glVertexAttribDivisor(6, 1);
  m_program->setAttributeBuffer(
      7, GL_FLOAT, MeshInstanceVertex::alphaOffset(),
      MeshInstanceVertex::AlphaSize, MeshInstanceVertex::stride());
  this->glVertexAttribDivisor(7, 1);
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
  if(!mesh) return;

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

  std::vector<GLuint> temp_faces;
  temp_faces.reserve(faces.size());
  for(int i = 0; i < faces.cols(); i++) {
      temp_faces.push_back(static_cast<GLuint>(faces(0, i)));
      temp_faces.push_back(static_cast<GLuint>(faces(1, i)));
      temp_faces.push_back(static_cast<GLuint>(faces(2, i)));
  }

  m_vertex.allocate(temp.data(),
                    static_cast<int>(sizeof(float) * temp.size()));
  m_numIndices = temp_faces.size();
  m_index.allocate(temp_faces.data(), static_cast<int>(sizeof(GLuint) * m_numIndices));

  {
      m_vertexPropertyBuffer.bind();
      std::vector<float> propertyData;
      auto availableProperties = mesh->availableVertexProperties();
      qDebug() << "Available properties: " << availableProperties;
      for(const auto &prop: availableProperties) {
	  if(prop == "None") continue;

	  const auto &vals = mesh->vertexProperty(prop);
          auto range = mesh->vertexPropertyRange(prop);

	  ColorMapFunc cmap(m_propertyColorMaps.value(prop, ColorMapName::Viridis), range.lower, range.upper);

	  for(size_t i = 0; i < vals.rows(); i++) {
	      QColor color = cmap(vals(i));
	      propertyData.push_back(color.redF());
	      propertyData.push_back(color.greenF());
	      propertyData.push_back(color.blueF());
	      propertyData.push_back(color.alphaF());
	  }
      }
      m_vertexPropertyBuffer.allocate(propertyData.data(), static_cast<int>(sizeof(float) * propertyData.size()));
      m_vertexPropertyBuffer.release();

      m_vertexPropertyTexture->bind();
      glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_vertexPropertyBuffer.bufferId());
      m_vertexPropertyTexture->release();

  }

  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    // Log or handle the error
    qDebug() << "OpenGL error (setMesh):" << err;
 }
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
  // After linking the program and before rendering
  m_program->setUniformValue("u_propertyBuffer", 0);
  m_vertexPropertyTexture->bind();
 
  this->glDrawElementsInstanced(DrawType, m_numIndices, IndexType, 0, static_cast<int>(m_instances.size()));

  m_vertexPropertyTexture->release();
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
