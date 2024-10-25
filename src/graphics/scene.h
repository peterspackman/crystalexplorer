#pragma once
#include <QMatrix4x4>
#include <QModelIndex>
#include <QPair>
#include <QtOpenGL>

#include "orientation.h"

#include "atom_label_options.h"
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
#include "frameworkoptions.h"
#include "json.h"
#include "linerenderer.h"
#include "measurementrenderer.h"
#include "mesh.h"
#include "meshrenderer.h"
#include "orbitcamera.h"
#include "rendereruniforms.h"
#include "renderselection.h"
#include "selection_information.h"
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

struct MeasurementObject {
  QVector3D position;
  cx::graphics::SelectionType selectionType;
  int index{-1};
  bool wholeObject{false};
};

struct DistanceMeasurementPoints {
  bool valid{false};
  QVector3D a;
  QVector3D b;
};

class Scene : public QObject {
  Q_OBJECT

public:
  Scene();
  Scene(ChemicalStructure *);

  [[nodiscard]] nlohmann::json toJson() const;
  bool fromJson(const nlohmann::json &);

  inline ChemicalStructure *chemicalStructure() { return m_structure; }
  inline const ChemicalStructure *chemicalStructure() const {
    return m_structure;
  }

  void resetViewAndSelections();
  bool hasOnScreenCloseContacts();
  void setSelectStatusForAtomDoubleClick(int);
  bool processSelectionDoubleClick(const QColor &);
  bool processSelectionSingleClick(const QColor &);
  bool processHitsForSingleClickSelectionWithAltKey(const QColor &color);
  bool processSelectionForInformation(const QColor &);
  MeasurementObject processMeasurementSingleClick(const QColor &,
                                                  bool wholeObject = false);
  cx::graphics::SelectionType decodeSelectionType(const QColor &);

  void selectAtomsSeparatedBySurface(bool inside);

  void setTransformationMatrix(const QMatrix4x4 &tMatrix);

  inline ScenePeriodicity periodicity() const {
    return ScenePeriodicity::ThreeDimensions;
  }

  const Orientation &orientation() const { return m_orientation; }
  Orientation &orientation() { return m_orientation; }

  void updateForPreferencesChange();

  QStringList uniqueElementSymbols() const;

  inline occ::Mat3 directCellMatrix() const {
    return m_structure->cellVectors();
  }
  inline occ::Mat3 inverseCellMatrix() const {
    return m_structure->cellVectors().inverse();
  }

  void setNeedsUpdate();
  bool anyAtomHasAdp() const;

  GLfloat scale() { return m_orientation.scale(); }
  void draw();
  void drawForPicking();
  inline auto selectionType() const { return m_selection.type; }

  const SelectedAtom &selectedAtom() const;
  const SelectedBond &selectedBond() const;
  const SelectedSurface &selectedSurface() const;

  bool showCells();
  void setShowCells(bool show);
  bool showMultipleCells();
  void setShowMultipleCells(bool);

  inline bool showAtomLabels() const { return atomLabelOptions().showAtoms; }
  inline bool showFragmentLabels() const { return atomLabelOptions().showFragment; }

  AtomLabelOptions atomLabelOptions() const;
  void setAtomLabelOptions(const AtomLabelOptions &);
  void toggleShowAtomLabels();

  void setFrameworkOptions(const FrameworkOptions &options);

  bool showHydrogenAtoms() const;
  void setShowHydrogenAtoms(bool show);
  void toggleShowHydrogenAtoms();

  void setShowSuppressedAtoms(bool);
  inline bool suppressedAtomsAreVisible() { return m_showSuppressedAtoms; }
  inline void setHydrogenBondsVisible(bool show) { m_showHydrogenBonds = show; }

  void selectAtomsOutsideRadiusOfSelectedAtoms(float);
  void setSelectStatusForAllAtoms(bool set);

  // Measurements
  void addMeasurement(const Measurement &m);
  void removeLastMeasurement();
  void removeAllMeasurements();
  bool hasMeasurements() const;

  inline auto origin() const { return m_structure->origin(); }
  bool hasVisibleAtoms() const;
  const QString &title() const { return m_name; }
  inline void setTitle(const QString &name) { m_name = name; }

  DistanceMeasurementPoints
  positionsForDistanceMeasurement(const MeasurementObject &,
                                  const MeasurementObject &) const;

  DistanceMeasurementPoints
  positionsForDistanceMeasurement(const MeasurementObject &,
                                  const QVector3D &) const;

  void setSelectionColor(const QColor &);
  void setDrawingStyle(DrawingStyle);
  DrawingStyle drawingStyle();
  void updateNoneProperties();

  void cycleDisorderHighlighting();
  bool applyDisorderColoring();
  void togglePairHighlighting(bool);

  void colorFragmentsByEnergyPair(FragmentPairSettings settings = {});
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

  void toggleDrawHydrogenEllipsoids(bool hEllipsoids);
  inline bool drawHydrogenEllipsoids() const { return _drawHydrogenEllipsoids; }

  void setModelViewProjection(const QMatrix4x4 &, const QMatrix4x4 &,
                              const QMatrix4x4 &);

  void generateAllExternalFragments();
  void generateInternalFragment();
  void generateExternalFragment();

  bool hasAllAtomsSelected();

  occ::Vec3 convertToCartesian(const occ::Vec3 &) const;
  void resetOrigin();
  void translateOrigin(const occ::Vec3 &);
  float radius() const;

  void deleteFragmentContainingAtomIndex(int);
  void resetAllAtomColors();
  void bondSelectedAtoms();
  void unbondSelectedAtoms();

  void filterAtoms(AtomFlag flag, bool state);

  void reset();

  void suppressSelectedAtoms();
  void unsuppressSelectedAtoms();
  void unsuppressAllAtoms();
  void setSelectStatusForSuppressedAtoms(bool status);
  void selectAllAtoms();
  void invertSelection();

  void expandAtomsWithinRadius(float radius, bool selection = true);

  void deleteIncompleteFragments();
  void completeAllFragments();
  void completeFragmentContainingAtom(int atomIndex);

  void colorSelectedAtoms(const QColor &, bool fragments = false);

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
  void generateSlab(SlabGenerationOptions);

  double getThermalEllipsoidProbability() const;

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

  void updateThermalEllipsoidProbability(double);
  void updateHydrogenBondCriteria(HBondCriteria);
  void updateCloseContactsCriteria(int, CloseContactCriteria);
  void handleSurfacesNeedUpdate();

private slots:
  void handleLabelsNeedUpdate();

private:
  void drawChemicalStructure();
  void drawExtras();
  void ensureRenderersInitialized();
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

  void updateCrystalPlanes();

  void drawLights();
  void drawMeasurements();
  void drawPlanes();

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
  OrbitCamera m_camera;

  LineRenderer *m_hydrogenBondLines{nullptr};
  LineRenderer *m_closeContactLines{nullptr};
  cx::graphics::MeasurementRenderer *m_measurementRenderer{nullptr};

  EllipsoidRenderer *m_lightPositionRenderer{nullptr};
  CrystalPlaneRenderer *m_crystalPlaneRenderer{nullptr};
  QMap<QString, Orientation> _savedOrientations;

  bool m_drawLights{false};
  bool m_lightTracksCamera{false};

  Orientation m_orientation;

  bool m_showSuppressedAtoms{false};
  bool m_showHydrogenBonds{false};
  HBondCriteria m_hbondCriteria;

  QMap<int, CloseContactCriteria> m_closeContactCriteria;

  cx::graphics::SelectionResult m_selection;

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

  ChemicalStructure *m_structure{nullptr};
  cx::graphics::RenderSelection *m_selectionHandler{nullptr};

  cx::graphics::ChemicalStructureRenderer *m_structureRenderer{nullptr};

  FragmentColorSettings m_fragmentColorSettings;

  SelectedAtom m_selectedAtom;
  SelectedBond m_selectedBond;
  SelectedSurface m_selectedSurface;
  void populateSelectedAtom();
  void populateSelectedBond();
  void populateSelectedSurface();

  cx::graphics::RendererUniforms m_uniforms;
};
