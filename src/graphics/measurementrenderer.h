#pragma once
#include "billboardrenderer.h"
#include "circlerenderer.h"
#include "linerenderer.h"
#include "measurement.h"
#include "rendereruniforms.h"

namespace cx::graphics {
class MeasurementRenderer : public QObject {
  Q_OBJECT
public:
  explicit MeasurementRenderer(QObject *parent = nullptr);

  void add(const Measurement &);
  void removeLastMeasurement();
  void clear();

  bool hasMeasurements() const;
  inline const auto &measurements() const { return m_measurementList; }

  void updateRendererUniforms(const RendererUniforms &);
  void draw();

private:
  void handleUpdate();

  bool m_needsUpdate{true};

  RendererUniforms m_uniforms;
  QList<Measurement> m_measurementList;
  LineRenderer *m_lines{nullptr};
  CircleRenderer *m_circles{nullptr};
  BillboardRenderer *m_labels{nullptr};
};

} // namespace cx::graphics
