#pragma once
#include <QMatrix4x4>
#include <QPair>
#include <QtOpenGL>
#include <QModelIndex>

#include "deprecatedcrystal.h"
#include "frameworkdescription.h"
#include "orientation.h"

#include "billboardrenderer.h"
#include "chemicalstructure.h"
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
#include "settings.h"
#include "sphereimpostorrenderer.h"
#include "renderselection.h"
#include "chemicalstructurerenderer.h"
#include "rendereruniforms.h"

enum class HighlightMode { Normal, Pair };
typedef QPair<QString, QVector3D> Label;
enum class ScenePeriodicity {
  ZeroDimensions,
  OneDimension,
  TwoDimensions,
  ThreeDimensions
};

// N.B. The indices in the map must be consecutive
static QMap<int, FrameworkType> getCycleToFrameworkMapping() {
  QMap<int, FrameworkType> map;
  map[0] = coulombFramework;
  map[1] = dispersionFramework;
  map[2] = totalFramework;
  map[3] = annotatedTotalFramework;
  return map;
}
const QMap<int, FrameworkType> cycleToFramework = getCycleToFrameworkMapping();

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


class Scene : public QObject {
  Q_OBJECT

  friend QDataStream &operator<<(QDataStream &, const Scene &);
  friend QDataStream &operator>>(QDataStream &, Scene &);

public:

  Scene();
  Scene(const XYZFile &);
  Scene(ChemicalStructure *);

  inline ChemicalStructure *chemicalStructure() { return m_structure; }
  inline DeprecatedCrystal *crystal() { return m_deprecatedCrystal; }
  inline const DeprecatedCrystal *crystal() const {
    return m_deprecatedCrystal;
  }

  void resetViewAndSelections();
  bool hasOnScreenCloseContacts();
  QSet<int> fragmentsConnectedToFragmentWithHydrogenBonds(int);
  QSet<int> fragmentsConnectedToFragmentWithCloseContacts(int, int);
  QList<int> fragmentsConnectedToFragmentWithOnScreenContacts(int);
  void setSelectStatusForAtomDoubleClick(int);
  bool processSelectionDoubleClick(const QColor &);
  bool processSelectionSingleClick(const QColor &);
  bool processHitsForSingleClickSelectionWithAltKey(const QColor &color);
  bool processSelectionForInformation(const QColor &);
  QVector4D processMeasurementSingleClick(const QColor &,
                                          bool doubleClick = false);
  cx::graphics::SelectionType decodeSelectionType(const QColor &);

  void selectAtomsSeparatedBySurface(bool inside);

  void setTransformationMatrix(const QMatrix4x4 &tMatrix);

  const Orientation &orientation() const { return m_orientation; }
  Orientation &orientation() { return m_orientation; }

  inline Matrix3q directCellMatrix() const {
    if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
      return crystal()->unitCell().directCellMatrix();
    } else {
      return Matrix3q::Identity();
    }
  }
  inline Matrix3q inverseCellMatrix() const {
    if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
      return crystal()->unitCell().inverseCellMatrix();
    } else {
      return Matrix3q::Identity();
    }
  }

  inline const auto &periodicity() const { return m_periodicity; }

  void updateForPreferencesChange();

  QStringList uniqueElementSymbols() const;

  inline void setNeedsUpdate() {
    m_atomsNeedUpdate = true;
    m_surfacesNeedUpdate = true;
    m_labelsNeedUpdate = true;
  }

  bool anyAtomHasAdp() const;

  GLfloat scale() { return m_orientation.scale(); }
  void draw();
  void drawForPicking();
  inline auto selectionType() const { return m_selection.type; }
  void resetNoSelection() { m_selection.type = cx::graphics::SelectionType::None; }
  const SelectedAtom &selectedAtom() const;
  std::vector<int> selectedAtomIndices() const;
  const SelectedBond &selectedBond() const;
  Surface *selectedSurface();
  int selectedSurfaceIndex();
  int selectedSurfaceFaceIndex();

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

  inline void setHydrogenBondsVisible(bool show) { _showHydrogenBonds = show; }

  void setCloseContactVisible(int, bool);
  void selectAtomsOutsideRadiusOfSelectedAtoms(float);

  inline int energyCycleIndex() const { return _energyCycleIndex; }

  void setSelectStatusForAllAtoms(bool set);

  // Measurements
  void addMeasurement(const Measurement &m);
  void removeLastMeasurement();
  void removeAllMeasurements();
  bool hasMeasurements() const;

  inline Vector3q origin() const {
    if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
      return crystal()->origin();
    }
    return m_structure->origin();
  }

  bool hasVisibleAtoms() const;

  const QString &title() const { return m_name; }
  inline void setTitle(const QString &name) { m_name = name; }

  QPair<QVector3D, QVector3D>
  positionsForDistanceMeasurement(const QPair<cx::graphics::SelectionType, int> &,
                                  const QPair<cx::graphics::SelectionType, int> &) const;
  QPair<QVector3D, QVector3D>
  positionsForDistanceMeasurement(const QPair<cx::graphics::SelectionType, int> &,
                                  const QVector3D &pos) const;

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
  FragmentPairStyle currentEnergyFrameworkStyle();

  void cycleDisorderHighlighting();
  bool applyDisorderColoring();
  void togglePairHighlighting(bool);

  void toggleFragmentColors();
  void colorFragmentsByEnergyPair();
  void clearFragmentColors();

  void cycleEnergyFramework(bool);
  void turnOffEnergyFramework();
  void turnOnEnergyFramework();
  FrameworkType currentFramework();
  int minCycleIndex();
  int maxCycleIndex();

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

  void setThermalEllipsoidProbabilityString(const QString &);
  QString thermalEllipsoidProbabilityString() {
    return _ellipsoidProbabilityString;
  }

  void toggleDrawHydrogenEllipsoids(bool hEllipsoids);
  bool drawHydrogenEllipsoids() { return _drawHydrogenEllipsoids; }

  void setModelViewProjection(const QMatrix4x4 &, const QMatrix4x4 &,
                              const QMatrix4x4 &);
  void exportToPovrayTextStream(QTextStream &);

  inline const CrystalSurfaceHandler *surfaceHandler() const {
    return m_surfaceHandler;
  }
  inline CrystalSurfaceHandler *surfaceHandler() { return m_surfaceHandler; }

  void generateAllExternalFragments();
  void generateInternalFragment();
  void generateExternalFragment();

  bool hasAllAtomsSelected();

  // Surfaces
  void deleteSelectedSurface();
  bool setCurrentSurfaceIndex(int);
  void resetCurrentSurface();
  void toggleVisibilityOfSurface(int);
  void hideSurface(int);
  void deleteCurrentSurface();
  void setSurfaceVisibilities(bool);
  bool loadSurfaceData(const JobParameters &);

  Surface *currentSurface();
  Surface *surface(int surfaceIndex);

  bool hasSurface() const;
  bool isCurrentSurface(int index) const;
  bool hasVisibleSurfaces();
  bool hasHiddenSurfaces();
  int numberOfVisibleSurfaces();

  QStringList listOfSurfaceTitles();
  QVector<bool> listOfSurfaceVisibilities();
  QVector<QVector3D> listOfSurfaceCentroids();
  int numberOfSurfaces() const;
  int surfaceEquivalent(const JobParameters &) const;
  int propertyEquivalentToRequestedProperty(const JobParameters &, int) const;
  void deleteSurface(Surface *);

  // Cloning
  void cloneCurrentSurfaceForSelection();
  void cloneCurrentSurfaceForAllFragments();
  void cloneCurrentSurfaceWithCellShifts(const QVector3D &);

  // Cloning
  Surface *cloneSurface(const Surface *);
  Surface *cloneSurfaceWithCellShift(const Surface *, Vector3q);
  Surface *cloneSurfaceForFragment(int, const Surface *);
  void cloneCurrentSurfaceForFragmentList(const QVector<int>);
  bool cloneAlreadyExists(const Surface *, CrystalSymops);
  Surface *existingClone(const Surface *, CrystalSymops);
  bool hasFragmentsWithoutClones(Surface *);

  // fingerprints
  void toggleAtomsForFingerprintSelectionFilter(bool show);
  void resetSurfaceFeatures();

  Vector3q convertToCartesian(const Vector3q &) const;
  void resetOrigin();
  void translateOrigin(const Vector3q &);
  float radius() const;

  void deleteFragmentContainingAtomIndex(int);
  void discardSelectedAtoms();
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

private slots:
  void handleAtomsNeedUpdate();
  void handleSurfacesNeedUpdate();
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

  bool isAtomName(int) const;
  bool isBondName(int) const;
  bool isSurfaceName(int) const;
  int atomBasename() const;
  int bondBasename() const;
  int surfacesBasename() const;
  int getAtomIndexFromAtomName(int) const;
  int getBondIndexFromBondName(int) const;
  QPair<int, int> surfaceIndexAndFaceFromName(int) const;
  int numberOfBonds() const;
  int numberOfAtoms() const;
  int basenameForSurfaceWithIndex(int) const;
  int numberOfFacesToDrawForAllSurfaces() const;

  void updateAtomsForDrawing();
  void updateBondsForDrawing();
  void updateMeshesForDrawing();
  void updateLabelsForDrawing();

  void updateAtomsFromCrystal();
  void updateAtomsFromChemicalStructure();
  void updateBondsFromCrystal();
  void updateBondsFromChemicalStructure();
  void updateCrystalPlanes();

  void drawAtomsAndBonds();
  void drawEllipsoids(bool for_picking = false);
  void drawSpheres(bool for_picking = false);
  void drawCylinders(bool for_picking = false);
  void drawMeshes(bool for_picking = false);
  void drawSurfaces(bool for_picking = false);
  void drawWireframe(bool for_picking = false);

  void drawLights();
  void drawLines();
  void drawLabels();
  void drawUnitCellBox();
  void drawMeasurements();
  void drawEnergyFrameworks();

  void setLightPositionsBasedOnCamera();

  bool skipBond(int atom_i, int atom_j);
  float bondThickness();
  float contactLineThickness();
  void putBondAlongZAxis(QVector3D);
  void drawHydrogenBonds();
  void drawCloseContacts();
  QColor getColorForCloseContact(int);

  AtomDrawingStyle atomStyle() const;
  BondDrawingStyle bondStyle() const;
  void setDisorderCycleHideShow(Atom &);
  QColor getDisorderColor(const Atom &);

  void updateRendererUniforms();
  void setRendererUniforms(QOpenGLShaderProgram *, bool selection_mode = false);
  void setRendererUniforms(Renderer *, bool selection_mode = false);

  QMatrix3x3 atomThermalEllipsoidTransformationMatrix(const Atom &) const;

  QString m_name;
  DeprecatedCrystal *m_deprecatedCrystal{nullptr};
  QList<Measurement> m_measurementList;

  OrbitCamera m_camera;
  QMap<QString, Renderer*> m_renderers;
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

  LineRenderer *m_energyFrameworkLines{nullptr};
  CylinderRenderer *m_energyFrameworkCylinders{nullptr};
  EllipsoidRenderer *m_energyFrameworkSpheres{nullptr};
  BillboardRenderer *m_energyFrameworkLabels{nullptr};

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
  bool _showHydrogenBonds;
  QList<bool> _showCloseContacts;

  cx::graphics::SelectionResult m_selection;
  QColor m_selectionColor;

  DrawingStyle m_drawingStyle{DrawingStyle::BallAndStick};

  bool m_depthFogEnabled{settings::GLOBAL_DEPTH_FOG_ENABLED};

  int _disorderCycleIndex;
  int _energyCycleIndex;
  bool _showEnergyFramework;

  QColor _backgroundColor;
  QString _ellipsoidProbabilityString;
  float _ellipsoidProbabilityScaleFactor;
  bool _drawHydrogenEllipsoids;
  bool _drawMultipleCellBoxes;

  bool m_atomsNeedUpdate{true};
  bool m_surfacesNeedUpdate{true};
  bool m_labelsNeedUpdate{true};
  bool m_crystalPlanesNeedUpdate{true};
  std::vector<CrystalPlane> m_crystalPlanes;

  HighlightMode _highlightMode;

  QMap<QString, Mesh> m_meshes;

  CrystalSurfaceHandler *m_surfaceHandler{nullptr};
  ScenePeriodicity m_periodicity{ScenePeriodicity::ThreeDimensions};
  ChemicalStructure *m_structure{nullptr};
  cx::graphics::RenderSelection *m_selectionHandler{nullptr};

  cx::graphics::ChemicalStructureRenderer *m_structureRenderer{nullptr};

  SelectedAtom m_selectedAtom;
  SelectedBond m_selectedBond;
  void populateSelectedAtom();
  void populateSelectedBond();

  cx::graphics::RendererUniforms m_uniforms;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &, const Scene &);
QDataStream &operator>>(QDataStream &, Scene &);
