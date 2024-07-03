#include <QDebug>
#include <QtOpenGL>

#include "crystalplanegenerator.h"
#include "drawingstyle.h"
#include "elementdata.h"
#include "fmt/core.h"
#include "globals.h"
#include "graphics.h"
#include "interactions.h"
#include "mathconstants.h"
#include "scene.h"
#include "settings.h"
#include "wavefrontobjfile.h"
#include "xyzfile.h"

using cx::graphics::SelectionType;

Scene::Scene(const XYZFile &xyz) : m_structure(new ChemicalStructure()) {
  m_structure->setAtoms(xyz.getAtomSymbols(), xyz.getAtomPositions());
  m_structure->updateBondGraph();
  init();
}

Scene::Scene(ChemicalStructure *structure) : m_structure(structure) { init(); }

Scene::Scene() : m_structure(new ChemicalStructure()) { init(); }

void Scene::init() {

  m_name = "Empty";

  m_uniforms.u_depthFogDensity = settings::GLOBAL_DEPTH_FOG_DENSITY;
  m_uniforms.u_depthFogOffset = settings::GLOBAL_DEPTH_FOG_OFFSET;

  setViewAngleAndScaleToDefaults();
  setShowStatusesToDefaults();
  setSelectionStatusToDefaults();
  setSurfaceLightingToDefaults();

  _backgroundColor = QColor(
      settings::readSetting(settings::keys::BACKGROUND_COLOR).toString());

  m_drawingStyle = GLOBAL_DRAWING_STYLE;

  _highlightMode = HighlightMode::Normal;
  _disorderCycleIndex = 0;

  _drawHydrogenEllipsoids = true;

  _drawMultipleCellBoxes = false;

  screenGammaChanged();
  materialChanged();
  lightSettingsChanged();

  if (m_structure) {
    connect(m_structure, &ChemicalStructure::childAdded, this,
            &Scene::structureChanged);
    connect(m_structure, &ChemicalStructure::childRemoved, this,
            &Scene::structureChanged);
  }

  m_selectionHandler = new cx::graphics::RenderSelection(this);
}

void Scene::setSurfaceLightingToDefaults() {
  m_uniforms.u_materialMetallic =
      settings::readSetting(settings::keys::MATERIAL_METALLIC).toFloat();
  m_uniforms.u_materialRoughness =
      settings::readSetting(settings::keys::MATERIAL_ROUGHNESS).toFloat();
  m_uniforms.u_renderMode =
      settings::readSetting(settings::keys::MATERIAL).toInt();
}

void Scene::setViewAngleAndScaleToDefaults() { m_orientation = Orientation(); }

void Scene::setShowStatusesToDefaults() {
  _showHydrogens = true;
  _showSuppressedAtoms = true;
  _showUnitCellBox = false;
  _showAtomicLabels = false;
  _showFragmentLabels = false;
  _showSurfaceLabels = false;
  m_showHydrogenBonds = false;
  _showCloseContacts.clear();
  _showCloseContacts = {false, false, false};
}

void Scene::setShowCloseContacts(bool set) {
  m_structure->setShowVanDerWaalsContactAtoms(set);
}

void Scene::setSelectStatusForAllAtoms(bool set) {
  m_structure->setFlagForAllAtoms(AtomFlag::Selected, set);
}

void Scene::addMeasurement(const Measurement &m) {
  auto idx = m_measurementList.size();
  m_measurementList.append(m);
  auto func = ColorMapFunc(ColorMapName::Github);
  func.lower = 0.0;
  func.upper = m_measurementList.size();
  m_measurementList.back().setColor(func(idx));
}

void Scene::removeLastMeasurement() {
  if (m_measurementList.size() > 0)
    m_measurementList.removeLast();
}

void Scene::removeAllMeasurements() { m_measurementList.clear(); }

bool Scene::hasMeasurements() const { return !m_measurementList.isEmpty(); }

void Scene::setSelectionColor(const QColor &color) { m_selectionColor = color; }

AtomDrawingStyle Scene::atomStyle() const {
  return atomStyleForDrawingStyle(m_drawingStyle);
}
BondDrawingStyle Scene::bondStyle() const {
  return bondStyleForDrawingStyle(m_drawingStyle);
}

void Scene::setDrawingStyle(DrawingStyle style) {
  m_drawingStyle = style;
  if (m_structureRenderer) {
    m_structureRenderer->setAtomStyle(atomStyle());
    m_structureRenderer->setBondStyle(bondStyle());
  }
}

DrawingStyle Scene::drawingStyle() { return m_drawingStyle; }

void Scene::setSelectionStatusToDefaults() {
  m_selection.type = SelectionType::None;
  m_selection.index = -1;
  m_selection.secondaryIndex = -1;
}

void Scene::resetViewAndSelections() {
  setViewAngleAndScaleToDefaults();
  setShowStatusesToDefaults();
  setSelectionStatusToDefaults();
}

void Scene::saveOrientation(QString orientationName) {
  _savedOrientations[orientationName] = m_orientation;
}

void Scene::resetOrientationToSavedOrientation(QString orientationName) {
  m_orientation = _savedOrientations[orientationName];
}

QList<QString> Scene::listOfSavedOrientationNames() {
  return _savedOrientations.keys();
}

bool Scene::anyAtomHasAdp() const {
  // TODO add ADP support
  return false;
}

/// \brief Returns all the labels (i.e. textual annotations)
/// That need to be rendered by glWindow on top of the scene
QVector<Label> Scene::labels() {
  QVector<Label> labels;
  if (_showAtomicLabels) {
    labels.append(atomicLabels());
  }
  if (_showFragmentLabels) {
    // TODO fragment labels
  }
  if (_showSurfaceLabels) {
    // TODO surface labels
  }
  if (hasMeasurements()) {
    labels.append(measurementLabels());
  }

  return labels;
}

QVector<Label> Scene::atomicLabels() {
  QVector<Label> labels;
  const auto &atomLabels = m_structure->labels();
  const auto &positions = m_structure->atomicPositions();
  for (int i = 0; i < m_structure->numberOfAtoms(); i++) {
    if (m_structure->testAtomFlag(i, AtomFlag::Contact))
      continue;
    labels.append({atomLabels[i], QVector3D(positions(0, i), positions(1, i),
                                            positions(2, i))});
  }
  return labels;
}

QVector<Label> Scene::fragmentLabels() {
  QVector<Label> labels;

  /*
  QVector<QVector3D> centroids = crystal()->centroidsOfFragments();
  for (int i = 0; i < centroids.size(); ++i) {
    // labels.append(qMakePair(QString("Fragment %1").arg(i), centroids[i]));
    labels.append(qMakePair(QString("%1").arg(i), centroids[i]));
  }
  */
  return labels;
}

QVector<Label> Scene::surfaceLabels() {
  QVector<Label> labels;

  /*
  QStringList titles = listOfSurfaceTitles();
  if (titles.size() > 0) {
    auto centroids = listOfSurfaceCentroids();

    Q_ASSERT(titles.size() == centroids.size());
    for (int i = 0; i < titles.size(); ++i) {
      labels.append(qMakePair(titles[i], centroids[i]));
    }
  }
  */
  return labels;
}

void Scene::generateCells(QPair<QVector3D, QVector3D> cellLimits) {
  m_structure->packUnitCells(cellLimits);
}

QVector<Label> Scene::measurementLabels() {
  QVector<Label> labels;
  for (const auto &measurement : m_measurementList) {
    labels.append({measurement.label(), measurement.labelPosition()});
  }
  return labels;
}

QVector<Label> Scene::energyLabels() {
  QVector<Label> labels;
  /*
  for (const auto &fragPairInfo : std::as_const(crystal()->energyInfos())) {
    labels.append(
        qMakePair(fragPairInfo.label(), fragPairInfo.labelPosition()));
  }
  */
  return labels;
}

void Scene::updateNoneProperties() {
  // TODO update none properties
  // surfaceHandler()->updateAllSurfaceNoneProperties();
  m_surfacesNeedUpdate = true;
}

/*!
 \brief Returns the name of the hit with the smallest z else returns -1;
 We expect two names in the hierachy, therefore is we don't get two names we
 have clicked
 on a graphics primitive without a name so we return -1.
 */
int Scene::nameWithSmallestZ(GLuint hits, GLuint buffer[]) {
  GLuint *names;
  GLuint numberOfNames;
  GLuint minZ = 0xffffffff;

  names = 0;
  numberOfNames = 0;

  GLuint *ptr = (GLuint *)buffer;
  for (uint h = 0; h < hits; ++h) {
    GLuint numNames = *ptr;
    ptr++;
    if (*ptr < minZ) { // found a closer object so (i) update minZ (ii) save the
      // number of names and the names themselves
      minZ = *ptr;
      numberOfNames = numNames;
      names = ptr + 2;
    }
    ptr += numberOfNames + 2; // move to beginning of next hit record
  }

  if (numberOfNames == 2) {
    return *(names + 1);
  } else {
    return -1;
  }
}

bool Scene::hasOnScreenCloseContacts() {
  return m_showHydrogenBonds || _showCloseContacts.contains(true);
}

void Scene::setSelectStatusForAtomDoubleClick(int atomIndex) {
  if (m_structure->testAtomFlag(atomIndex, AtomFlag::Contact))
    return;
  int fragmentIndex = m_structure->fragmentIndexForAtom(atomIndex);
  const auto &atomIndices = m_structure->atomsForFragment(fragmentIndex);
  m_structure->setAtomFlag(atomIndex, AtomFlag::Selected, true);
  bool selected =
      std::all_of(atomIndices.begin(), atomIndices.end(), [&](int x) {
        return m_structure->atomFlagsSet(x, AtomFlag::Selected);
      });
  m_structure->setFlagForAtoms(atomIndices, AtomFlag::Selected, !selected);
}

void Scene::selectAtomsSeparatedBySurface(bool inside) {
  // TODO
}

bool Scene::processSelectionDoubleClick(const QColor &color) {
  m_selection = m_selectionHandler->getSelectionFromColor(color);

  switch (m_selection.type) {
  case SelectionType::Atom: {
    setSelectStatusForAtomDoubleClick(m_selection.index);
    return true;
  }
  case SelectionType::Bond: {
    int bondIndex = m_selection.index;
    int atomIndex = m_structure->atomsForBond(bondIndex).first;
    setSelectStatusForAtomDoubleClick(atomIndex);
    return true;
  }
  default:
    break;
  }
  return false;
}

void Scene::handleSurfacesNeedUpdate() { m_surfacesNeedUpdate = true; }

void Scene::handleLabelsNeedUpdate() { m_labelsNeedUpdate = true; }

bool Scene::processSelectionForInformation(const QColor &color) {
  m_selection = m_selectionHandler->getSelectionFromColor(color);

  switch (m_selection.type) {
  case SelectionType::Atom: {
    return true;
  }
  case SelectionType::Bond: {
    return true;
  }
  case SelectionType::Surface: {
    return true;
  }
  default:
    break;
  }
  return false;
}

bool Scene::processSelectionSingleClick(const QColor &color) {
  m_selection = m_selectionHandler->getSelectionFromColor(color);
  qDebug() << "Process selection single click:" << color;

  switch (m_selection.type) {
  case SelectionType::Atom: {
    qDebug() << "Selection type: Atom";
    size_t atom_idx = m_selection.index;
    // TODO handle contact atom
    if (m_structure->testAtomFlag(atom_idx, AtomFlag::Contact)) {
      m_structure->completeFragmentContaining(atom_idx);
      emit contactAtomExpanded();
    } else {
      m_structure->atomFlags(atom_idx) ^= AtomFlag::Selected;
      emit atomSelectionChanged();
    }
    return true;
    break;
  }
  case SelectionType::Bond: {
    qDebug() << "Selection type: Bond";
    size_t bond_idx = m_selection.index;
    const auto bondedAtoms = m_structure->atomsForBond(bond_idx);
    auto &flags_a = m_structure->atomFlags(bondedAtoms.first);
    auto &flags_b = m_structure->atomFlags(bondedAtoms.second);
    if ((flags_a & AtomFlag::Selected) != (flags_b & AtomFlag::Selected)) {
      flags_a |= AtomFlag::Selected;
      flags_b |= AtomFlag::Selected;
    } else {
      // toggle
      flags_a ^= AtomFlag::Selected;
      flags_b ^= AtomFlag::Selected;
    }
    emit atomSelectionChanged();
    return true;
    break;
  }
  case SelectionType::Surface: {
    qDebug() << "Selection type: Surface";
    size_t surfaceIndex = m_selection.index;
    qDebug() << "Surface index clicked:" << surfaceIndex;

    MeshInstance *meshInstance = m_structureRenderer->getMeshInstance(m_selection.index);

    if (!meshInstance)
      break;

    emit clickedSurface(
        m_structure->treeModel()->indexFromObject(meshInstance));

    float propertyValue =
        meshInstance->valueForSelectedPropertyAt(m_selection.secondaryIndex);
    emit clickedSurfacePropertyValue(propertyValue);

    return true;
    break;
  }
  default: {
    qDebug() << "Selection type: None";
    break;
  }
  }
  return false;
}

// An alt-click on the surface changes the view to:
// (i) centre the surface
// (ii) rotate the view to look from the de atom to the di atom of the clicked
// on surface triangle
// This only has an effect on hirshfeld surfaces
bool Scene::processHitsForSingleClickSelectionWithAltKey(const QColor &color) {

  m_selection = m_selectionHandler->getSelectionFromColor(color);
  switch (m_selection.type) {
  case SelectionType::Surface: {
    // TODO handle surface selection
    /*
    size_t surfaceIndex = m_selection.index;

    Surface *surface = surfaceHandler()->surfaceFromIndex(surfaceIndex);
    if (!surface)
      break;

    // name belonged to a Hirshfeld surface?
    if (surface->isHirshfeldBased()) {
      int faceIndex = surface->faceIndexForVertex(m_selection.secondaryIndex);
      // Recenter of clicked on surface
      QVector3D centroid = surface->centroid();
      Vector3q center;
      center << centroid.x(), centroid.y(), centroid.z();
      crystal()->setOrigin(center);

      // Look at surface from de atom to di atom
      AtomId diAtomId = surface->insideAtomIdForFace(faceIndex);
      Atom diAtom = crystal()->generateAtomFromIndexAndShift(
          diAtomId.unitCellIndex, diAtomId.shift);

      AtomId deAtomId = surface->outsideAtomIdForFace(faceIndex);
      Atom deAtom = crystal()->generateAtomFromIndexAndShift(
          deAtomId.unitCellIndex, deAtomId.shift);
      QVector3D qv = deAtom.pos() - diAtom.pos();
      QMatrix4x4 t = m_orientation.transformationMatrix();
      cx::graphics::viewDownVector(qv, t);
      m_orientation.setTransformationMatrix(t);
      // Force redraw
      emit viewChanged();
    }
    */
    return true;
  }
  default:
    break;
  }
  return false;
}

void Scene::setTransformationMatrix(const QMatrix4x4 &T) {
  m_orientation.setTransformationMatrix(T);
  m_camera.setView(T);
}

// A control-click (command-click on mac) causes the surface to highlight
// surface patches
// void DrawableCrystal::processHitsForSingleClickSelectionWithControlKey(
//    GLuint hits, GLuint buffer[]) {
//  // Get the topmost GL name
//  int nameSelected = nameWithSmallestZ(hits, buffer);

//  // Process named hit
//  if (nameSelected != -1) {

//    // We are only interested in surface hits...
//    QPair<int, int> surfaceAndFace =
//    surfaceIndexAndFaceFromName(nameSelected); int surfaceIndex =
//    surfaceAndFace.first; int faceIndex = surfaceAndFace.second;

//    if (surfaceIndex > -1) {
//      _surfaceList[surfaceIndex]->highlightFragmentPatchForFace(faceIndex);
//      //_surfaceList[surfaceIndex]->highlightDiDePatchForFace(faceIndex);
//      //_surfaceList[surfaceIndex]->highlightDiPatchForFace(faceIndex);
//      //_surfaceList[surfaceIndex]->highlightDePatchForFace(faceIndex);
//      //_surfaceList[surfaceIndex]->highlightCurvednessPatchForFace(faceIndex,
//      //-0.6);
//    }
//  }
//}

QVector4D Scene::processMeasurementSingleClick(const QColor &color,
                                               bool doubleClick) {
  QVector4D result(0, 0, 0, -1); // -1 indicates no hit
  m_selection = m_selectionHandler->getSelectionFromColor(color);

  switch (m_selection.type) {
  case SelectionType::Atom: {
    size_t atom_idx = m_selection.index;
    if (m_structure->atomFlagsSet(atom_idx, AtomFlag::Contact))
      break;
    if (doubleClick)
      m_structure->selectFragmentContaining(atom_idx);
    else {
      m_structure->atomFlags(atom_idx) ^= AtomFlag::Selected;
    }
    emit atomSelectionChanged();
    Vector3q pos = m_structure->atomicPositions().col(atom_idx);
    result = QVector4D(pos.x(), pos.y(), pos.z(), atom_idx);
    break;
  }
  case SelectionType::Bond: {
    size_t bond_idx = m_selection.index;
    auto atomsForBond = m_structure->atomsForBond(bond_idx);
    auto &flags_a = m_structure->atomFlags(atomsForBond.first);
    auto &flags_b = m_structure->atomFlags(atomsForBond.second);
    if ((flags_a & AtomFlag::Contact) && (flags_b & AtomFlag::Contact))
      break;
    if (doubleClick)
      m_structure->selectFragmentContaining(atomsForBond.first);
    else {
      flags_a ^= AtomFlag::Selected;
      flags_b ^= AtomFlag::Selected;
    }
    const auto &positions = m_structure->atomicPositions();
    Vector3q pos = 0.5 * (positions.col(atomsForBond.first) +
                          positions.col(atomsForBond.second));
    result = QVector4D(pos.x(), pos.y(), pos.z(), bond_idx);
    break;
  }
  case SelectionType::Surface: {
    size_t surfaceIndex = m_selection.index;
    auto *meshInstance = m_structureRenderer->getMeshInstance(surfaceIndex);
    qDebug() << "Found meshInstance:" << meshInstance;
    if (!meshInstance)
      break;
    // TODO finalize surface picking
    break;
  }
  default:
    break;
  }

  return result;
}

void Scene::populateSelectedSurface() {
  m_selectedSurface.index = m_selection.index;
  m_selectedSurface.faceIndex = m_selection.secondaryIndex;
  auto * surface = m_structureRenderer->getMeshInstance(m_selection.index);
  m_selectedSurface.surface = surface;

  if(surface) {
    m_selectedSurface.property = surface->getSelectedProperty();
    m_selectedSurface.propertyValue = surface->valueForSelectedPropertyAt(m_selection.secondaryIndex);
  }
}

void Scene::populateSelectedAtom() {
  m_selectedAtom.index = m_selection.index;
  m_selectedAtom.atomicNumber = m_structure->atomicNumbers()(m_selection.index);
  m_selectedAtom.label = m_structure->labels()[m_selection.index];
  const auto pos = m_structure->atomicPositions().col(m_selection.index);
  m_selectedAtom.position = QVector3D(pos.x(), pos.y(), pos.z());
}

void Scene::populateSelectedBond() {
  m_selectedBond.index = m_selection.index;
  const auto [idx_a, idx_b] = m_structure->atomsForBond(m_selection.index);
  {
    auto &atomInfo = m_selectedBond.a;
    atomInfo.index = idx_a;
    atomInfo.atomicNumber = m_structure->atomicNumbers()(idx_a);
    atomInfo.label = m_structure->labels()[idx_a];
    const auto pos = m_structure->atomicPositions().col(idx_a);
    atomInfo.position = QVector3D(pos.x(), pos.y(), pos.z());
  }
  {
    auto &atomInfo = m_selectedBond.b;
    atomInfo.index = idx_b;
    atomInfo.atomicNumber = m_structure->atomicNumbers()(idx_b);
    atomInfo.label = m_structure->labels()[idx_b];
    const auto pos = m_structure->atomicPositions().col(idx_b);
    atomInfo.position = QVector3D(pos.x(), pos.y(), pos.z());
  }
}

SelectionType Scene::decodeSelectionType(const QColor &color) {
  m_selection = m_selectionHandler->getSelectionFromColor(color);
  m_selectedAtom = {};
  m_selectedSurface = {};
  m_selectedBond = {};

  m_selection = m_selectionHandler->getSelectionFromColor(color);
  switch (m_selection.type) {
  case SelectionType::Atom: {
    populateSelectedAtom();
    break;
  }
  case SelectionType::Bond: {
    populateSelectedBond();
    break;
  }
  case SelectionType::Surface: {
    populateSelectedSurface();
    break;
  }
  default:
    break;
  }
  return m_selection.type;
}

const SelectedBond &Scene::selectedBond() const { return m_selectedBond; }

const SelectedSurface &Scene::selectedSurface() const { return m_selectedSurface; }

void Scene::updateForPreferencesChange() {
  if (m_structure) {
    setNeedsUpdate();
  }
}

QStringList Scene::uniqueElementSymbols() const {
  if (m_structure) {
    return m_structure->uniqueElementSymbols();
  }
  return {};
}

QPair<QVector3D, QVector3D> Scene::positionsForDistanceMeasurement(
    const QPair<SelectionType, int> &object1,
    const QPair<SelectionType, int> &object2) const {
  // For convenience, and to make code slightly more readable ...
  int index1 = object1.second;
  int index2 = object2.second;

  QPair<QVector3D, QVector3D> result;
  // TODO fix
  if (object1.first == SelectionType::Surface &&
      object2.first == SelectionType::Surface) {
    // Both objects selected are whole surfaces
    return result;

  } else if (object1.first == SelectionType::Surface) {
    // First object is whole surface, second is whole fragment
    return result;

  } else if (object2.first == SelectionType::Surface) {
    // First object is whole fragment, second is whole surface
    return result;
  } else {
    // Both objects selected are whole fragments
    return result;
  }
}
QPair<QVector3D, QVector3D>
Scene::positionsForDistanceMeasurement(const QPair<SelectionType, int> &object,
                                       const QVector3D &pos) const {
  // TODO fix
  QPair<QVector3D, QVector3D> result;
  if (object.first == SelectionType::Surface) {
    // First object is whole surface, second is a single atom or
    return result;
  } else {
    // First object is whole fragment, second is single atom or
    // single triangle
    return result;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Atom Picking Names
//
////////////////////////////////////////////////////////////////////////////////////////////////
int Scene::numberOfAtoms() const { return m_structure->numberOfAtoms(); }

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Bond Picking Names
//
////////////////////////////////////////////////////////////////////////////////////////////////

int Scene::numberOfBonds() const { return m_structure->covalentBonds().size(); }

void Scene::setCloseContactVisible(int contactIndex, bool show) {
  Q_ASSERT(contactIndex >= 0 && contactIndex <= CCMAX_INDEX);
  _showCloseContacts[contactIndex] = show;
}

bool Scene::hasVisibleAtoms() const { return m_structure->numberOfAtoms() > 0; }

void Scene::updateRendererUniforms() {
  auto time = std::chrono::steady_clock::now().time_since_epoch();

  QColor settingsSelectionColor =
      QColor(settings::readSetting(settings::keys::SELECTION_COLOR).toString());
  float settingsExposure =
      settings::readSetting(settings::keys::LIGHTING_EXPOSURE).toFloat();
  int settingsToneMap =
      settings::readSetting(settings::keys::LIGHTING_TONEMAP).toInt();

  QVector4D selectionColor(settingsSelectionColor.redF(),
                           settingsSelectionColor.greenF(),
                           settingsSelectionColor.blueF(), 1.0f);
  GLint vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  QVector2D viewportSize(vp[2], vp[3]);
  if (m_lightTracksCamera) {
    setLightPositionsBasedOnCamera();
  }
  QVector3D fogColor =
      QVector3D(m_depthFogEnabled ? _backgroundColor.redF() : -1.0,
                _backgroundColor.greenF(), _backgroundColor.blueF());
  m_uniforms.u_pointSize = 10 * m_orientation.scale();
  m_uniforms.u_selectionColor = selectionColor;
  m_uniforms.u_selectionMode = false;
  // camera
  m_uniforms.u_scale = m_orientation.scale();
  m_uniforms.u_viewMat = m_camera.view();
  m_uniforms.u_modelMat = m_camera.model();
  m_uniforms.u_projectionMat = m_camera.projection();
  m_uniforms.u_modelViewMat = m_camera.modelView();
  m_uniforms.u_modelViewMatInv = m_camera.modelViewInverse();
  m_uniforms.u_viewMatInv = m_camera.viewInverse();
  m_uniforms.u_modelViewProjectionMat = m_camera.modelViewProjection();
  m_uniforms.u_lightingExposure = settingsExposure;
  m_uniforms.u_toneMapIdentifier = settingsToneMap;
  m_uniforms.u_viewport_size = viewportSize;
  m_uniforms.u_ortho =
      (m_camera.projectionType() == CameraProjection::Orthographic) ? 1.0f
                                                                    : 0.0f;
  m_uniforms.u_normalMat = m_camera.normal();
  m_uniforms.u_cameraPosVec = m_camera.location();
  m_uniforms.u_time = std::chrono::duration<float>(time).count();
  m_uniforms.u_depthFogColor = fogColor;

  if (m_structureRenderer) {
    m_structureRenderer->updateRendererUniforms(m_uniforms);
  }
}

////////////////////////////////////////////////////////////////////////////////////
//
// Draws all parts of the crystal
// (i) Atoms and bonds
// (ii) Close contacts
// (iii) Measurements
// (iv) Surfaces
// (v) Unit cell box
//
////////////////////////////////////////////////////////////////////////////////////
void Scene::drawForPicking() {
  updateRendererUniforms();
  const auto storedRenderMode = m_uniforms.u_renderMode;
  m_uniforms.u_renderMode = 0;
  m_uniforms.u_selectionMode = true;
  m_structureRenderer->draw(true);
  m_uniforms.u_renderMode = storedRenderMode;
  m_uniforms.u_selectionMode = false;
}

void Scene::draw() {
  // needs to be done with a current opengl context
  if (m_structure && !m_structureRenderer) {
    m_structureRenderer =
        new cx::graphics::ChemicalStructureRenderer(m_structure, this);
    m_structureRenderer->setSelectionHandler(m_selectionHandler);
    m_structureRenderer->setAtomStyle(atomStyle());
    m_structureRenderer->setBondStyle(bondStyle());
    connect(m_structureRenderer,
            &cx::graphics::ChemicalStructureRenderer::meshesChanged, this,
            &Scene::sceneContentsChanged);
  }
  updateRendererUniforms();

  if (_showUnitCellBox) {
    drawUnitCellBox();
  }

  if (m_structureRenderer) {
    m_structureRenderer->draw();
  }

  if (hasVisibleAtoms()) {

    if (m_showHydrogenBonds) {
      drawHydrogenBonds();
    }
    drawCloseContacts();
    drawMeasurements();
  }

  if (_showAtomicLabels) {
    updateLabelsForDrawing();
    drawLabels();
  }

  if (m_drawLights) {
    drawLights();
  }

  updateCrystalPlanes();
  m_crystalPlaneRenderer->bind();
  setRendererUniforms(m_crystalPlaneRenderer, false);
  m_crystalPlaneRenderer->draw();
  m_crystalPlaneRenderer->release();
}

void Scene::setModelViewProjection(const QMatrix4x4 &model,
                                   const QMatrix4x4 &view,
                                   const QMatrix4x4 &projection) {
  m_camera.setModel(model);
  m_camera.setView(view);
  m_camera.setProjection(projection);
}

void Scene::setLightPositionsBasedOnCamera() {
  QVector3D pos = m_camera.location();
  float d = 2.0f;
  QVector3D right = m_camera.right() * d;
  QVector3D up = m_camera.up() * d;
  m_uniforms.u_lightPos.setColumn(0, QVector4D(pos + right * d + up * d));
  m_uniforms.u_lightPos.setColumn(1, QVector4D(pos - right * d + up * d));
  m_uniforms.u_lightPos.setColumn(2, QVector4D(-pos + right * d + up * d));
  m_uniforms.u_lightPos.setColumn(3, QVector4D(-pos - right * d + up * d));
}

void Scene::setRendererUniforms(Renderer *renderer, bool selection_mode) {
  auto prog = renderer->program();
  setRendererUniforms(prog, selection_mode);
}

void Scene::setRendererUniforms(QOpenGLShaderProgram *prog,
                                bool selection_mode) {

#define SET_UNIFORM(uniform) prog->setUniformValue(#uniform, m_uniforms.uniform)

  SET_UNIFORM(u_pointSize);
  SET_UNIFORM(u_lightSpecular);
  SET_UNIFORM(u_renderMode);
  SET_UNIFORM(u_numLights);
  SET_UNIFORM(u_lightPos);
  SET_UNIFORM(u_lightGlobalAmbient);
  SET_UNIFORM(u_selectionColor);
  SET_UNIFORM(u_selectionMode);
  SET_UNIFORM(u_scale);
  SET_UNIFORM(u_viewMat);
  SET_UNIFORM(u_modelMat);
  SET_UNIFORM(u_projectionMat);
  SET_UNIFORM(u_modelViewMat);
  SET_UNIFORM(u_modelViewMatInv);
  SET_UNIFORM(u_viewMatInv);
  SET_UNIFORM(u_normalMat);
  SET_UNIFORM(u_modelViewProjectionMat);
  SET_UNIFORM(u_cameraPosVec);
  SET_UNIFORM(u_lightingExposure);
  SET_UNIFORM(u_toneMapIdentifier);
  SET_UNIFORM(u_attenuationClamp);

  SET_UNIFORM(u_viewport_size);
  SET_UNIFORM(u_ortho);
  SET_UNIFORM(u_time);
  SET_UNIFORM(u_screenGamma);
  SET_UNIFORM(u_ellipsoidLineWidth);
  SET_UNIFORM(u_texture);
  SET_UNIFORM(u_materialRoughness);
  SET_UNIFORM(u_materialMetallic);

  SET_UNIFORM(u_textSDFOutline);
  SET_UNIFORM(u_textSDFBuffer);
  SET_UNIFORM(u_textSDFSmoothing);
  SET_UNIFORM(u_textColor);
  SET_UNIFORM(u_textOutlineColor);
  SET_UNIFORM(u_depthFogDensity);
  SET_UNIFORM(u_depthFogColor);
  SET_UNIFORM(u_depthFogOffset);

#undef SET_UNIFORM
}

void Scene::screenGammaChanged() {
  m_uniforms.u_screenGamma =
      settings::readSetting(settings::keys::SCREEN_GAMMA).toFloat();
}

void Scene::depthFogSettingsChanged() {
  m_uniforms.u_depthFogDensity =
      settings::readSetting(settings::keys::DEPTH_FOG_DENSITY).toFloat();
  m_depthFogEnabled =
      settings::readSetting(settings::keys::DEPTH_FOG_ENABLED).toBool();
  m_uniforms.u_depthFogOffset =
      settings::readSetting(settings::keys::DEPTH_FOG_OFFSET).toFloat();
}

void Scene::addCrystalPlane(CrystalPlane plane) {
  m_crystalPlanes.push_back(plane);
  m_crystalPlanesNeedUpdate = true;
}

void Scene::setCrystalPlanes(const std::vector<CrystalPlane> &planes) {
  m_crystalPlanes = planes;
  m_crystalPlanesNeedUpdate = true;
}

void Scene::materialChanged() {
  m_uniforms.u_materialMetallic =
      settings::readSetting(settings::keys::MATERIAL_METALLIC).toFloat();
  m_uniforms.u_materialRoughness =
      settings::readSetting(settings::keys::MATERIAL_ROUGHNESS).toFloat();
  m_uniforms.u_renderMode =
      settings::readSetting(settings::keys::MATERIAL).toInt();
}

void Scene::textSettingsChanged() {
  auto c2v = [](const QColor &c) {
    return QVector3D(c.redF(), c.greenF(), c.blueF());
  };

  m_uniforms.u_textColor =
      c2v(settings::readSetting(settings::keys::TEXT_COLOR).toString());
  m_uniforms.u_textOutlineColor =
      c2v(settings::readSetting(settings::keys::TEXT_OUTLINE_COLOR).toString());
  m_uniforms.u_textSDFBuffer =
      settings::readSetting(settings::keys::TEXT_BUFFER).toFloat();
  m_uniforms.u_textSDFSmoothing =
      settings::readSetting(settings::keys::TEXT_SMOOTHING).toFloat();
  m_uniforms.u_textSDFOutline =
      settings::readSetting(settings::keys::TEXT_OUTLINE).toFloat();
}

void Scene::lightSettingsChanged() {
  auto c2v = [](const QColor &c) {
    return QVector4D(c.redF(), c.greenF(), c.blueF(), 1.0f);
  };
  // light properties
  // TODO cache these uniforms, only update/set those that change for each draw.
  QColor color =
      QColor(settings::readSetting(settings::keys::LIGHT_AMBIENT).toString());
  float intensity =
      settings::readSetting(settings::keys::LIGHT_AMBIENT_INTENSITY).toFloat();
  qDebug() << "Light ambient intensity: " << intensity;
  m_uniforms.u_lightGlobalAmbient = c2v(color) * intensity;

  intensity =
      settings::readSetting(settings::keys::LIGHT_INTENSITY_1).toFloat();
  color = QColor(
      settings::readSetting(settings::keys::LIGHT_SPECULAR_1).toString());
  m_uniforms.u_lightSpecular.setColumn(0, c2v(color) * intensity);

  intensity =
      settings::readSetting(settings::keys::LIGHT_INTENSITY_2).toFloat();
  color = QColor(
      settings::readSetting(settings::keys::LIGHT_SPECULAR_2).toString());
  m_uniforms.u_lightSpecular.setColumn(1, c2v(color) * intensity);

  intensity =
      settings::readSetting(settings::keys::LIGHT_INTENSITY_3).toFloat();
  color = QColor(
      settings::readSetting(settings::keys::LIGHT_SPECULAR_3).toString());
  m_uniforms.u_lightSpecular.setColumn(2, c2v(color) * intensity);

  intensity =
      settings::readSetting(settings::keys::LIGHT_INTENSITY_4).toFloat();
  color = QColor(
      settings::readSetting(settings::keys::LIGHT_SPECULAR_4).toString());

  m_uniforms.u_lightSpecular.setColumn(3, c2v(color) * intensity);

  QVector3D pos = qvariant_cast<QVector3D>(
      settings::readSetting(settings::keys::LIGHT_POSITION_1));
  m_uniforms.u_lightPos.setColumn(0, QVector4D(pos));
  pos = qvariant_cast<QVector3D>(
      settings::readSetting(settings::keys::LIGHT_POSITION_2));
  m_uniforms.u_lightPos.setColumn(1, QVector4D(pos));
  pos = qvariant_cast<QVector3D>(
      settings::readSetting(settings::keys::LIGHT_POSITION_3));
  m_uniforms.u_lightPos.setColumn(2, QVector4D(pos));
  pos = qvariant_cast<QVector3D>(
      settings::readSetting(settings::keys::LIGHT_POSITION_4));
  m_uniforms.u_lightPos.setColumn(3, QVector4D(pos));

  m_uniforms.u_attenuationClamp =
      QVector2D(settings::readSetting(settings::keys::LIGHT_ATTENUATION_MINIMUM)
                    .toFloat(),
                settings::readSetting(settings::keys::LIGHT_ATTENUATION_MAXIMUM)
                    .toFloat());
  m_lightTracksCamera =
      settings::readSetting(settings::keys::LIGHT_TRACKS_CAMERA).toBool();
  m_drawLights =
      settings::readSetting(settings::keys::SHOW_LIGHT_POSITIONS).toBool();
  if (m_lightTracksCamera) {
    setLightPositionsBasedOnCamera();
  }
}

void Scene::drawLights() {
  if (!m_lightPositionRenderer) {
    m_lightPositionRenderer = new EllipsoidRenderer();
  }
  m_lightPositionRenderer->beginUpdates();
  m_lightPositionRenderer->clear();
  for (int i = 0; i < m_uniforms.u_numLights; i++) {
    cx::graphics::addSphereToEllipsoidRenderer(
        m_lightPositionRenderer, m_uniforms.u_lightPos.column(i).toVector3D(),
        Qt::yellow, 2.5);
  }
  m_lightPositionRenderer->endUpdates();
  m_lightPositionRenderer->bind();
  setRendererUniforms(m_lightPositionRenderer, false);
  m_lightPositionRenderer->draw();
  m_lightPositionRenderer->release();
}

void Scene::drawLines() {
  for (LineRenderer *lineRenderer : m_lineRenderers) {
    lineRenderer->bind();
    setRendererUniforms(lineRenderer);
    lineRenderer->draw();
    lineRenderer->release();
  }
}

float Scene::contactLineThickness() {
  double thicknessRatio =
      settings::readSetting(settings::keys::CONTACT_LINE_THICKNESS).toInt() /
      100.0;
  return thicknessRatio;
}

float Scene::bondThickness() {
  double bondThicknessRatio =
      settings::readSetting(settings::keys::BOND_THICKNESS).toInt() / 100.0;
  return ElementData::elementFromAtomicNumber(1)->covRadius() *
         bondThicknessRatio;
}


void Scene::updateHydrogenBondCriteria(HBondCriteria criteria) {
  m_hbondCriteria = criteria;
  m_hbondsNeedUpdate = true;
}

void Scene::drawHydrogenBonds() {
  qDebug() << "Draw hydrogenbonds";
  if (m_hydrogenBondLines == nullptr) {
    m_hydrogenBondLines = new LineRenderer();
  } else {
    m_hydrogenBondLines->clear();
  }
  // TODO check if needs update

  QColor color =
      QColor(settings::readSetting(settings::keys::HBOND_COLOR).toString());
  double radius = contactLineThickness();
  m_hydrogenBondLines->beginUpdates();

  const auto &bonds = m_structure->hydrogenBonds(m_hbondCriteria);
  const auto &positions = m_structure->atomicPositions();
  qDebug() << "Structure has" << bonds.size() << "hydrogen bonds";
  for (const auto &[d, h, a] : bonds) {
    const auto fragD = m_structure->fragmentIndexForAtom(d);
    const auto fragA = m_structure->fragmentIndexForAtom(a);
    // skip intramolecular contacts
    if (!m_hbondCriteria.includeIntra && (fragD == fragA)) {
      continue;
    }
    QVector3D pos_h(positions(0, h), positions(1, h), positions(2, h));
    QVector3D pos_a(positions(0, a), positions(1, a), positions(2, a));
    cx::graphics::addDashedLineToLineRenderer(*m_hydrogenBondLines, pos_h,
                                              pos_a, radius, color);
  }
  m_hydrogenBondLines->endUpdates();
  m_hydrogenBondLines->bind();
  setRendererUniforms(m_hydrogenBondLines);
  m_hydrogenBondLines->draw();
  m_hydrogenBondLines->release();
}

void Scene::drawCloseContacts() {
  if (m_closeContactLines == nullptr) {
    m_closeContactLines = new LineRenderer();
  } else {
    m_closeContactLines->clear();
  }
  double radius = contactLineThickness();
  m_closeContactLines->beginUpdates();

  const auto &positions = m_structure->atomicPositions();
  for (int ccIndex = 0; ccIndex <= CCMAX_INDEX; ++ccIndex) {
    if (!_showCloseContacts[ccIndex]) {
      continue; // Don't draw set of close contacts if not visible
    }

    QColor color = getColorForCloseContact(ccIndex);
    for (const auto &[a, b] : m_structure->vdwContacts()) {
      // skip intramolecular contacts
      if (m_structure->fragmentIndexForAtom(a) ==
          m_structure->fragmentIndexForAtom(b))
        continue;
      QVector3D pos_a(positions(0, a), positions(1, a), positions(2, a));
      QVector3D pos_b(positions(0, b), positions(1, b), positions(2, b));
      cx::graphics::addDashedLineToLineRenderer(*m_closeContactLines, pos_a,
                                                pos_b, radius, color);
    }
  }
  m_closeContactLines->endUpdates();
  m_closeContactLines->bind();
  setRendererUniforms(m_closeContactLines);
  m_closeContactLines->draw();
  m_closeContactLines->release();
}

QColor Scene::getColorForCloseContact(int contactIndex) {
  QColor color;
  switch (contactIndex) {
  case CC1_INDEX:
    color = QColor(
        settings::readSetting(settings::keys::CONTACT1_COLOR).toString());
    break;
  case CC2_INDEX:
    color = QColor(
        settings::readSetting(settings::keys::CONTACT2_COLOR).toString());
    break;
  case CC3_INDEX:
    color = QColor(
        settings::readSetting(settings::keys::CONTACT3_COLOR).toString());
    break;
  default:
    Q_ASSERT(false); // Shouldn't ever get here
    break;
  }
  return color;
}

void Scene::setShowSuppressedAtoms(bool show) {
  if (!show) {
    setSelectStatusForSuppressedAtoms(false);
  }
  _showSuppressedAtoms = show;
}

void Scene::expandAtomsWithinRadius(float radius, bool selection) {
  m_structure->expandAtomsWithinRadius(radius, selection);
}

void Scene::selectAtomsOutsideRadiusOfSelectedAtoms(float radius) {
  // TODO
}

void Scene::reset() {
  m_structure->resetAtomsAndBonds();
  resetViewAndSelections();
}

void Scene::updateCrystalPlanes() {
  if (!m_crystalPlanesNeedUpdate)
    return;
  if (m_crystalPlaneRenderer == nullptr)
    m_crystalPlaneRenderer = new CrystalPlaneRenderer();
  else
    m_crystalPlaneRenderer->clear();
  m_crystalPlaneRenderer->beginUpdates();
  for (const auto &plane : std::as_const(m_crystalPlanes)) {
    if (plane.hkl.h == 0 && plane.hkl.k == 0 && plane.hkl.l == 0)
      continue;
    CrystalPlaneGenerator generator(m_structure, plane.hkl);
    const auto &aVector = generator.aVector();
    const auto &bVector = generator.aVector();
    const auto &origin = generator.origin();
    QVector3D qorigin(origin(0), origin(1), origin(2));
    QVector3D qa(aVector(0), aVector(1), aVector(2));
    QVector3D qb(bVector(0), bVector(1), bVector(2));
    cx::graphics::addPlaneToCrystalPlaneRenderer(*m_crystalPlaneRenderer,
                                                 qorigin, qa, qb, plane.color);
  }
  m_crystalPlaneRenderer->endUpdates();
  m_crystalPlanesNeedUpdate = false;
}

void Scene::updateLabelsForDrawing() {
  if (!m_labelsNeedUpdate)
    return;
  if (m_billboardTextLabels == nullptr) {
    m_billboardTextLabels = new BillboardRenderer();
  } else {
    m_billboardTextLabels->clear();
  }
  const auto label_list = labels();
  if (label_list.empty())
    return;
  m_billboardTextLabels->beginUpdates();
  for (const auto &label : label_list) {
    cx::graphics::addTextToBillboardRenderer(*m_billboardTextLabels,
                                             label.second, label.first);
  }
  m_billboardTextLabels->endUpdates();
  m_labelsNeedUpdate = false;
}

void Scene::drawLabels() {
  m_billboardTextLabels->bind();
  setRendererUniforms(m_billboardTextLabels);
  m_billboardTextLabels->draw();
  m_billboardTextLabels->release();
}

void Scene::drawUnitCellBox() {
  // setup unit cell lines

  if (m_unitCellLines == nullptr) {

    m_unitCellLines = new LineRenderer();
    m_unitCellLines->beginUpdates();
    QVector3D a(1, 0, 0);
    QVector3D b(0, 1, 0);
    QVector3D c(0, 0, 1);

    const auto &unitCell = m_structure->cellVectors();
    a = QVector3D(unitCell(0, 0), unitCell(1, 0), unitCell(2, 0));
    b = QVector3D(unitCell(0, 1), unitCell(1, 1), unitCell(2, 1));
    c = QVector3D(unitCell(0, 2), unitCell(1, 2), unitCell(2, 2));
    int hmin = 0, hmax = 1;
    int kmin = 0, kmax = 1;
    int lmin = 0, lmax = 1;
    if (_drawMultipleCellBoxes) {
      hmin = -1;
      hmax = 2;
      kmin = -1, kmax = 2;
      lmin = -1;
      lmax = 2;
    }

    const QColor A_AXISCOLOR =
        settings::readSetting(settings::keys::CE_RED_COLOR).toString();
    const QColor B_AXISCOLOR =
        settings::readSetting(settings::keys::CE_GREEN_COLOR).toString();
    const QColor C_AXISCOLOR =
        settings::readSetting(settings::keys::CE_BLUE_COLOR).toString();
    const QColor UNITCELLCOLOR = QColor("#646464");

    for (int h = hmin; h <= hmax; h++) {
      QVector3D pa = h * a;
      for (int k = kmin; k <= kmax; k++) {
        QVector3D pb = k * b;
        for (int l = lmin; l <= lmax; l++) {
          QColor aColor =
              (h == 0 && k == 0 && l == 0) ? A_AXISCOLOR : UNITCELLCOLOR;
          QColor bColor =
              (h == 0 && k == 0 && l == 0) ? B_AXISCOLOR : UNITCELLCOLOR;
          QColor cColor =
              (h == 0 && k == 0 && l == 0) ? C_AXISCOLOR : UNITCELLCOLOR;
          QVector3D pc = l * c;
          QVector3D pabc = pa + pb + pc;
          if (h < hmax)
            cx::graphics::addLineToLineRenderer(
                *m_unitCellLines, pabc, pabc + a,
                DrawingStyleConstants::unitCellLineWidth, aColor);
          if (k < kmax)
            cx::graphics::addLineToLineRenderer(
                *m_unitCellLines, pabc, pabc + b,
                DrawingStyleConstants::unitCellLineWidth, bColor);
          if (l < lmax)
            cx::graphics::addLineToLineRenderer(
                *m_unitCellLines, pabc, pabc + c,
                DrawingStyleConstants::unitCellLineWidth, cColor);
        }
      }
    }
    m_unitCellLines->endUpdates();
  }

  // draw
  m_unitCellLines->bind();
  setRendererUniforms(m_unitCellLines);
  m_unitCellLines->draw();
  m_unitCellLines->release();
}

void Scene::drawMeasurements() {

  //  setSurfaceSpecularityAndShininess();
  if (m_measurementLines == nullptr) {
    m_measurementLines = new LineRenderer();
    m_measurementCircles = new CircleRenderer();
    m_measurementLabels = new BillboardRenderer();
  } else {
    m_measurementLines->clear();
    m_measurementCircles->clear();
    m_measurementLabels->clear();
  }
  m_measurementLines->beginUpdates();
  m_measurementCircles->beginUpdates();
  m_measurementLabels->beginUpdates();
  for (const auto &measurement : std::as_const(m_measurementList)) {
    cx::graphics::addTextToBillboardRenderer(
        *m_measurementLabels, measurement.labelPosition(), measurement.label());
    measurement.draw(m_measurementLines, m_measurementCircles);
  }
  m_measurementCircles->endUpdates();
  m_measurementLines->endUpdates();
  m_measurementLabels->endUpdates();

  m_measurementLines->bind();
  setRendererUniforms(m_measurementLines);
  m_measurementLines->draw();
  m_measurementLines->release();

  m_measurementCircles->bind();
  setRendererUniforms(m_measurementCircles);
  m_measurementCircles->draw();
  m_measurementCircles->release();

  m_measurementLabels->bind();
  setRendererUniforms(m_measurementLabels);
  m_measurementLabels->draw();
  m_measurementLabels->release();
}

void Scene::cycleDisorderHighlighting() {
  // TODO
  /*
  const auto &disorderGroups = crystal()->disorderGroups();
  if (_disorderCycleIndex == disorderGroups.size()) {
    _disorderCycleIndex = -2;
  }
  _disorderCycleIndex += 1;
  */
}

bool Scene::applyDisorderColoring() {
  return _highlightMode == HighlightMode::Normal && _disorderCycleIndex == -1;
}

void Scene::toggleFragmentColors() {
  // TODO toggle fragment colors
}

void Scene::colorFragmentsByEnergyPair() {
  // TODO color fragments by energy pair
}

void Scene::clearFragmentColors() {
  // TODO clear fragment colors
}

void Scene::togglePairHighlighting(bool show) {
  if (show) {
    _highlightMode = HighlightMode::Pair;
    colorFragmentsByEnergyPair();
    _disorderCycleIndex = 0; // Turn off disorder highlighting
  } else {
    _highlightMode = HighlightMode::Normal;
    clearFragmentColors();
  }
}

void Scene::toggleDrawHydrogenEllipsoids(bool hEllipsoids) {
  _drawHydrogenEllipsoids = hEllipsoids;
}

void Scene::setShowHydrogens(bool show) {
  _showHydrogens = show;
}

void Scene::generateAllExternalFragments() {
  // TODO generateAllExternalFragments
}

void Scene::generateInternalFragment() {
  // TODO generate internal fragment
}

void Scene::generateExternalFragment() {
  // TODO generateExternalFragment
}

bool Scene::hasAllAtomsSelected() {
  return m_structure->allAtomsHaveFlags(AtomFlag::Selected);
}

Vector3q Scene::convertToCartesian(const Vector3q &vec) const {
  auto direct = m_structure->cellVectors();
  return direct * vec;
}

void Scene::resetOrigin() { m_structure->resetOrigin(); }

void Scene::translateOrigin(const Vector3q &t) {
  m_structure->setOrigin(m_structure->origin() + t);
}

float Scene::radius() const { return m_structure->radius(); }

void Scene::resetAllAtomColors() {
  // TODO reset all atom colors
}

void Scene::bondSelectedAtoms() {
  // TODO bond selected atoms
}

void Scene::unbondSelectedAtoms() {
  // TODO unbond selected atoms
}

void Scene::suppressSelectedAtoms() {
  m_structure->setFlagForAtomsFiltered(AtomFlag::Suppressed, AtomFlag::Selected,
                                       true);
}

void Scene::unsuppressSelectedAtoms() {
  m_structure->setFlagForAtomsFiltered(AtomFlag::Suppressed, AtomFlag::Selected,
                                       false);
}

void Scene::unsuppressAllAtoms() {
  m_structure->setFlagForAllAtoms(AtomFlag::Suppressed, false);
}

void Scene::setSelectStatusForSuppressedAtoms(bool status) {
  m_structure->setFlagForAtomsFiltered(AtomFlag::Selected, AtomFlag::Suppressed,
                                       status);
}

void Scene::selectAllAtoms() {
  m_structure->setFlagForAllAtoms(AtomFlag::Selected);
}

void Scene::invertSelection() {
  for (int i = 0; i < m_structure->numberOfAtoms(); i++) {
    auto &flags = m_structure->atomFlags(i);
    flags ^= AtomFlag::Selected;
  }
}

void Scene::deleteIncompleteFragments() {
  m_structure->deleteIncompleteFragments();
}


void Scene::deleteSelectedAtoms() {
  auto idxs = m_structure->atomsWithFlags(AtomFlag::Selected);
  m_structure->deleteAtoms(idxs);
}

void Scene::completeAllFragments() {
  m_structure->completeAllFragments();
}

void Scene::colorSelectedAtoms(const QColor &color) {
  // TODO color selected atoms
}

bool Scene::hasHydrogens() const {
  return (m_structure->atomicNumbers().array() == 1).any();
}

bool Scene::hasSelectedAtoms() const {
  return m_structure->anyAtomHasFlags(AtomFlag::Selected);
}

bool Scene::hasSuppressedAtoms() const {
  return m_structure->anyAtomHasFlags(AtomFlag::Suppressed);
}

bool Scene::hasIncompleteFragments() const {
  return m_structure->hasIncompleteFragments();
}

int Scene::numberOfSelectedAtoms() const {
  return m_structure->atomIndicesWithFlags(AtomFlag::Selected).size();
}

bool Scene::hasAtomsWithCustomColor() const {
  return m_structure->anyAtomHasFlags(AtomFlag::CustomColor);
}

void Scene::deleteFragmentContainingAtomIndex(int atomIndex) {
  m_structure->deleteFragmentContainingAtomIndex(atomIndex);
}

const SelectedAtom &Scene::selectedAtom() const { return m_selectedAtom; }

std::vector<int> Scene::selectedAtomIndices() const {
  if (m_structure != nullptr) {
    return m_structure->atomIndicesWithFlags(AtomFlag::Selected);
  } else {
    return {};
  }
}

void Scene::completeFragmentContainingAtom(int atomIndex) {
  m_structure->completeFragmentContaining(atomIndex);
  emit atomSelectionChanged();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Fingerprints
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void Scene::toggleAtomsForFingerprintSelectionFilter(bool show) {
  // TODO fingerprint selection filter
  /*
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return;
  static int numberOfAddedAtoms = 0;
  Q_ASSERT(currentSurface()->isHirshfeldBased());

  if (show) { // add fragments
    int numAtomsBefore = m_deprecatedCrystal->numberOfAtoms();
    QVector<int> diAtoms = m_surfaceHandler->generateInternalFragment(
        m_deprecatedCrystal, m_surfaceHandler->currentSurfaceIndex(), false);
    QVector<int> deAtoms =
        m_surfaceHandler->generateExternalFragmentsForSurface(
            m_deprecatedCrystal, false);
    numberOfAddedAtoms = m_deprecatedCrystal->numberOfAtoms() - numAtomsBefore;

    m_deprecatedCrystal->colorAllAtoms(Qt::lightGray);
    m_deprecatedCrystal->colorAtomsByFragment(diAtoms);
    m_deprecatedCrystal->colorAtomsByFragment(deAtoms);

  } else { // reset back to how it was before
    m_deprecatedCrystal->resetAllAtomColors();
    m_deprecatedCrystal->removeLastAtoms(numberOfAddedAtoms);
  }

  */
}

QDataStream &operator<<(QDataStream &ds, const Scene &scene) {
  /*
  ds << scene.m_orientation;
  ds << scene._showHydrogens;
  ds << scene._showSuppressedAtoms;
  ds << scene._showUnitCellBox;
  ds << scene._showAtomicLabels;
  ds << scene._showFragmentLabels;
  ds << scene._showSurfaceLabels;

  ds << static_cast<int>(scene.m_drawingStyle);

  ds << scene._backgroundColor;

  ds << scene._disorderCycleIndex;
  ds << scene._energyCycleIndex;
  ds << scene._showEnergyFramework;

  ds << scene._ellipsoidProbabilityString;
  ds << scene._ellipsoidProbabilityScaleFactor;
  ds << scene._drawHydrogenEllipsoids;
  ds << scene._drawMultipleCellBoxes;

  ds << scene._savedOrientations;


  if(scene.m_deprecatedCrystal != nullptr) {
    ds << true;
    const DeprecatedCrystal &c = *scene.m_deprecatedCrystal;
    ds << c;
  }
  else {
    ds << false;
  }

  */
  return ds;
}

QDataStream &operator>>(QDataStream &ds, Scene &scene) {
  // When the drawable crystal is created, init() is called which
  // initializes all the private members. The settings::keys:: are over
  // written for all the ones read in to return the DrawableCrystal
  // to the (almost) the state it was in when it was saved.

  /*
  ds >> scene.m_orientation;
  ds >> scene._showHydrogens;
  ds >> scene._showSuppressedAtoms;
  ds >> scene._showUnitCellBox;
  ds >> scene._showAtomicLabels;
  ds >> scene._showFragmentLabels;
  ds >> scene._showSurfaceLabels;

  int drawingStyle;
  ds >> drawingStyle;
  scene.m_drawingStyle = static_cast<DrawingStyle>(drawingStyle);

  ds >> scene._backgroundColor;

  ds >> scene._disorderCycleIndex;
  ds >> scene._energyCycleIndex;
  ds >> scene._showEnergyFramework;

  ds >> scene._ellipsoidProbabilityString;
  ds >> scene._ellipsoidProbabilityScaleFactor;
  ds >> scene._drawHydrogenEllipsoids;
  ds >> scene._drawMultipleCellBoxes;

  ds >> scene._savedOrientations;

  bool haveDeprecatedCrystal{false};
  ds >> haveDeprecatedCrystal;
  if(haveDeprecatedCrystal) {
      scene.m_deprecatedCrystal = new DeprecatedCrystal();
      DeprecatedCrystal &c = *scene.m_deprecatedCrystal;
      ds >> c;
  }
  */
  return ds;
}
