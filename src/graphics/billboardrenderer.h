#pragma once

#include "billboardvertex.h"
#include "renderer.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <vector>

using std::vector;

class BillboardRenderer : public IndexedRenderer {
public:
  static constexpr size_t MaxVertices = 65536 / sizeof(BillboardVertex);

  BillboardRenderer();
  BillboardRenderer(const vector<BillboardVertex> &vertices,
                    const QString &label = "Label");
  void addVertices(const vector<BillboardVertex> &vertices,
                   const QString &label = "Label");
  void addVertices(const vector<BillboardVertex> &vertices,
                   const QString &label, QOpenGLTexture *texture);

  inline bool hasTextureForText(const QString &text) const {
    return m_textures.contains(text);
  }
  virtual ~BillboardRenderer();

  inline size_t size() const { return m_vertices.size() / 4; }

  virtual void beginUpdates();
  virtual void endUpdates();
  virtual void draw();
  void clear();

private:
  void updateBuffers();
  QOpenGLBuffer m_vertex;
  vector<BillboardVertex> m_vertices;
  vector<GLuint> m_indices;
  QMap<QString, QOpenGLTexture *> m_textures;
  std::vector<QImage> m_images;
  std::vector<QString> m_labels;

protected:
  bool m_impostor = true;
};
