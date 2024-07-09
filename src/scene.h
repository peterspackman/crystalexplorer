#pragma once
#include <QMatrix4x4>
#include <QModelIndex>
#include <QPair>
#include <QtOpenGL>

#include "orientation.h"
#include "qeigen.h"

#include "billboardrenderer.h"
#include "chemicalstructure.h"
#include "chemicalstructurerenderer.h"
#include "circlerenderer.h"
#include "crystalplane.h"
#include "crystalplanerenderer.h"
#include "crystalstructure.h"
#include "cylinderimpostorrenderer.h"
#include "cylinderrenderer.h"
#include "drawingstyle.h"
#include "ellipsoidrenderer.h"
#include "linerenderer.h"
#include "measurement.h"
#include "mesh.h"
#include "meshrenderer.h"
#include "orbitcamera.h"
#include "rendereruniforms.h"
#include "renderselection.h"
#include "settings.h"
#include "sphereimpostorrenderer.h"

enum class HighlightMode { Normal, Pair };
typedef QPair<QString, QVector3D> Label;
enum class ScenePeriodicity {
  ZeroDimensions,
  OneDimension,
  TwoDimensions,
  ThreeDimensions
};

class XYZFile;

struct SelectedAtom {
  int index{-1};
  int atomicNumber{-1};
  QVector3D position;
  QString label;
};

struct SelectedBond {
  int index{-1};
  SelectedAtom a;
  SelectedAtom b;
};

struct SelectedSurface {
  int index{-1};
  int faceIndex{-1};
  MeshInstance *surface{nullptr};
  float propertyValue{0.0};
  QString property{"None"};
};

struct MeasurementObject {
  QVector3D position;
  cx::graphics::SelectionType selectionType;
  int index{-1};
  bool wholeObject{false};
};

struct DistanceMeasurementPoints{
  bool valid{false};
  QVector3D a;
  QVector3D b;
};

class Scene : public QObject {
  Q_OBJECT

  friend QDataStream &operator<<(QDataStream &, const Scene &);
  friend QDataStream &operator>>(QDataStream &, Scene &);

public:
  Scene();
  Scene(const XYZFile &);
  Scene(ChemicalStructure *);

  inline ChemicalStructure *chemicalStructure() { return m_structure; }

  void resetViewAndSelections();
  bool hasOnScreenCloseContacts();
  void setSelectStatusForAtomDoubleClick(int);
  bool processSelectionDoubleClick(const QColor &);
  bool processSelectionSingleClick(const QColor &);
  bool processHitsForSingleClickSelectionWithAltKey(const QColor &color);
  bool processSelectionForInformation(const QColor &);
  MeasurementObject processMeasurementSingleClick(const QColor &, bool wholeObject = false);
  cx::graphics::SelectionType decodeSelectionType(const QColor &);

  void selectAtomsSeparatedBySurface(bool inside);

  void setTransformationMatrix(const QMatrix4x4 &tMatrix);

  inline ScenePeriodicity periodicity() const { return ScenePeriodicity::ThreeDimensions; }

  const Orientation &orientation() const { return m_orientation; }
  Orientation &orientation() { return m_orientation; }

  void updateForPreferencesChange();

  QStringList uniqueElementSymbols() const;

  inline Matrix3q directCellMatrix() const {
    return m_structure->cellVectors();
  }
  inline Matrix3q inverseCellMatrix() const {
    return m_structure->cellVectors().inverse();
  }

  inline void setNeedsUpdate() {
    m_labelsNeedUpdate = true;
  }

  bool anyAtomHasAdp() const;

  GLfloat scale() { return m_orientation.scale(); }
  void draw();
  void drawForPicking();
  inline auto selectionType() const { return m_selection.type; }
  void resetNoSelection() {
    m_selection.type = cx::graphics::SelectionType::None;
  }
  const SelectedAtom &selectedAtom() const;
  std::vector<int> selectedAtomIndices() const;
  const SelectedBond &selectedBond() const;
  const SelectedSurface &selectedSurface() const;

  bool unitCellBoxIsVisible() { return _showUnitCellBox; }
  void setUnitCellBoxVisible(bool show) { _showUnitCellBox = show; }
  void enableMultipleUnitCellBoxes(bool enable) {
    _drawMultipleCellBoxes = enable;
    if (m_unitCellLines != nullptr) {
      delete m_unitCellLines;
      m_unitCellLines = nullptr;
    }
  }
  inline bool atomicLabelsVisible() const { return _showAtomicLabels; }
  inline void setAtomicLabelsVisible(bool show) { _showAtomicLabels = show; }

  void setShowHydrogens(bool show);
  inline bool hydrogensAreVisible() const { return _showHydrogens; }

  void setShowSuppressedAtoms(bool);
  inline bool suppressedAtomsAreVisible() { return _showSuppressedAtoms; }

  inline void setHydrogenBondsVisible(bool show) { m_showHydrogenBonds = show; }

  void selectAtomsOutsideRadiusOfSelectedAtoms(float);

  void setSelectStatusForAllAtoms(bool set);

  // Measurements
  void addMeasurement(const Measurement &m);
  void removeLastMeasurement();
  void removeAllMeasurements();
  bool hasMeasurements() const;

  inline Vector3q origin() const { return m_structure->origin(); }

  bool hasVisibleAtoms() const;

  const QString &title() const { return m_name; }
  inline void setTitle(const QString &name) { m_name = name; }

  DistanceMeasurementPoints positionsForDistanceMeasurement(
      const MeasurementObject &,
      const MeasurementObject &) const;

  DistanceMeasurementPoints positionsForDistanceMeasurement(
      const MeasurementObject &,
      const QVector3D &) const;


  QVector<Label> labels();
  QVector<Label> measurementLabels();
  QVector<Label> atomicLabels();
  QVector<Label> fragmentLabels();
  QVector<Label> surfaceLabels();
  QVector<Label> energyLabels();
  void setSelectionColor(const QColor &);
  void setDrawingStyle(DrawingStyle);
  DrawingStyle drawingStyle();
  void updateNoneProperties();

  void cycleDisorderHighlighting();
  bool applyDisorderColoring();
  void togglePairHighlighting(bool);

  void toggleFragmentColors();
  void colorFragmentsByEnergyPair();
  void clearFragmentColors();

  void setShowSurfaceInteriors(bool);
  void setShowCloseContacts(bool);

  QPair<cx::graphics::SelectionType, int>
  selectionTypeAndIndexOfGraphicalObject(int openGlObjectName,
                                         bool firstAtomOfBond = false);
  void saveOrientation(QString);
  void resetOrientationToSavedOrientation(QString);
  QList<QString> listOfSavedOrientationNames();

  inline const QColor &backgroundColor() { return _backgroundColor; }
  inline void setBackgroundColor(const QColor &color) {
    _backgroundColor = color;
  }

  inline void setThermalEllipsoidProbabilityString(const QString &s) { _ellipsoidProbabilityString = s; }
  QString thermalEllipsoidProbabilityString() {
    return _ellipsoidProbabilityString;
  }

  void toggleDrawHydrogenEllipsoids(bool hEllipsoids);
  bool drawHydrogenEllipsoids() { return _drawHydrogenEllipsoids; }

  void setModelViewProjection(const QMatrix4x4 &, const QMatrix4x4 &,
                              const QMatrix4x4 &);

  void generateAllExternalFragments();
  void generateInternalFragment();
  void generateExternalFragment();

  bool hasAllAtomsSelected();

  // fingerprints
  void toggleAtomsForFingerprintSelectionFilter(bool show);

  Vector3q convertToCartesian(const Vector3q &) const;
  void resetOrigin();
  void translateOrigin(const Vector3q &);
  float radius() const;

  void deleteFragmentContainingAtomIndex(int);
  void resetAllAtomColors();
  void bondSelectedAtoms();
  void unbondSelectedAtoms();

  void reset();

  void suppressSelectedAtoms();
  void unsuppressSelectedAtoms();
  void unsuppressAllAtoms();
  void setSelectStatusForSuppressedAtoms(bool status);
  void selectAllAtoms();
  void invertSelection();

  void expandAtomsWithinRadius(float radius, bool selection = true);

  void deleteIncompleteFragments();
  void deleteSelectedAtoms();
  void completeAllFragments();
  void completeFragmentContainingAtom(int atomIndex);

  void colorSelectedAtoms(const QColor &);

  bool hasHydrogens() const;
  bool hasSelectedAtoms() const;
  bool hasSuppressedAtoms() const;
  bool hasIncompleteFragments() const;

  int numberOfSelectedAtoms() const;
  bool hasAtomsWithCustomColor() const;

  const std::vector<CrystalPlane> &crystalPlanes() const {
    return m_crystalPlanes;
  }
  void setCrystalPlanes(const std::vector<CrystalPlane> &);

  void generateCells(QPair<QVector3D, QVector3D>);

signals:
  void clickedSurface(QModelIndex);
  void clickedSurfacePropertyValue(float);

  void contactAtomExpanded();
  void surfaceVisibilityChanged();
  void viewChanged();
  void sceneContentsChanged();
  void atomSelectionChanged();
  void structureChanged();

public slots:
  void screenGammaChanged();
  void materialChanged();
  void lightSettingsChanged();
  void textSettingsChanged();
  void depthFogSettingsChanged();
  void addCrystalPlane(CrystalPlane);

  void updateHydrogenBondCriteria(HBondCriteria);
  void updateCloseContactsCriteria(int, CloseContactCriteria);
  void handleSurfacesNeedUpdate();

private slots:
  void handleLabelsNeedUpdate();

private:
  // Cloning
  void rebuildSurfaceParentCloneRelationship();

  void init();
  void setSurfaceLightingToDefaults();
  void setViewAngleAndScaleToDefaults();
  void setShowStatusesToDefaults();
  void setSelectionStatusToDefaults();
  int nameWithSmallestZ(GLuint, GLuint[]);

  int numberOfBonds() const;
  int numberOfAtoms() const;

  void updateLabelsForDrawing();

  void updateCrystalPlanes();

  void drawLights();
  void drawLines();
  void drawLabels();
  void drawUnitCellBox();
  void drawMeasurements();

  void setLightPositionsBasedOnCamera();

  float bondThickness();
  float contactLineThickness();
  void drawHydrogenBonds();
  void drawCloseContacts();

  AtomDrawingStyle atomStyle() const;
  BondDrawingStyle bondStyle() const;

  void updateRendererUniforms();
  void setRendererUniforms(QOpenGLShaderProgram *, bool selection_mode = false);
  void setRendererUniforms(Renderer *, bool selection_mode = false);

  QString m_name;
  QList<Measurement> m_measurementList;

  OrbitCamera m_camera;
  QMap<QString, Renderer *> m_renderers;
  CylinderImpostorRenderer *m_cylinderImpostorRenderer{nullptr};
  CylinderRenderer *m_meshCylinderRenderer{nullptr};
  EllipsoidRenderer *m_ellipsoidRenderer{nullptr};
  QVector<MeshRenderer *> m_meshRenderers;
  QVector<LineRenderer *> m_lineRenderers;
  LineRenderer *m_wireFrameBonds{nullptr};
  LineRenderer *m_faceHighlights{nullptr};

  // TODO merge line renderers
  LineRenderer *m_measurementLines{nullptr};
  CircleRenderer *m_measurementCircles{nullptr};
  BillboardRenderer *m_measurementLabels{nullptr};

  LineRenderer *m_hydrogenBondLines{nullptr};
  LineRenderer *m_closeContactLines{nullptr};
  LineRenderer *m_unitCellLines{nullptr};

  CircleRenderer *m_circles{nullptr};
  EllipsoidRenderer *m_lightPositionRenderer{nullptr};
  BillboardRenderer *m_billboardTextLabels{nullptr};
  CrystalPlaneRenderer *m_crystalPlaneRenderer{nullptr};
  QMap<QString, Orientation> _savedOrientations;

  bool m_drawLights{false};
  bool m_lightTracksCamera{false};

  Orientation m_orientation;

  bool _showHydrogens;
  bool _showSuppressedAtoms;
  bool _showUnitCellBox;
  bool _showAtomicLabels;
  bool _showFragmentLabels;
  bool _showSurfaceLabels;

  bool m_showHydrogenBonds{false};
  HBondCriteria m_hbondCriteria;

  QMap<int, CloseContactCriteria> m_closeContactCriteria;

  cx::graphics::SelectionResult m_selection;
  QColor m_selectionColor;

  DrawingStyle m_drawingStyle{DrawingStyle::BallAndStick};

  bool m_depthFogEnabled{settings::GLOBAL_DEPTH_FOG_ENABLED};

  int _disorderCycleIndex;

  QColor _backgroundColor;
  QString _ellipsoidProbabilityString;
  float _ellipsoidProbabilityScaleFactor;
  bool _drawHydrogenEllipsoids;
  bool _drawMultipleCellBoxes;

  bool m_labelsNeedUpdate{true};
  bool m_crystalPlanesNeedUpdate{true};
  bool m_hydrogenBondsNeedUpdate{true};
  bool m_closeContactsNeedUpdate{true};

  std::vector<CrystalPlane> m_crystalPlanes;

  HighlightMode _highlightMode;

  QMap<QString, Mesh> m_meshes;

  ChemicalStructure *m_structure{nullptr};
  cx::graphics::RenderSelection *m_selectionHandler{nullptr};

  cx::graphics::ChemicalStructureRenderer *m_structureRenderer{nullptr};

  SelectedAtom m_selectedAtom;
  SelectedBond m_selectedBond;
  SelectedSurface m_selectedSurface;
  void populateSelectedAtom();
  void populateSelectedBond();
  void populateSelectedSurface();

  cx::graphics::RendererUniforms m_uniforms;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &, const Scene &);
QDataStream &operator>>(QDataStream &, Scene &);
