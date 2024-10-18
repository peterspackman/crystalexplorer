#include "measurementrenderer.h"
#include "colormap.h"
#include "graphics.h"

namespace cx::graphics {

MeasurementRenderer::MeasurementRenderer(QObject *parent) : QObject(parent) {
  m_lines = new LineRenderer();
  m_circles = new CircleRenderer();
  m_labels = new BillboardRenderer();
}

void MeasurementRenderer::add(const Measurement &m) {
  auto idx = m_measurementList.size();
  m_measurementList.append(m);
  auto func = ColorMapFunc(ColorMapName::Turbo);
  func.lower = 0.0;
  func.upper = qMax(m_measurementList.size(), 10);
  m_measurementList.back().setColor(func(idx));
  qDebug() << "Add measurement";
  m_needsUpdate = true;
}

void MeasurementRenderer::removeLastMeasurement() {
  if (m_measurementList.size() > 0) {
    m_measurementList.removeLast();
    m_needsUpdate = true;
  }
}

void MeasurementRenderer::clear() {
  m_measurementList.clear();
  m_needsUpdate = true;
}

bool MeasurementRenderer::hasMeasurements() const {
  return !m_measurementList.isEmpty();
}

void MeasurementRenderer::handleUpdate() {
  if (!m_needsUpdate)
    return;
  m_lines->clear();
  m_circles->clear();
  m_labels->clear();

  m_lines->beginUpdates();
  m_circles->beginUpdates();
  m_labels->beginUpdates();
  for (const auto &measurement : std::as_const(m_measurementList)) {
    cx::graphics::addTextToBillboardRenderer(
        *m_labels, measurement.labelPosition(), measurement.label());
    measurement.draw(m_lines, m_circles);
  }
  m_circles->endUpdates();
  m_lines->endUpdates();
  m_labels->endUpdates();
  m_needsUpdate = false;
}

void MeasurementRenderer::updateRendererUniforms(
    const RendererUniforms &uniforms) {
  m_uniforms = uniforms;
}

void MeasurementRenderer::draw() {
  handleUpdate();
  m_lines->bind();
  m_uniforms.apply(m_lines);
  m_lines->draw();
  m_lines->release();

  m_circles->bind();
  m_uniforms.apply(m_circles);
  m_circles->draw();
  m_circles->release();

  m_labels->bind();
  m_uniforms.apply(m_labels);
  m_labels->draw();
  m_labels->release();
}

} // namespace cx::graphics
