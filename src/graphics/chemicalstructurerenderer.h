#pragma once
#include "billboardrenderer.h"
#include "chemicalstructure.h"
#include "colormap.h"
#include "cylinderrenderer.h"
#include "drawingstyle.h"
#include "ellipsoidrenderer.h"
#include "frameworkrenderer.h"
#include "linerenderer.h"
#include "mesh.h"
#include "meshinstance.h"
#include "meshinstancerenderer.h"
#include "meshrenderer.h"
#include "pointcloudrenderer.h"
#include "rendereruniforms.h"
#include "renderselection.h"

namespace cx::graphics {


struct TextLabel {
  QString text;
  QVector3D position;
};

class ChemicalStructureRenderer : public QObject {
  Q_OBJECT
public:
  ChemicalStructureRenderer(ChemicalStructure *, QObject *parent = nullptr);
  void update(ChemicalStructure *);

  void setSelectionHandler(RenderSelection *);

  void setShowHydrogenAtoms(bool show);
  [[nodiscard]] bool showHydrogenAtoms() const;
  void toggleShowHydrogenAtoms();

  void setShowSuppressedAtoms(bool show);
  [[nodiscard]] bool showSuppressedAtoms() const;
  void toggleShowSuppressedAtoms();

  void setShowAtomLabels(bool show);
  [[nodiscard]] bool showAtomLabels() const;
  void toggleShowAtomLabels();

  void setAtomStyle(AtomDrawingStyle);
  void setBondStyle(BondDrawingStyle);

  [[nodiscard]] AtomDrawingStyle atomStyle() const;
  [[nodiscard]] BondDrawingStyle bondStyle() const;

  [[nodiscard]] float bondThickness() const;

  void updateRendererUniforms(const RendererUniforms &);

  [[nodiscard]] inline const auto &frameworkOptions() const {
    return m_frameworkOptions;
  }
  void setFrameworkOptions(const FrameworkOptions &);

  void updateAtoms();
  void updateLabels();
  void updateBonds();
  void updateInteractions();
  void updateMeshes();

  void beginUpdates();
  void endUpdates();

  void draw(bool forPicking = false);

  [[nodiscard]] inline CylinderRenderer *cylinderRenderer() {
    return m_cylinderRenderer;
  }
  [[nodiscard]] inline EllipsoidRenderer *ellipsoidRenderer() {
    return m_ellipsoidRenderer;
  }
  [[nodiscard]] inline LineRenderer *lineRenderer() { return m_lineRenderer; }

  [[nodiscard]] MeshInstance *getMeshInstance(size_t index) const;

signals:
  void meshesChanged();

public slots:
  void childAddedToStructure(QObject *);
  void childRemovedFromStructure(QObject *);
  void childVisibilityChanged();
  void childPropertyChanged();

private:
  bool needsUpdate();
  void initStructureChildren();

  void handleLabelsUpdate();
  void handleAtomsUpdate();
  void handleBondsUpdate();
  void handleInteractionsUpdate();
  void handleMeshesUpdate();

  void clearMeshRenderers();
  QList<TextLabel> getCurrentLabels();

  [[nodiscard]] bool shouldSkipAtom(int idx) const;

  bool m_atomsNeedsUpdate{true};
  bool m_bondsNeedsUpdate{true};
  bool m_meshesNeedsUpdate{true};
  bool m_labelsNeedsUpdate{true};

  bool m_showHydrogens{true};
  bool m_showAtomLabels{false};
  bool m_showSuppressedAtoms{false};
  float m_bondThicknessFactor{0.325};

  // helper for keeping track of mesh selection
  std::vector<MeshInstance *> m_meshIndexToMesh;

  AtomDrawingStyle m_atomStyle{AtomDrawingStyle::CovalentRadiusSphere};
  BondDrawingStyle m_bondStyle{BondDrawingStyle::Stick};

  RenderSelection *m_selectionHandler{nullptr};
  LineRenderer *m_lineRenderer{nullptr};
  LineRenderer *m_highlightRenderer{nullptr};
  EllipsoidRenderer *m_ellipsoidRenderer{nullptr};
  CylinderRenderer *m_cylinderRenderer{nullptr};
  std::vector<MeshInstanceRenderer *> m_meshRenderers;
  PointCloudRenderer *m_pointCloudRenderer{nullptr};
  FrameworkRenderer *m_frameworkRenderer{nullptr};
  BillboardRenderer *m_labelRenderer{nullptr};

  ChemicalStructure *m_structure{nullptr};

  QMap<QString, ColorMapName> m_propertyColorMaps;

  RendererUniforms m_uniforms;
  FrameworkOptions m_frameworkOptions;
};

} // namespace cx::graphics
