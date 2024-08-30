#include "volumerenderer.h"
#include "shaderloader.h"
#include <QDebug>

VolumeRenderer::VolumeRenderer()
    :
      m_volumeTexture(nullptr), m_transferFunctionTexture(nullptr),
      m_gridVec1(1, 0, 0), m_gridVec2(0, 1, 0), m_gridVec3(0, 0, 1),
      m_updatesDisabled(false), m_initialized(false) {

  initializeOpenGLFunctions();
  initializeGL();
}

VolumeRenderer::~VolumeRenderer() {
  if (m_program) {
    delete m_program;
  }
  if (m_volumeTexture) {
    delete m_volumeTexture;
  }
  if (m_transferFunctionTexture) {
    delete m_transferFunctionTexture;
  }
}

void VolumeRenderer::initializeGL() {
  if (m_initialized)
    return;

  qDebug() << "Initializing volume renderer GL";
  m_program = new QOpenGLShaderProgram();

  if (!m_program->addCacheableShaderFromSourceCode(
          QOpenGLShader::Vertex,
          cx::shader::loadShaderFile(":/shaders/volume.vert"))) {
    qDebug() << "Failed to compile vertex shader:" << m_program->log();
    return;
  }
  if (!m_program->addCacheableShaderFromSourceCode(
          QOpenGLShader::Fragment,
          cx::shader::loadShaderFile(":/shaders/volume.frag"))) {
    qDebug() << "Failed to compile fragment shader:" << m_program->log();
    return;
  }
  if (!m_program->link()) {
    qDebug() << "Failed to link shader program:" << m_program->log();
    return;
  }

  m_program->bind();

  // Cache uniform locations
  m_volumeTextureLoc = m_program->uniformLocation("u_volumeTexture");
  m_transferFunctionLoc = m_program->uniformLocation("u_transferFunction");
  m_volumeDimensionsLoc = m_program->uniformLocation("u_volumeDimensions");
  m_gridVec1Loc = m_program->uniformLocation("u_gridVec1");
  m_gridVec2Loc = m_program->uniformLocation("u_gridVec2");
  m_gridVec3Loc = m_program->uniformLocation("u_gridVec3");

  m_program->release();

  // Create buffers
  m_vertex.create();
  m_vertex.bind();
  m_vertex.setUsagePattern(QOpenGLBuffer::StaticDraw);

  m_index.create();
  m_index.bind();
  m_index.setUsagePattern(QOpenGLBuffer::StaticDraw);


  // Create VAO
  m_object.create();
  m_object.bind();
  m_vertex.bind();
  m_index.bind();
  m_program->enableAttributeArray(0);
  m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(QVector3D));
  m_object.release();

  createGeometry();
  updateBuffers();

  qDebug() << "Finished initilizing volume renderer GL";
  m_initialized = true;
}

void VolumeRenderer::setVolumeData(const std::vector<float> &data, int width,
                                   int height, int depth) {
  m_volumeWidth = width;
  m_volumeHeight = height;
  m_volumeDepth = depth;

  if (m_volumeTexture) {
    delete m_volumeTexture;
  }

  m_volumeTexture = new QOpenGLTexture(QOpenGLTexture::Target3D);
  m_volumeTexture->setSize(width, height, depth);
  m_volumeTexture->setFormat(QOpenGLTexture::R32F);
  m_volumeTexture->setMinificationFilter(QOpenGLTexture::Linear);
  m_volumeTexture->setMagnificationFilter(QOpenGLTexture::Linear);
  m_volumeTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
  m_volumeTexture->allocateStorage();
  m_volumeTexture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32,
                           data.data());
}

void VolumeRenderer::setGridVectors(const QVector3D &vec1,
                                    const QVector3D &vec2,
                                    const QVector3D &vec3) {
  m_gridVec1 = vec1;
  m_gridVec2 = vec2;
  m_gridVec3 = vec3;
}

void VolumeRenderer::setTransferFunction(
    const std::vector<QVector4D> &transferFunction) {
  if (m_transferFunctionTexture) {
    delete m_transferFunctionTexture;
  }

  m_transferFunctionTexture = new QOpenGLTexture(QOpenGLTexture::Target1D);
  m_transferFunctionTexture->setSize(transferFunction.size());
  m_transferFunctionTexture->setFormat(QOpenGLTexture::RGBA32F);
  m_transferFunctionTexture->setMinificationFilter(QOpenGLTexture::Linear);
  m_transferFunctionTexture->setMagnificationFilter(QOpenGLTexture::Linear);
  m_transferFunctionTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
  m_transferFunctionTexture->allocateStorage();
  m_transferFunctionTexture->setData(
      QOpenGLTexture::RGBA, QOpenGLTexture::Float32, transferFunction.data());
}

void VolumeRenderer::beginUpdates() { 
  Renderer::beginUpdates();
}

void VolumeRenderer::endUpdates() {
  Renderer::endUpdates();
  updateBuffers();
}

void VolumeRenderer::draw() {
  if (!m_initialized) {
    initializeGL();
  }

  if (!m_volumeTexture || !m_transferFunctionTexture) {
    qDebug() << "Textures not initialized!";
    return;
  }

  m_volumeTexture->bind(0);
  m_transferFunctionTexture->bind(1);

  m_program->setUniformValue(m_volumeTextureLoc, 0);
  m_program->setUniformValue(m_transferFunctionLoc, 1);
  m_program->setUniformValue(
      m_volumeDimensionsLoc,
      QVector3D(m_volumeWidth, m_volumeHeight, m_volumeDepth));
  m_program->setUniformValue(m_gridVec1Loc, m_gridVec1);
  m_program->setUniformValue(m_gridVec2Loc, m_gridVec2);
  m_program->setUniformValue(m_gridVec3Loc, m_gridVec3);

  glFrontFace(
      GL_CW); // Assuming counter-clockwise winding order for front faces
  glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);

  glFrontFace(
      GL_CCW); // Assuming counter-clockwise winding order for front faces
  glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);

  m_transferFunctionTexture->release();
  m_volumeTexture->release();

  // Error checking
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    qDebug() << "OpenGL error (draw):" << err;
  }
}

void VolumeRenderer::clear() {
  m_vertices.clear();
  m_indices.clear();
  updateBuffers();
}

void VolumeRenderer::updateBuffers() {
  if (m_updatesDisabled) {
    return;
  }

  m_vertex.bind();
  m_vertex.allocate(
      m_vertices.data(),
      static_cast<int>(sizeof(QVector3D) * m_vertices.size()));

  m_index.bind();
  m_index.allocate(m_indices.data(),
                         static_cast<int>(sizeof(GLuint) * m_indices.size()));
}

void VolumeRenderer::createGeometry() {
  // Create a cube for volume rendering
  m_vertices = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};

  m_indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4, 0, 4, 7, 7, 3, 0,
               1, 5, 6, 6, 2, 1, 0, 1, 5, 5, 4, 0, 3, 2, 6, 6, 7, 3};
}
