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
#include "pointcloudinstancerenderer.h"
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

  void setShowCells(bool show);
  bool showCells() const;
  void toggleShowCells();

  void setShowMultipleCells(bool show);
  bool showMultipleCells() const;
  void toggleShowMultipleCells();

  void setShowAtomLabels(bool show);
  [[nodiscard]] bool showAtomLabels() const;
  void toggleShowAtomLabels();

  void setShowHydrogenAtomEllipsoids(bool show);
  [[nodiscard]] bool showHydrogenAtomEllipsoids() const;
  void toggleShowHydrogenAtomEllipsoids();


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
  void updateCells();
  void updateLabels();
  void updateBonds();
  void updateInteractions();
  void updateMeshes();

  void beginUpdates();
  void endUpdates();

  void forceUpdates();
  void draw(bool forPicking = false);

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
  void handleCellsUpdate();
  void handleInteractionsUpdate();
  void handleMeshesUpdate();

  void clearMeshRenderers();
  QList<TextLabel> getCurrentLabels();

  [[nodiscard]] bool shouldSkipAtom(int idx) const;

  bool m_atomsNeedsUpdate{true};
  bool m_bondsNeedsUpdate{true};
  bool m_meshesNeedsUpdate{true};
  bool m_labelsNeedsUpdate{true};
  bool m_cellsNeedsUpdate{true};

  bool m_showHydrogens{true};
  bool m_showAtomLabels{false};
  bool m_showSuppressedAtoms{false};
  bool m_showHydrogenAtomEllipsoids{true};

  bool m_showCells{false};
  bool m_showMultipleCells{false};

  // helper for keeping track of mesh selection
  std::vector<MeshInstance *> m_meshIndexToMesh;

  AtomDrawingStyle m_atomStyle{AtomDrawingStyle::CovalentRadiusSphere};
  BondDrawingStyle m_bondStyle{BondDrawingStyle::Stick};

  RenderSelection *m_selectionHandler{nullptr};
  LineRenderer *m_bondLineRenderer{nullptr};
  LineRenderer *m_highlightRenderer{nullptr};
  EllipsoidRenderer *m_ellipsoidRenderer{nullptr};
  CylinderRenderer *m_cylinderRenderer{nullptr};
  std::vector<MeshInstanceRenderer *> m_meshRenderers;
  std::vector<PointCloudInstanceRenderer *> m_pointCloudRenderers;
  FrameworkRenderer *m_frameworkRenderer{nullptr};
  BillboardRenderer *m_labelRenderer{nullptr};

  LineRenderer *m_cellLinesRenderer{nullptr};

  ChemicalStructure *m_structure{nullptr};

  QMap<QString, ColorMapName> m_propertyColorMaps;

  RendererUniforms m_uniforms;
  FrameworkOptions m_frameworkOptions;
};

} // namespace cx::graphics
