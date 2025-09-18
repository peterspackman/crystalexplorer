#pragma once
#include "atom_label_options.h"
#include "billboardrenderer.h"
#include "bimap.h"
#include "chemicalstructure.h"
#include "colormap.h"
#include "crystalplane.h"
#include "cylinderimpostorrenderer.h"
#include "cylinderrenderer.h"
#include "drawingstyle.h"
#include "ellipsoidrenderer.h"
#include "frameworkrenderer.h"
#include "linerenderer.h"
#include "mesh.h"
#include "meshinstance.h"
#include "meshinstancerenderer.h"
#include "meshrenderer.h"
#include "plane.h"
#include "planeinstance.h"
#include "planerenderer.h"
#include "pointcloudinstancerenderer.h"
#include "rendereruniforms.h"
#include "renderselection.h"
#include "scene_export_data.h"
#include "sphereimpostorrenderer.h"

namespace cx::graphics {

struct TextLabel {
  QString text;
  QVector3D position;
};

struct AggregateIndex {
  FragmentIndex fragment;
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

  void setAtomLabelOptions(const AtomLabelOptions &);
  [[nodiscard]] const AtomLabelOptions &atomLabelOptions() const;
  void toggleShowAtomLabels();

  void setShowHydrogenAtomEllipsoids(bool show);
  [[nodiscard]] bool showHydrogenAtomEllipsoids() const;
  void toggleShowHydrogenAtomEllipsoids();

  void setAtomStyle(AtomDrawingStyle);
  void setBondStyle(BondDrawingStyle);
  void setDrawingStyle(DrawingStyle);

  [[nodiscard]] DrawingStyle drawingStyle() const;
  [[nodiscard]] AtomDrawingStyle atomStyle() const;
  [[nodiscard]] BondDrawingStyle bondStyle() const;
  [[nodiscard]] AggregateIndex getAggregateIndex(size_t index) const;

  [[nodiscard]] float bondThickness() const;

  void updateRendererUniforms(const RendererUniforms &);

  [[nodiscard]] inline const auto &frameworkOptions() const {
    return m_frameworkOptions;
  }
  void setFrameworkOptions(const FrameworkOptions &);

  // New plane management
  void updatePlanes();

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
  [[nodiscard]] inline double getThermalEllipsoidProbability() const {
    return m_thermalEllipsoidProbability;
  }

  [[nodiscard]] MeshInstance *getMeshInstance(size_t index) const;
  [[nodiscard]] int getMeshInstanceIndex(MeshInstance *) const;

signals:
  void meshesChanged();

public slots:
  void childAddedToStructure(QObject *);
  void childRemovedFromStructure(QObject *);
  void childVisibilityChanged();
  void childPropertyChanged();
  void updateThermalEllipsoidProbability(double);

  // Export convenience methods
  void getCurrentAtomsForExport(SceneExportData &data) const;
  void getCurrentBondsForExport(SceneExportData &data) const;
  void getCurrentFrameworkForExport(SceneExportData &data) const;
  void getCurrentMeshesForExport(SceneExportData &data) const;
  // TODO: Camera export is currently broken, needs investigation
  // void getCurrentCameraForExport(SceneExportData& data) const;

private:
  void connectMeshSignals(Mesh *mesh);
  void connectPlaneSignals(Plane *plane);
  bool needsUpdate();
  void initStructureChildren();

  void handleLabelsUpdate();
  void handleAtomsUpdate();
  void handleBondsUpdate();
  void handleCellsUpdate();
  void handleInteractionsUpdate();
  void handleMeshesUpdate();

  void addAggregateRepresentations();

  void clearMeshRenderers();
  void addFaceHighlightsForMeshInstance(Mesh *, MeshInstance *);
  QList<TextLabel> getCurrentLabels();

  [[nodiscard]] bool shouldSkipAtom(int idx) const;

  bool m_atomsNeedsUpdate{true};
  bool m_bondsNeedsUpdate{true};
  bool m_meshesNeedsUpdate{true};
  bool m_labelsNeedsUpdate{true};
  bool m_cellsNeedsUpdate{true};
  bool m_planesNeedUpdate{true};

  bool m_showHydrogens{true};
  bool m_showSuppressedAtoms{false};
  bool m_showHydrogenAtomEllipsoids{true};

  bool m_showCells{false};
  bool m_showMultipleCells{false};

  AtomLabelOptions m_atomLabelOptions;

  // helper for keeping track of mesh selection
  BiMap<MeshInstance *> m_meshMap;

  DrawingStyle m_drawingStyle{DrawingStyle::BallAndStick};
  AtomDrawingStyle m_atomStyle{AtomDrawingStyle::CovalentRadiusSphere};
  BondDrawingStyle m_bondStyle{BondDrawingStyle::Stick};
  double m_thermalEllipsoidProbability{0.50};

  RenderSelection *m_selectionHandler{nullptr};
  LineRenderer *m_bondLineRenderer{nullptr};
  LineRenderer *m_highlightRenderer{nullptr};
  EllipsoidRenderer *m_ellipsoidRenderer{nullptr};
  CylinderRenderer *m_cylinderRenderer{nullptr};
  SphereImpostorRenderer *m_sphereImpostorRenderer{nullptr};
  CylinderImpostorRenderer *m_cylinderImpostorRenderer{nullptr};
  std::vector<MeshInstanceRenderer *> m_meshRenderers;
  std::vector<PointCloudInstanceRenderer *> m_pointCloudRenderers;
  FrameworkRenderer *m_frameworkRenderer{nullptr};
  BillboardRenderer *m_labelRenderer{nullptr};

  LineRenderer *m_cellLinesRenderer{nullptr};
  PlaneRenderer *m_planeRenderer{nullptr};

  ChemicalStructure *m_structure{nullptr};

  RendererUniforms m_uniforms;
  FrameworkOptions m_frameworkOptions;
  std::vector<AggregateIndex> m_aggregateIndices;
};

} // namespace cx::graphics
