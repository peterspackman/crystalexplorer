#pragma once
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QRandomGenerator>
#include <QString>
#include <QVector3D>
#include <vector>

using std::vector;

template <typename VertexType> class GroupIndex {
public:
  /* Not sure if the indices array code is correct
   * but it should look something like this */
  GroupIndex(const size_t first, const size_t last, vector<VertexType> &data)
      : m_data(data), m_firstVertex(first), m_lastVertex(last) {}

  auto begin() const { return m_data.begin() + m_firstVertex; }
  auto end() const { return m_data.begin() + m_lastVertex; }
  auto firstVertex() const { return m_firstVertex; }
  auto lastVertex() const { return m_lastVertex; }

private:
  vector<VertexType> &m_data;
  size_t m_firstVertex = 0;
  size_t m_lastVertex = 0;
};

/*
 * Abstract renderer base class
 */
class Renderer {
public:
  static const GLenum IndexType = GL_UNSIGNED_INT;
  static const GLenum DrawType = GL_TRIANGLES;
  enum class ShadingMode { PBR, Flat };
  virtual ~Renderer() {}
  virtual void draw() {
    if (m_numberOfIndices == 0)
      return;
    // bind the object, draw, then release
    glDrawElements(DrawType, m_numberOfIndices, IndexType, 0);
  }

  virtual void bind() {
    m_program->bind();
    m_object.bind();
  }

  virtual void release() {
    m_object.release();
    m_program->release();
  }
  void setShadingMode(ShadingMode mode) {
    auto result = m_shader_programs.find(mode);
    if (result != m_shader_programs.end()) {
      m_program = result.value();
      m_shading_mode = mode;
    }
  }
  virtual void beginUpdates() { m_updatesDisabled = true; }
  virtual void endUpdates() { m_updatesDisabled = false; }
  virtual void clear() {
    // do nothing in base case
  }

  inline QOpenGLShaderProgram *program() const { return m_program; }
  inline bool isImpostor() const { return m_impostor; }
  inline const QVector3D center() const { return m_center; }
  inline const QString &id() const { return m_id; }

  static const QString generateId() {
    const QString possibleCharacters("abcdefghijklmnopqrstuvwxyz0123456789");
    const int randomStringLength = 6;
    QString randomString;
    for (int i = 0; i < randomStringLength; ++i) {
      int index =
          QRandomGenerator::global()->generate() % possibleCharacters.length();
      QChar nextChar = possibleCharacters.at(index);
      randomString.append(nextChar);
    }
    return randomString;
  }

protected:
  QOpenGLVertexArrayObject m_object;
  QOpenGLShaderProgram *m_program = nullptr;
  QMap<ShadingMode, QOpenGLShaderProgram *> m_shader_programs;
  QString m_id = "!id not set!";
  QVector3D m_center = QVector3D(0, 0, 0);
  GLsizei m_numberOfIndices = 0;
  bool m_impostor = false;
  bool m_updatesDisabled{false};
  ShadingMode m_shading_mode{ShadingMode::PBR};
};

/*
 * Renderer with an index buffer base class
 */
class IndexedRenderer : public Renderer {
public:
  IndexedRenderer() : m_index(QOpenGLBuffer::IndexBuffer) {}

  virtual void bind() {
    m_program->bind();
    m_object.bind();
    m_index.bind();
  }

  virtual void release() {
    m_index.release();
    m_object.release();
    m_program->release();
  }

protected:
  QOpenGLBuffer m_index;
};

Q_DECLARE_METATYPE(Renderer *);
