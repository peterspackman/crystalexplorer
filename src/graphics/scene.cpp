#include <QDebug>
#include <QtOpenGL>

#include "crystalplanegenerator.h"
#include "drawingstyle.h"
#include "elementdata.h"
#include "fmt/core.h"
#include "globals.h"
#include "graphics.h"
#include "mathconstants.h"
#include "scene.h"
#include "settings.h"
#include <ankerl/unordered_dense.h>

using cx::graphics::SelectionType;

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
  textSettingsChanged();
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
  m_showSuppressedAtoms = true;
  m_showHydrogenBonds = false;
  m_closeContactCriteria.clear();
}

void Scene::setShowCloseContacts(bool set) {
  ContactSettings settings;
  settings.show = set;
  m_structure->setShowContacts(settings);
}

void Scene::setFrameworkOptions(const FrameworkOptions &options) {
  if (m_structureRenderer) {
    m_structureRenderer->setFrameworkOptions(options);
  }
}

AtomLabelOptions Scene::atomLabelOptions() const {
  if (m_structureRenderer) {
    return m_structureRenderer->atomLabelOptions();
  }
  return {};
}

void Scene::setAtomLabelOptions(const AtomLabelOptions &options) {
  if (m_structureRenderer) {
    m_structureRenderer->setAtomLabelOptions(options);
  }
}

void Scene::toggleShowAtomLabels() {
  if (m_structureRenderer) {
    m_structureRenderer->toggleShowAtomLabels();
  }
}

void Scene::setSelectStatusForAllAtoms(bool set) {
  m_structure->setFlagForAllAtoms(AtomFlag::Selected, set);
  emit atomSelectionChanged();
}

void Scene::addMeasurement(const Measurement &m) {
  if (m_measurementRenderer)
    m_measurementRenderer->add(m);
}

void Scene::removeLastMeasurement() {
  if (m_measurementRenderer)
    m_measurementRenderer->removeLastMeasurement();
}

void Scene::removeAllMeasurements() {
  if (m_measurementRenderer)
    m_measurementRenderer->clear();
}

bool Scene::hasMeasurements() const {
  if (!m_measurementRenderer)
    return false;
  return m_measurementRenderer->hasMeasurements();
}

AtomDrawingStyle Scene::atomStyle() const {
  if (m_structureRenderer)
    return m_structureRenderer->atomStyle();
  return atomStyleForDrawingStyle(m_drawingStyle);
}
BondDrawingStyle Scene::bondStyle() const {
  if (m_structureRenderer)
    return m_structureRenderer->bondStyle();
  return bondStyleForDrawingStyle(m_drawingStyle);
}

void Scene::setDrawingStyle(DrawingStyle style) {
  m_drawingStyle = style;
  if (m_structureRenderer) {
    m_structureRenderer->setDrawingStyle(style);
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

void Scene::generateSlab(SlabGenerationOptions options) {
  m_structure->buildSlab(options);
}

void Scene::updateNoneProperties() {
  // TODO update none properties
  // surfaceHandler()->updateAllSurfaceNoneProperties();
  if (m_structureRenderer) {
    m_structureRenderer->updateMeshes();
  }
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

  return m_showHydrogenBonds ||
         std::any_of(m_closeContactCriteria.begin(),
                     m_closeContactCriteria.end(),
                     [](const auto &criteria) { return criteria.show; });
}

void Scene::setSelectStatusForAtomDoubleClick(int atom) {
  auto atomIndex = m_structure->indexToGenericIndex(atom);
  if (m_structure->testAtomFlag(atomIndex, AtomFlag::Contact))
    return;
  const auto fragmentIndex =
      m_structure->fragmentIndexForGeneralAtom(atomIndex);
  const auto &atomIndices = m_structure->atomIndicesForFragment(fragmentIndex);
  auto idx = m_structure->indexToGenericIndex(atom);
  m_structure->setAtomFlag(idx, AtomFlag::Selected, true);
  bool selected = std::all_of(
      atomIndices.begin(), atomIndices.end(), [&](GenericAtomIndex x) {
        return m_structure->atomFlagsSet(x, AtomFlag::Selected);
      });
  m_structure->setFlagForAtoms(atomIndices, AtomFlag::Selected, !selected);
}

void Scene::selectAtomsSeparatedBySurface(bool inside) {
  if (m_selectedSurface.surface) {
    m_structure->setFlagForAllAtoms(AtomFlag::Selected, !inside);
    auto atomIndices = m_selectedSurface.surface->atomsInside();
    for (const auto &idx : atomIndices) {
      m_structure->setAtomFlag(idx, AtomFlag::Selected, inside);
    }
  }
}

bool Scene::processSelectionDoubleClick(const QColor &color) {
  m_selection = m_selectionHandler->getSelectionFromColor(color);

  switch (m_selection.type) {
  case SelectionType::Atom: {
    setSelectStatusForAtomDoubleClick(m_selection.index);
    emit atomSelectionChanged();
    return true;
  }
  case SelectionType::Bond: {
    int bondIndex = m_selection.index;
    int atomIndex = m_structure->atomsForBond(bondIndex).first;
    setSelectStatusForAtomDoubleClick(atomIndex);
    emit atomSelectionChanged();
    return true;
  }
  default:
    break;
  }
  return false;
}

void Scene::handleSurfacesNeedUpdate() {
  if (m_structureRenderer)
    m_structureRenderer->updateMeshes();
}

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

  switch (m_selection.type) {
  case SelectionType::Atom: {
    const auto atomIndex = m_structure->indexToGenericIndex(m_selection.index);
    // TODO handle contact atom
    if (m_structure->testAtomFlag(atomIndex, AtomFlag::Contact)) {
      m_structure->completeFragmentContaining(atomIndex);
      emit contactAtomExpanded();
    } else {
      m_structure->toggleAtomFlag(atomIndex, AtomFlag::Selected);
      emit atomSelectionChanged();
    }
    return true;
    break;
  }
  case SelectionType::Bond: {
    const auto [a, b] = m_structure->atomIndicesForBond(m_selection.index);
    auto flagsA = m_structure->atomFlags(a);
    auto flagsB = m_structure->atomFlags(b);
    if ((flagsA & AtomFlag::Selected) != (flagsB & AtomFlag::Selected)) {
      flagsA |= AtomFlag::Selected;
      flagsB |= AtomFlag::Selected;
    } else {
      // toggle
      flagsA ^= AtomFlag::Selected;
      flagsB ^= AtomFlag::Selected;
    }
    m_structure->setAtomFlags(a, flagsA);
    m_structure->setAtomFlags(b, flagsB);
    emit atomSelectionChanged();
    return true;
    break;
  }
  case SelectionType::Surface: {
    size_t surfaceIndex = m_selection.index;
    MeshInstance *meshInstance =
        m_structureRenderer->getMeshInstance(m_selection.index);

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
  case SelectionType::Aggregate: {
    auto agg = m_structureRenderer->getAggregateIndex(m_selection.index);
    const auto &fragments = m_structure->getFragments();
    const auto kv = fragments.find(agg.fragment);
    if (kv == fragments.end())
      break;
    const auto &frag = kv->second;
    for (const auto &atom : frag.atomIndices) {
      m_structure->toggleAtomFlag(atom, AtomFlag::Selected);
    }
    emit atomSelectionChanged();
    return true;
    break;
  }
  default: {
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

MeasurementObject Scene::processMeasurementSingleClick(const QColor &color,
                                                       bool wholeObject) {
  MeasurementObject result;
  result.wholeObject = wholeObject;
  m_selection = m_selectionHandler->getSelectionFromColor(color);

  switch (m_selection.type) {
  case SelectionType::Atom: {
    const auto atomIndex = m_structure->indexToGenericIndex(m_selection.index);
    if (m_structure->atomFlagsSet(atomIndex, AtomFlag::Contact))
      break;
    if (wholeObject)
      m_structure->selectFragmentContaining(atomIndex);
    else {
      auto flags = m_structure->atomFlags(atomIndex);
      flags ^= AtomFlag::Selected;
      m_structure->setAtomFlags(atomIndex, flags);
    }
    emit atomSelectionChanged();
    occ::Vec3 pos = m_structure->atomPosition(atomIndex);
    result.position = QVector3D(pos.x(), pos.y(), pos.z());
    result.selectionType = SelectionType::Atom;
    result.index = m_selection.index;
    break;
  }
  case SelectionType::Bond: {
    size_t bond_idx = m_selection.index;
    auto [a, b] = m_structure->atomIndicesForBond(bond_idx);
    auto flagsA = m_structure->atomFlags(a);
    auto flagsB = m_structure->atomFlags(b);
    if ((flagsA & AtomFlag::Contact) && (flagsB & AtomFlag::Contact))
      break;
    if (wholeObject)
      m_structure->selectFragmentContaining(a);
    else {
      flagsA ^= AtomFlag::Selected;
      flagsB ^= AtomFlag::Selected;
      m_structure->setAtomFlags(a, flagsA);
      m_structure->setAtomFlags(b, flagsB);
    }
    occ::Vec3 pos =
        0.5 * (m_structure->atomPosition(a) + m_structure->atomPosition(b));
    result.position = QVector3D(pos.x(), pos.y(), pos.z());
    result.selectionType = SelectionType::Bond;
    result.index = bond_idx;
    break;
  }
  case SelectionType::Surface: {
    size_t surfaceIndex = m_selection.index;
    auto *meshInstance = m_structureRenderer->getMeshInstance(surfaceIndex);
    if (!meshInstance)
      break;
    const auto pos = meshInstance->vertex(m_selection.secondaryIndex);
    result.position = QVector3D(pos.x(), pos.y(), pos.z());
    result.selectionType = SelectionType::Surface;
    result.index = surfaceIndex;

    break;
  }
  case SelectionType::Aggregate: {
    auto agg = m_structureRenderer->getAggregateIndex(m_selection.index);
    const auto &fragments = m_structure->getFragments();
    const auto kv = fragments.find(agg.fragment);
    if (kv == fragments.end())
      break;
    const auto &frag = kv->second;
    result.position = agg.position;
    result.selectionType = SelectionType::Aggregate;
    result.index = m_selection.index;
    for (const auto &atom : frag.atomIndices) {
      m_structure->toggleAtomFlag(atom, AtomFlag::Selected);
    }
    emit atomSelectionChanged();
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
  auto *surface = m_structureRenderer->getMeshInstance(m_selection.index);
  m_selectedSurface.surface = surface;

  if (surface) {
    m_selectedSurface.property = surface->getSelectedProperty();
    m_selectedSurface.propertyValue =
        surface->valueForSelectedPropertyAt(m_selection.secondaryIndex);
  }
}

void Scene::populateSelectedAtom() {
  m_selectedAtom.index = m_selection.index;
  m_selectedAtom.atomicNumber = m_structure->atomicNumbers()(m_selection.index);
  m_selectedAtom.label = m_structure->labels()[m_selection.index];
  const auto pos = m_structure->atomicPositions().col(m_selection.index);
  m_selectedAtom.position = QVector3D(pos.x(), pos.y(), pos.z());
  m_selectedAtom.fragmentLabel = "Not set";
  auto maybeFragment = m_structure->getFragmentForAtom(m_selection.index);
  if (maybeFragment) {
    const auto &fragment = maybeFragment->get();
    m_selectedAtom.fragmentLabel =
        m_structure->getFragmentLabel(fragment.asymmetricFragmentIndex);
  }
}

void Scene::populateSelectedBond() {
  m_selectedBond.index = m_selection.index;
  const auto [idx_a, idx_b] = m_structure->atomsForBond(m_selection.index);
  auto maybeFragment = m_structure->getFragmentForAtom(idx_a);
  if (maybeFragment) {
    const auto &fragment = maybeFragment->get();
    m_selectedBond.fragmentLabel =
        m_structure->getFragmentLabel(fragment.asymmetricFragmentIndex);
  }
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

const SelectedSurface &Scene::selectedSurface() const {
  return m_selectedSurface;
}

void Scene::updateForPreferencesChange() {
  if (m_structure) {
    updateNoneProperties();
    setNeedsUpdate();
  }
}

void Scene::setNeedsUpdate() {
  m_labelsNeedUpdate = true;
  if (m_structureRenderer) {
    m_structureRenderer->forceUpdates();
  }
}

QStringList Scene::uniqueElementSymbols() const {
  if (m_structure) {
    return m_structure->uniqueElementSymbols();
  }
  return {};
}

DistanceMeasurementPoints
Scene::positionsForDistanceMeasurement(const MeasurementObject &object,
                                       const QVector3D &pos) const {

  DistanceMeasurementPoints result;
  result.a = object.position;
  result.b = pos;
  if (!m_structureRenderer)
    return result;

  if (!object.wholeObject) {
    result.valid = true;
    return result;
  }

  switch (object.selectionType) {
  case SelectionType::Surface: {
    auto *meshInstance = m_structureRenderer->getMeshInstance(object.index);
    if (!meshInstance)
      break;

    auto res =
        meshInstance->nearestPoint(Eigen::Vector3d(pos.x(), pos.y(), pos.z()));
    result.a = meshInstance->vertexVector3D(res.idx_this);
    result.valid = true;
    break;
  }
  default: {
    const auto &fragments = m_structure->getFragments();
    const auto fragIndex = m_structure->fragmentIndexForAtom(object.index);

    if (fragIndex.isValid()) {
      const auto frag = fragments.at(fragIndex);
      auto res =
          frag.nearestAtomToPoint(Eigen::Vector3d(pos.x(), pos.y(), pos.z()));
      result.a = frag.posVector3D(res.idx_this);
      result.valid = true;
      break;
    }
  }
  }
  return result;
}

DistanceMeasurementPoints
Scene::positionsForDistanceMeasurement(const MeasurementObject &object1,
                                       const MeasurementObject &object2) const {
  // For convenience, and to make code slightly more readable ...

  // if one object is just a point
  if (!object1.wholeObject) {
    return positionsForDistanceMeasurement(object2, object1.position);
  }
  if (!object2.wholeObject) {
    return positionsForDistanceMeasurement(object1, object2.position);
  }

  DistanceMeasurementPoints result;
  result.a = object1.position;
  result.b = object2.position;

  switch (object1.selectionType) {
  case SelectionType::Surface: {
    switch (object2.selectionType) {
    case SelectionType::Surface: {
      auto *meshInstanceA = m_structureRenderer->getMeshInstance(object1.index);
      if (!meshInstanceA)
        break;
      auto *meshInstanceB = m_structureRenderer->getMeshInstance(object2.index);
      if (!meshInstanceB)
        break;

      auto res = meshInstanceA->nearestPoint(meshInstanceB);
      result.a = meshInstanceA->vertexVector3D(res.idx_this);
      result.b = meshInstanceB->vertexVector3D(res.idx_other);
      result.valid = true;

      break;
    }
    default: {
      auto *meshInstance = m_structureRenderer->getMeshInstance(object1.index);
      if (!meshInstance)
        break;

      const auto &fragments = m_structure->getFragments();
      const auto fragIndex = m_structure->fragmentIndexForAtom(object2.index);
      const bool validIndex = fragIndex.isValid();
      if (!validIndex)
        break;

      const auto &frag = fragments.at(fragIndex);
      auto res = meshInstance->nearestPoint(frag);
      result.a = meshInstance->vertexVector3D(res.idx_this);
      result.b = frag.posVector3D(res.idx_other);
      result.valid = true;
      break;
    }
    }
    break;
  }
  default: {
    switch (object2.selectionType) {
    case SelectionType::Surface: {
      auto *meshInstance = m_structureRenderer->getMeshInstance(object2.index);
      if (!meshInstance)
        break;

      const auto &fragments = m_structure->getFragments();
      const auto fragIndex = m_structure->fragmentIndexForAtom(object1.index);
      const bool validIndex = fragIndex.isValid();
      if (!validIndex)
        break;

      const auto &frag = fragments.at(fragIndex);
      auto res = meshInstance->nearestPoint(frag);
      result.b = meshInstance->vertexVector3D(res.idx_this);
      result.a = frag.posVector3D(res.idx_other);
      result.valid = true;
      break;
    }
    default: {
      const auto &fragments = m_structure->getFragments();
      const auto fragIndexA = m_structure->fragmentIndexForAtom(object1.index);
      const auto fragIndexB = m_structure->fragmentIndexForAtom(object2.index);

      const bool validIndexA = fragIndexA.isValid();
      const bool validIndexB = fragIndexA.isValid();

      if (validIndexA && validIndexB && (fragIndexA != fragIndexB)) {
        const auto fragA = fragments.at(fragIndexA);
        const auto fragB = fragments.at(fragIndexB);
        auto res = fragA.nearestAtom(fragB);
        result.a = fragA.posVector3D(res.idx_this);
        result.b = fragB.posVector3D(res.idx_other);
        result.valid = true;
      }
      break;
    }
    }
    break;
  }
  }
  return result;
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

  if (m_measurementRenderer) {
    m_measurementRenderer->updateRendererUniforms(m_uniforms);
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

void Scene::updateThermalEllipsoidProbability(double p) {
  if (!m_structureRenderer)
    return;
  qDebug() << "Setting probability to " << p;
  m_structureRenderer->updateThermalEllipsoidProbability(p);
}

void Scene::ensureRenderersInitialized() {
  if (m_structure && !m_structureRenderer) {
    m_structureRenderer =
        new cx::graphics::ChemicalStructureRenderer(m_structure, this);
    m_structureRenderer->setSelectionHandler(m_selectionHandler);
    m_structureRenderer->setDrawingStyle(drawingStyle());
    connect(m_structureRenderer,
            &cx::graphics::ChemicalStructureRenderer::meshesChanged, this,
            &Scene::sceneContentsChanged);
  }

  if (!m_measurementRenderer) {
    m_measurementRenderer = new cx::graphics::MeasurementRenderer(this);
  }

  if (!m_hydrogenBondLines) {
    m_hydrogenBondLines = new LineRenderer();
  }

  if (!m_closeContactLines) {
    m_closeContactLines = new LineRenderer();
  }

  if (!m_lightPositionRenderer) {
    m_lightPositionRenderer = new EllipsoidRenderer();
  }
  if (!m_crystalPlaneRenderer) {
    m_crystalPlaneRenderer = new CrystalPlaneRenderer();
  }
}

void Scene::drawChemicalStructure() {
  if (m_structureRenderer) {
    m_structureRenderer->draw();
  }
}

void Scene::drawExtras() {
  if (hasVisibleAtoms()) {
    drawHydrogenBonds();
    drawCloseContacts();
    drawMeasurements();
  }

  drawLights();
  drawPlanes();
}

void Scene::drawPlanes() {
  updateCrystalPlanes();

  if (m_crystalPlaneRenderer->size() > 0) {
    m_crystalPlaneRenderer->bind();
    setRendererUniforms(m_crystalPlaneRenderer, false);
    m_crystalPlaneRenderer->draw();
    m_crystalPlaneRenderer->release();
  }
}

void Scene::draw() {
  ensureRenderersInitialized();
  updateRendererUniforms();
  drawChemicalStructure();
  drawExtras();
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
  m_uniforms.u_textSize =
      settings::readSetting(settings::keys::TEXT_FONT_SIZE).toFloat() * 0.25;
  setNeedsUpdate();
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
  if (!m_drawLights)
    return;

  m_lightPositionRenderer->beginUpdates();
  m_lightPositionRenderer->clear();
  for (int i = 0; i < m_uniforms.u_numLights; i++) {
    cx::graphics::addSphereToEllipsoidRenderer(
        m_lightPositionRenderer, m_uniforms.u_lightPos.column(i).toVector3D(),
        Qt::yellow, 1.0);
  }
  m_lightPositionRenderer->endUpdates();
  m_lightPositionRenderer->bind();
  setRendererUniforms(m_lightPositionRenderer, false);
  m_lightPositionRenderer->draw();
  m_lightPositionRenderer->release();
}

double Scene::getThermalEllipsoidProbability() const {
  if (!m_structureRenderer)
    return 0.0;
  double result = m_structureRenderer->getThermalEllipsoidProbability();
  qDebug() << "Current probability = " << result;
  return result;
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
  m_hydrogenBondsNeedUpdate = true;
}

void Scene::drawHydrogenBonds() {
  if (!m_showHydrogenBonds)
    return;

  if (m_hydrogenBondsNeedUpdate) {
    m_hydrogenBondLines->clear();
    double radius = contactLineThickness();
    m_hydrogenBondLines->beginUpdates();

    const auto &bonds = m_structure->hydrogenBonds(m_hbondCriteria);
    const auto &positions = m_structure->atomicPositions();
    for (const auto &[d, h, a] : bonds) {
      const auto fragD = m_structure->fragmentIndexForAtom(d);
      const auto fragA = m_structure->fragmentIndexForAtom(a);
      // skip intramolecular contacts
      if (!m_hbondCriteria.includeIntra && (fragD == fragA)) {
        continue;
      }
      QVector3D pos_h(positions(0, h), positions(1, h), positions(2, h));
      QVector3D pos_a(positions(0, a), positions(1, a), positions(2, a));
      cx::graphics::addDashedLineToLineRenderer(
          *m_hydrogenBondLines, pos_h, pos_a, radius, m_hbondCriteria.color);
    }
    m_hydrogenBondLines->endUpdates();
    m_hydrogenBondsNeedUpdate = false;
  }
  m_hydrogenBondLines->bind();
  setRendererUniforms(m_hydrogenBondLines);
  m_hydrogenBondLines->draw();
  m_hydrogenBondLines->release();
}

void Scene::updateCloseContactsCriteria(int index,
                                        CloseContactCriteria criteria) {
  m_closeContactCriteria[index] = criteria;
  m_closeContactsNeedUpdate = true;
}

void Scene::drawCloseContacts() {
  if (m_closeContactsNeedUpdate) {
    m_closeContactLines->clear();
    double radius = contactLineThickness();
    m_closeContactLines->beginUpdates();

    const auto &positions = m_structure->atomicPositions();
    // Using C++17 structured bindings
    for (const auto &[cc_id, criteria] :
         m_closeContactCriteria.asKeyValueRange()) {
      if (!criteria.show)
        continue;
      for (const auto &[a, b] : m_structure->closeContacts(criteria)) {
        // skip intramolecular contacts
        if (m_structure->fragmentIndexForAtom(a) ==
            m_structure->fragmentIndexForAtom(b))
          continue;
        QVector3D pos_a(positions(0, a), positions(1, a), positions(2, a));
        QVector3D pos_b(positions(0, b), positions(1, b), positions(2, b));
        cx::graphics::addDashedLineToLineRenderer(
            *m_closeContactLines, pos_a, pos_b, radius, criteria.color);
      }
    }
    m_closeContactLines->endUpdates();
    m_closeContactsNeedUpdate = false;
  }
  m_closeContactLines->bind();
  setRendererUniforms(m_closeContactLines);
  m_closeContactLines->draw();
  m_closeContactLines->release();
}

void Scene::setShowSuppressedAtoms(bool show) {
  if (!show) {
    setSelectStatusForSuppressedAtoms(false);
  }
  // TODO propagate to chemical structure renderer
  m_showSuppressedAtoms = show;
}

void Scene::expandAtomsWithinRadius(float radius, bool selection) {
  m_structure->expandAtomsWithinRadius(radius, selection);
}

void Scene::selectAtomsOutsideRadiusOfSelectedAtoms(float radius) {
  // TODO
}

void Scene::reset() {
  m_structure->resetAtomsAndBonds();
  m_structure->resetAtomColorOverrides();
  clearFragmentColors();
  resetViewAndSelections();
}

void Scene::updateCrystalPlanes() {
  if (!m_crystalPlanesNeedUpdate)
    return;
  m_crystalPlaneRenderer->clear();
  m_crystalPlaneRenderer->beginUpdates();
  for (const auto &plane : std::as_const(m_crystalPlanes)) {
    if (plane.hkl.h == 0 && plane.hkl.k == 0 && plane.hkl.l == 0)
      continue;
    CrystalPlaneGenerator generator(m_structure, plane.hkl);
    const auto &aVector = generator.aVector();
    const auto &bVector = generator.bVector();
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

bool Scene::showCells() {
  if (m_structureRenderer) {
    return m_structureRenderer->showCells();
  }
  return false;
}

void Scene::setShowCells(bool show) {
  if (m_structureRenderer) {
    return m_structureRenderer->setShowCells(show);
  }
}

bool Scene::showMultipleCells() {
  if (m_structureRenderer) {
    return m_structureRenderer->showMultipleCells();
  }
  return false;
}

void Scene::setShowMultipleCells(bool show) {
  if (m_structureRenderer) {
    return m_structureRenderer->setShowMultipleCells(show);
  }
}

void Scene::drawMeasurements() {
  if (!hasMeasurements())
    return;
  m_measurementRenderer->draw();
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

void Scene::colorFragmentsByEnergyPair(FragmentPairSettings pairSettings) {
  auto selectedFragments = m_structure->selectedFragments();
  auto *interactions = m_structure->pairInteractions();
  interactions->resetCounts();
  interactions->resetColors();

  if (selectedFragments.size() == 1) {
    m_structure->setAllFragmentColors(FragmentColorSettings{
        FragmentColorSettings::Method::Constant, Qt::gray});
    pairSettings.keyFragment = selectedFragments[0];
    auto fragmentPairs = m_structure->findFragmentPairs(pairSettings);
    ColorMapFunc colorMap(ColorMapName::Austria, 0,
                          fragmentPairs.uniquePairs.size() - 1);
    std::vector<int> counts(fragmentPairs.uniquePairs.size(), 0);
    for (const auto &[fragmentPair, idx] :
         fragmentPairs.pairs[selectedFragments[0]]) {
      qDebug() << "Setting fragment color" << fragmentPair.index.b
               << colorMap(idx);
      QColor c = colorMap(idx);
      counts[idx]++;
      m_structure->setFragmentColor(fragmentPair.index.b, c);
    }
    auto interactionMap = interactions->getInteractionsMatchingFragments(
        fragmentPairs.uniquePairs);
    for (auto interactionList : interactionMap) {
      for (int i = 0; i < interactionList.size(); i++) {
        if (interactionList[i]) {
          interactionList[i]->setColor(colorMap(i));
          interactionList[i]->setCount(counts[i]);
        }
      }
    }
  } else {
    m_structure->setAllFragmentColors(m_fragmentColorSettings);
  }
  m_structure->setAtomColoring(ChemicalStructure::AtomColoring::Fragment);
}

void Scene::clearFragmentColors() {
  m_structure->setAllFragmentColors(m_fragmentColorSettings);
  m_structure->setAtomColoring(ChemicalStructure::AtomColoring::Element);
}

void Scene::togglePairHighlighting(bool show) {
  if (!m_structure)
    return;

  if (show) {
    _highlightMode = HighlightMode::Pair;
    // TODO fix this to be more robust
    auto *interactions = m_structure->pairInteractions();
    FragmentPairSettings settings;
    settings.allowInversion = interactions->hasPermutationSymmetry();
    colorFragmentsByEnergyPair(settings);
    _disorderCycleIndex = 0; // Turn off disorder highlighting
  } else {
    _highlightMode = HighlightMode::Normal;
    clearFragmentColors();
  }
}

void Scene::toggleDrawHydrogenEllipsoids(bool hEllipsoids) {
  _drawHydrogenEllipsoids = hEllipsoids;
}

bool Scene::showHydrogenAtoms() const {
  if (m_structureRenderer) {
    return m_structureRenderer->showHydrogenAtoms();
  }
  // default to true for this case
  return true;
}

void Scene::setShowHydrogenAtoms(bool show) {
  if (m_structureRenderer) {
    m_structureRenderer->setShowHydrogenAtoms(show);
  }
}

void Scene::toggleShowHydrogenAtoms() {
  if (m_structureRenderer) {
    m_structureRenderer->toggleShowHydrogenAtoms();
  }
}

void Scene::generateAllExternalFragments() {
  if (m_selectedSurface.surface) {
    auto *mesh = m_selectedSurface.surface->mesh();
    qDebug() << "Generate external fragment";
    if (!mesh)
      return;
    occ::IVec de_idxs = mesh->vertexProperty("External atom index").cast<int>();
    qDebug() << "num de_idxs" << de_idxs.size();
    ankerl::unordered_dense::set<int> unique(de_idxs.data(),
                                             de_idxs.data() + de_idxs.size());
    auto atomIndices = m_selectedSurface.surface->atomsOutside();
    for (const auto &i : unique) {
      if (i >= atomIndices.size())
        continue;
      const auto &idx = atomIndices[i];
      m_structure->completeFragmentContaining(idx);
    }
  }
}

void Scene::generateInternalFragment() {
  if (m_selectedSurface.surface) {
    auto atomIndices = m_selectedSurface.surface->atomsInside();
    for (const auto &idx : atomIndices) {
      m_structure->completeFragmentContaining(idx);
    }
  }
}

void Scene::generateExternalFragment() {
  if (m_selectedSurface.surface) {
    auto *mesh = m_selectedSurface.surface->mesh();
    qDebug() << "Generate external fragment";
    if (!mesh)
      return;
    const auto &de_idxs = mesh->vertexProperty("de_idx");
    qDebug() << "num de_idxs" << de_idxs.size();
    if ((de_idxs.size() < m_selection.secondaryIndex) ||
        (m_selection.secondaryIndex < 0))
      return;
    const int de_idx = static_cast<int>(de_idxs(m_selection.secondaryIndex));
    auto atomIndices = m_selectedSurface.surface->atomsOutside();
    qDebug() << "de_idx" << de_idx << atomIndices.size();
    if (de_idx >= atomIndices.size())
      return;
    qDebug() << "de_idx valid";
    const auto &idx = atomIndices[de_idx];
    m_structure->completeFragmentContaining(idx);
  }
}

bool Scene::hasAllAtomsSelected() {
  return m_structure->allAtomsHaveFlags(AtomFlag::Selected);
}

occ::Vec3 Scene::convertToCartesian(const occ::Vec3 &vec) const {
  auto direct = m_structure->cellVectors();
  return direct * vec;
}

void Scene::resetOrigin() { m_structure->resetOrigin(); }

void Scene::translateOrigin(const occ::Vec3 &t) {
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
  m_structure->toggleFlagForAllAtoms(AtomFlag::Selected);
}

void Scene::deleteIncompleteFragments() {
  m_structure->deleteIncompleteFragments();
}

void Scene::filterAtoms(AtomFlag flag, bool state) {
  auto idxs = m_structure->atomsWithFlags(flag, state);
  m_structure->deleteAtoms(idxs);
}

void Scene::completeAllFragments() { m_structure->completeAllFragments(); }

void Scene::colorSelectedAtoms(const QColor &color, bool fragments) {
  auto idxs = m_structure->atomsWithFlags(AtomFlag::Selected);
  if (fragments) {
    for (const auto &idx : idxs) {
      const auto frag = m_structure->fragmentIndexForGeneralAtom(idx);
      m_structure->setFragmentColor(frag, color);
    }
  } else {
    AtomFlags flags = AtomFlag::Selected;
    m_structure->setColorForAtomsWithFlags(flags, color);
  }
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
  return m_structure->atomsWithFlags(AtomFlag::Selected).size();
}

bool Scene::hasAtomsWithCustomColor() const {
  return m_structure->anyAtomHasFlags(AtomFlag::CustomColor);
}

void Scene::deleteFragmentContainingAtomIndex(int atomIndex) {
  m_structure->deleteFragmentContainingAtomIndex(atomIndex);
}

const SelectedAtom &Scene::selectedAtom() const { return m_selectedAtom; }

void Scene::completeFragmentContainingAtom(int atomIndex) {
  m_structure->completeFragmentContaining(atomIndex);
  emit atomSelectionChanged();
}

nlohmann::json Scene::toJson() const {
  return {
      {"title", m_name},
      {"structure", m_structure->toJson()},
      {"orientation", m_orientation},
  };
}

bool Scene::fromJson(const nlohmann::json &j) {
  if (!j.contains("structure"))
    return false;
  if (!j.contains("title"))
    return false;
  if (!j.contains("orientation"))
    return false;

  // TODO handle crystal structure
  auto structure = new ChemicalStructure();
  if (!structure->fromJson(j.at("structure")))
    return false;
  m_structure = structure;
  j.at("orientation").get_to(m_orientation);
  j.at("title").get_to(m_name);
  setNeedsUpdate();
  return true;
}
