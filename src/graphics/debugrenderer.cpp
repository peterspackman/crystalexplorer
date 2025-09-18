#include "debugrenderer.h"
#include "ellipsoidrenderer.h"
#include "graphics.h"
#include "rendereruniforms.h"
#include <QtOpenGL>

DebugRenderer::DebugRenderer()
    : m_lineRenderer(new LineRenderer()),
      m_pointRenderer(new EllipsoidRenderer()), m_visible(false),
      m_needsUpdate(false) {

  // Add permanent grid in XY plane at Z=0 from -10 to 10
  QColor gridColor = Qt::darkGray;
  float gridThickness = 0.3f;

  // Grid lines parallel to X axis (horizontal)
  for (int y = -10; y <= 10; y += 2) {
    m_permanentLines.push_back(
        {QVector3D(-10, y, 0), QVector3D(10, y, 0), gridColor, gridThickness});
  }

  // Grid lines parallel to Y axis (vertical)
  for (int x = -10; x <= 10; x += 2) {
    m_permanentLines.push_back(
        {QVector3D(x, -10, 0), QVector3D(x, 10, 0), gridColor, gridThickness});
  }

  // Add origin axes for reference
  m_permanentLines.push_back(
      {QVector3D(-10, 0, 0), QVector3D(10, 0, 0), Qt::red, 0.5f}); // X axis
  m_permanentLines.push_back(
      {QVector3D(0, -10, 0), QVector3D(0, 10, 0), Qt::green, 0.5f}); // Y axis
  m_permanentLines.push_back(
      {QVector3D(0, 0, -5), QVector3D(0, 0, 5), Qt::blue, 0.5f}); // Z axis
  m_needsUpdate = true; // Force update for permanent elements
}

DebugRenderer::~DebugRenderer() {
  delete m_lineRenderer;
  delete m_pointRenderer;
}

void DebugRenderer::addLine(const QVector3D &start, const QVector3D &end,
                            const QColor &color, float thickness) {
  m_lines.push_back({start, end, color, thickness});
  m_needsUpdate = true;
}

void DebugRenderer::addPoint(const QVector3D &position, const QColor &color,
                             float size) {
  m_points.push_back({position, color, size});
  m_needsUpdate = true;
}

void DebugRenderer::addRay(const QVector3D &origin, const QVector3D &direction,
                           float length, const QColor &color) {
  QVector3D end = origin + direction * length;
  addLine(origin, end, color);
}

void DebugRenderer::addLines(const std::vector<QVector3D> &points,
                             const QColor &color, float thickness) {
  for (size_t i = 0; i < points.size() - 1; ++i) {
    addLine(points[i], points[i + 1], color, thickness);
  }
}

void DebugRenderer::addWireframeSphere(const QVector3D &center, float radius,
                                       const QColor &color, int segments) {
  float angleStep = 2.0f * M_PI / segments;

  // Draw three circles for X, Y, Z axes
  std::vector<QVector3D> circleXY, circleXZ, circleYZ;

  for (int i = 0; i <= segments; ++i) {
    float angle = i * angleStep;
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);

    // XY circle (around Z axis)
    circleXY.push_back(center + QVector3D(radius * cosA, radius * sinA, 0));
    // XZ circle (around Y axis)
    circleXZ.push_back(center + QVector3D(radius * cosA, 0, radius * sinA));
    // YZ circle (around X axis)
    circleYZ.push_back(center + QVector3D(0, radius * cosA, radius * sinA));
  }

  addLines(circleXY, color);
  addLines(circleXZ, color);
  addLines(circleYZ, color);
}

void DebugRenderer::clear() {
  m_lines.clear();
  m_points.clear();
  m_needsUpdate = true;
}

void DebugRenderer::setVisible(bool visible) { m_visible = visible; }

bool DebugRenderer::isVisible() const { return m_visible; }

void DebugRenderer::updateRenderers() {
  if (!m_needsUpdate)
    return;

  // Update line renderer
  m_lineRenderer->clear();
  m_lineRenderer->beginUpdates();

  // Add permanent lines first
  for (const auto &line : m_permanentLines) {
    cx::graphics::addLineToLineRenderer(*m_lineRenderer, line.start, line.end,
                                        line.thickness, line.color);
  }

  // Add regular lines
  for (const auto &line : m_lines) {
    cx::graphics::addLineToLineRenderer(*m_lineRenderer, line.start, line.end,
                                        line.thickness, line.color);
  }

  m_lineRenderer->endUpdates();

  // Update point renderer
  m_pointRenderer->clear();
  m_pointRenderer->beginUpdates();

  // Add permanent points first
  for (const auto &point : m_permanentPoints) {
    cx::graphics::addSphereToEllipsoidRenderer(m_pointRenderer, point.position,
                                               point.color, point.size);
  }

  // Add regular points
  for (const auto &point : m_points) {
    cx::graphics::addSphereToEllipsoidRenderer(m_pointRenderer, point.position,
                                               point.color, point.size);
  }

  m_pointRenderer->endUpdates();

  m_needsUpdate = false;
}

void DebugRenderer::draw() {
  if (!m_visible)
    return;

  updateRenderers();

  // Draw lines
  m_lineRenderer->bind();
  m_uniforms.apply(m_lineRenderer);
  m_lineRenderer->draw();
  m_lineRenderer->release();

  // Draw points
  m_pointRenderer->bind();
  m_uniforms.apply(m_pointRenderer);
  m_pointRenderer->draw();
  m_pointRenderer->release();
}

void DebugRenderer::updateRendererUniforms(
    const cx::graphics::RendererUniforms &uniforms) {
  m_uniforms = uniforms;
}
