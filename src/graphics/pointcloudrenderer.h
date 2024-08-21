#pragma once
#include "renderer.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <vector>

class PointCloudVertex {
public:
  // Constructors
  constexpr explicit PointCloudVertex() : m_position(), m_color() {}

  constexpr explicit PointCloudVertex(const QVector3D &position,
                                      const QVector3D &color)
      : m_position(position), m_color(color) {}

  constexpr inline const auto &position() const { return m_position; }
  constexpr inline const auto &color() const { return m_color; }

  constexpr inline void setPosition(const QVector3D &position) {
    m_position = position;
  }
  constexpr inline void setColor(const QVector3D &color) { m_color = color; }

  // OpenGL Helpers
  static constexpr int PositionSize = 3;
  static constexpr int ColorSize = 3;

  static constexpr int positionOffset() {
    return offsetof(PointCloudVertex, m_position);
  }
  static constexpr int colorOffset() {
    return offsetof(PointCloudVertex, m_color);
  }

  static constexpr int stride() { return sizeof(PointCloudVertex); }

private:
  QVector3D m_position;
  QVector3D m_color;
};

Q_DECLARE_TYPEINFO(PointCloudVertex, Q_MOVABLE_TYPE);

class PointCloudRenderer : public Renderer {
public:
  PointCloudRenderer();
  PointCloudRenderer(const vector<PointCloudVertex> &);
  void addPoint(const PointCloudVertex &);
  void addPoints(const vector<PointCloudVertex> &);

  virtual void beginUpdates() override;
  virtual void endUpdates() override;
  virtual void draw() override;
  virtual void clear() override;

private:
  float m_alpha{1.0};
  void updateBuffers();
  QOpenGLBuffer m_vertex;
  vector<PointCloudVertex> m_points;
};

