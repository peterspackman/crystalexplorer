#pragma once

#include "linerenderer.h"
#include "rendereruniforms.h"
#include <QColor>
#include <vector>

class DebugRenderer {
public:
  DebugRenderer();
  ~DebugRenderer();

  // Line and point management
  void addLine(const QVector3D &start, const QVector3D &end,
               const QColor &color = Qt::red, float thickness = 1.0f);
  void addPoint(const QVector3D &position, const QColor &color = Qt::magenta,
                float size = 5.0f);
  void addRay(const QVector3D &origin, const QVector3D &direction, float length,
              const QColor &color = Qt::yellow);

  // Batch operations for performance
  void addLines(const std::vector<QVector3D> &points,
                const QColor &color = Qt::red, float thickness = 1.0f);
  void addWireframeSphere(const QVector3D &center, float radius,
                          const QColor &color = Qt::cyan, int segments = 16);

  // Control
  void clear();
  void setVisible(bool visible);
  bool isVisible() const;

  // Rendering
  void draw();
  void updateRendererUniforms(const cx::graphics::RendererUniforms &uniforms);

private:
  struct DebugLine {
    QVector3D start;
    QVector3D end;
    QColor color;
    float thickness;
  };

  struct DebugPoint {
    QVector3D position;
    QColor color;
    float size;
  };

  std::vector<DebugLine> m_lines;
  std::vector<DebugPoint> m_points;
  std::vector<DebugLine> m_permanentLines;   // Always visible test lines
  std::vector<DebugPoint> m_permanentPoints; // Always visible test points
  LineRenderer *m_lineRenderer;
  class EllipsoidRenderer *m_pointRenderer;
  bool m_visible{false};
  bool m_needsUpdate{true};
  cx::graphics::RendererUniforms m_uniforms;

  void updateRenderers();
};
