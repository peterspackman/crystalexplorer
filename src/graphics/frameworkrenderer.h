#pragma once
#include "billboardrenderer.h"
#include "chemicalstructure.h"
#include "colormap.h"
#include "cylinderimpostorrenderer.h"
#include "cylinderrenderer.h"
#include "ellipsoidrenderer.h"
#include "frameworkoptions.h"
#include "linerenderer.h"
#include "rendereruniforms.h"
#include "sphereimpostorrenderer.h"

namespace cx::graphics {
class SceneExportData;
}

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

  void forceUpdates();

  void setModel(const QString &);
  void setComponent(const QString &);

  void draw(bool forPicking = false);

  // Export convenience method
  void getCurrentFrameworkForExport(SceneExportData &data) const;

  [[nodiscard]] inline CylinderRenderer *cylinderRenderer() {
    return m_cylinderRenderer;
  }
  [[nodiscard]] inline EllipsoidRenderer *ellipsoidRenderer() {
    return m_ellipsoidRenderer;
  }
  [[nodiscard]] inline SphereImpostorRenderer *sphereImpostorRenderer() {
    return m_sphereImpostorRenderer;
  }
  [[nodiscard]] inline CylinderImpostorRenderer *cylinderImpostorRenderer() {
    return m_cylinderImpostorRenderer;
  }
  [[nodiscard]] inline LineRenderer *lineRenderer() { return m_lineRenderer; }

signals:
  void frameworkChanged();

private:
  struct FrameworkTube {
    QVector3D startPos;
    QVector3D endPos;
    QColor color;
    float radius;
    QString label;
  };

  void handleInteractionsUpdate();
  std::pair<QVector3D, QVector3D> getPairPositions(const FragmentDimer &) const;
  std::vector<FrameworkTube> generateFrameworkTubes() const;

  [[nodiscard]] bool shouldSkipAtom(int idx) const;

  bool m_needsUpdate{true};
  float m_thicknessScale{1.0};

  LineRenderer *m_lineRenderer{nullptr};
  EllipsoidRenderer *m_ellipsoidRenderer{nullptr};
  CylinderRenderer *m_cylinderRenderer{nullptr};
  SphereImpostorRenderer *m_sphereImpostorRenderer{nullptr};
  CylinderImpostorRenderer *m_cylinderImpostorRenderer{nullptr};
  BillboardRenderer *m_labelRenderer{nullptr};

  ChemicalStructure *m_structure{nullptr};

  PairInteractions *m_interactions{nullptr};
  RendererUniforms m_uniforms;

  FrameworkOptions m_options;

  QMap<QString, QColor> m_interactionComponentColors;
  QColor m_defaultInteractionComponentColor;
};

} // namespace cx::graphics
