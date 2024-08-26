#pragma once
#include "chemicalstructure.h"
#include "colormap.h"
#include "cylinderrenderer.h"
#include "ellipsoidrenderer.h"
#include "frameworkoptions.h"
#include "linerenderer.h"
#include "rendereruniforms.h"

namespace cx::graphics {

class FrameworkRenderer : public QObject {
  Q_OBJECT
public:
  FrameworkRenderer(ChemicalStructure *, QObject *parent = nullptr);

  void update(ChemicalStructure *);

  [[nodiscard]] float thickness() const;
  void setThickness(float);

  [[nodiscard]] inline const auto &options() const { return m_options; }
  void setOptions(const FrameworkOptions &);

  void updateRendererUniforms(const RendererUniforms &);

  void updateInteractions();

  void beginUpdates();
  void endUpdates();

  void setModel(const QString &);
  void setComponent(const QString &);

  void draw(bool forPicking = false);

  [[nodiscard]] inline CylinderRenderer *cylinderRenderer() {
    return m_cylinderRenderer;
  }
  [[nodiscard]] inline EllipsoidRenderer *ellipsoidRenderer() {
    return m_ellipsoidRenderer;
  }
  [[nodiscard]] inline LineRenderer *lineRenderer() { return m_lineRenderer; }

signals:
  void frameworkChanged();

private:
  void handleInteractionsUpdate();

  [[nodiscard]] bool shouldSkipAtom(int idx) const;

  bool m_needsUpdate{true};
  float m_thicknessScale{1.0};

  LineRenderer *m_lineRenderer{nullptr};
  EllipsoidRenderer *m_ellipsoidRenderer{nullptr};
  CylinderRenderer *m_cylinderRenderer{nullptr};

  ChemicalStructure *m_structure{nullptr};

  PairInteractions *m_interactions{nullptr};
  RendererUniforms m_uniforms;

  FrameworkOptions m_options;

  QMap<QString, QColor> m_interactionComponentColors;
  QColor m_defaultInteractionComponentColor;
};

} // namespace cx::graphics
