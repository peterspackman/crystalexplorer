#include <QDebug>
#include <QtOpenGL>

#include "crystalplanegenerator.h"
#include "elementdata.h"
#include "fmt/core.h"
#include "graphics.h"
#include "mathconstants.h"
#include "scene.h"
#include "settings.h"
#include "xyzfile.h"
#include "wavefrontobjfile.h"
#include "interactions.h"

using cx::graphics::SelectionType;

Scene::Scene(const XYZFile &xyz)
    : m_deprecatedCrystal(nullptr),
      m_surfaceHandler(nullptr),
      m_structure(new ChemicalStructure()) {

  m_periodicity = ScenePeriodicity::ZeroDimensions;
  m_structure->setAtoms(xyz.getAtomSymbols(), xyz.getAtomPositions());
  m_structure->updateBondGraph();
  //WavefrontObjectFile obj("surface.obj");
  //Mesh * mesh = obj.getFirstMesh(m_structure);
  auto * interactions = new DimerInteractions(m_structure);
  interactions->setValue(DimerPair(0, 1), 10.0, "coulomb");
  init();
}

Scene::Scene(ChemicalStructure *structure)
    : m_deprecatedCrystal(nullptr),
      m_surfaceHandler(nullptr),
      m_structure(structure) {
  m_periodicity = ScenePeriodicity::ZeroDimensions;
  init();
}

Scene::Scene()
    : m_deprecatedCrystal(new DeprecatedCrystal()),
      m_surfaceHandler(new CrystalSurfaceHandler()) {
  init();
}

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
  _energyCycleIndex = minCycleIndex();

  _ellipsoidProbabilityScaleFactor =
      Atom::thermalEllipsoidProbabilityScaleFactors[1];
  _drawHydrogenEllipsoids = true;

  _drawMultipleCellBoxes = false;

  _showEnergyFramework = false;
  screenGammaChanged();
  materialChanged();
  lightSettingsChanged();
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    QObject::connect(crystal(), &DeprecatedCrystal::atomsChanged, this,
                     &Scene::handleAtomsNeedUpdate);
    QObject::connect(surfaceHandler(), &CrystalSurfaceHandler::surfacesChanged,
                     this, &Scene::handleSurfacesNeedUpdate);
    m_atomsNeedUpdate = true;
  }

  m_selectionHandler = new cx::graphics::RenderSelection(this);
}

void Scene::setSurfaceLightingToDefaults() {
  m_uniforms.u_materialMetallic =
      settings::readSetting(settings::keys::MATERIAL_METALLIC).toFloat();
  m_uniforms.u_materialRoughness =
      settings::readSetting(settings::keys::MATERIAL_ROUGHNESS).toFloat();
  m_uniforms.u_renderMode = settings::readSetting(settings::keys::MATERIAL).toInt();
}

void Scene::setViewAngleAndScaleToDefaults() { m_orientation = Orientation(); }

void Scene::setShowStatusesToDefaults() {
  _showHydrogens = true;
  _showSuppressedAtoms = true;
  _showUnitCellBox = false;
  _showAtomicLabels = false;
  _showFragmentLabels = false;
  _showSurfaceLabels = false;
  _showHydrogenBonds = false;
  _showCloseContacts.clear();
  _showCloseContacts = {false, false, false};
}

void Scene::setShowCloseContacts(bool set) {
  if (m_deprecatedCrystal != nullptr) {
    m_deprecatedCrystal->showVdwContactAtoms(set);
    m_atomsNeedUpdate = true;
  } else {
    m_structure->setShowVanDerWaalsContactAtoms(set);
    m_atomsNeedUpdate = true;
  }
}

void Scene::setSelectStatusForAllAtoms(bool set) {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    crystal()->setSelectStatusForAllAtoms(set);
  } else {
    m_structure->setFlagForAllAtoms(AtomFlag::Selected, set);
    m_atomsNeedUpdate = true;
  }
}

void Scene::addMeasurement(const Measurement &m) {
  m_measurementList.append(m);
  m_measurementList.back().setColor(ColorSchemer::color(
      ColorScheme::MaterialDesign, m_measurementList.size() - 1, 0,
      MATERIAL_DESIGN_SIZE));
}

void Scene::removeLastMeasurement() {
  if (m_measurementList.size() > 0)
    m_measurementList.removeLast();
}

void Scene::removeAllMeasurements() { m_measurementList.clear(); }

bool Scene::hasMeasurements() const { return !m_measurementList.isEmpty(); }

void Scene::setSelectionColor(const QColor &color) { m_selectionColor = color; }

void Scene::setDrawingStyle(DrawingStyle style) {
  m_drawingStyle = style;
  if(m_structureRenderer) {
      m_structureRenderer->setAtomStyle(atomStyle());
      m_structureRenderer->setBondStyle(bondStyle());
  }
  m_atomsNeedUpdate = true;
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
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    return crystal()->anyAtomHasAdp();
  }
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
    labels.append(fragmentLabels());
  }
  if (_showSurfaceLabels) {
    labels.append(surfaceLabels());
  }
  if (hasMeasurements()) {
    labels.append(measurementLabels());
  }
  if (_showEnergyFramework) {
    auto annotateFrameworkFlags = getAnnotateFrameworkFlags();
    if (annotateFrameworkFlags[currentFramework()]) {
      labels.append(energyLabels());
    }
  }

  return labels;
}

QVector<Label> Scene::atomicLabels() {
  QVector<Label> labels;
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    for (const auto &atom : std::as_const(crystal()->atoms())) {
      if (!atom.isContactAtom() && atom.isVisible()) {
        labels.append({atom.description(), atom.pos()});
      }
    }
  } else {
    const auto &atomLabels = m_structure->labels();
    const auto &positions = m_structure->atomicPositions();
    for (int i = 0; i < m_structure->numberOfAtoms(); i++) {
      if (m_structure->testAtomFlag(i, AtomFlag::Contact))
        continue;
      labels.append({atomLabels[i], QVector3D(positions(0, i), positions(1, i),
                                              positions(2, i))});
    }
  }
  return labels;
}

QVector<Label> Scene::fragmentLabels() {
  QVector<Label> labels;

  QVector<QVector3D> centroids = crystal()->centroidsOfFragments();
  for (int i = 0; i < centroids.size(); ++i) {
    // labels.append(qMakePair(QString("Fragment %1").arg(i), centroids[i]));
    labels.append(qMakePair(QString("%1").arg(i), centroids[i]));
  }
  return labels;
}

QVector<Label> Scene::surfaceLabels() {
  QVector<Label> labels;

  QStringList titles = listOfSurfaceTitles();
  if (titles.size() > 0) {
    auto centroids = listOfSurfaceCentroids();

    Q_ASSERT(titles.size() == centroids.size());
    for (int i = 0; i < titles.size(); ++i) {
      labels.append(qMakePair(titles[i], centroids[i]));
    }
  }
  return labels;
}

void Scene::generateCells(QPair<QVector3D, QVector3D> cellLimits) {
  if (m_deprecatedCrystal) {
    m_deprecatedCrystal->packUnitCells(cellLimits.second);
    m_atomsNeedUpdate = true;
  } else {
    m_structure->packUnitCells(cellLimits);
    m_atomsNeedUpdate = true;
  }
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
  for (const auto &fragPairInfo : std::as_const(crystal()->energyInfos())) {
    labels.append(
        qMakePair(fragPairInfo.label(), fragPairInfo.labelPosition()));
  }
  return labels;
}

void Scene::updateNoneProperties() {
  surfaceHandler()->updateAllSurfaceNoneProperties();
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
  return _showHydrogenBonds || _showCloseContacts.contains(true);
}

QSet<int> Scene::fragmentsConnectedToFragmentWithHydrogenBonds(int fragIndex) {
  QSet<int> result;

  const auto &hbondList = crystal()->hbondList();
  const auto &idsForFragment = crystal()->atomIdsForFragmentsRef()[fragIndex];
  const auto &hbondIntraFlag = crystal()->intramolecularHbondFlags();
  const auto &fragmentForAtom = crystal()->fragmentForAtom();
  foreach (int atomIndex, idsForFragment) {
    for (int i = 0; i < hbondList.size(); ++i) {
      if (hbondIntraFlag[i]) {
        continue; // skip intra hbonds
      }

      QPair<int, int> hbond = hbondList[i];

      if (hbond.first == atomIndex) {
        result.insert(fragmentForAtom[hbond.second]);
      }
      if (atomIndex == hbond.second) {
        result.insert(fragmentForAtom[hbond.first]);
      }
    }
  }

  return result;
}

QSet<int> Scene::fragmentsConnectedToFragmentWithCloseContacts(int fragIndex,
                                                               int ccIndex) {
  QSet<int> result;

  ContactsList contactsList = crystal()->closeContactsTable()[ccIndex];
  const auto &idsForFragment = crystal()->atomIdsForFragmentsRef()[fragIndex];
  const auto &fragmentForAtom = crystal()->fragmentForAtom();

  foreach (int atomIndex, idsForFragment) {
    for (int i = 0; i < contactsList.size(); ++i) {

      QPair<int, int> contact = contactsList[i];

      if (contact.first == atomIndex) {
        result.insert(fragmentForAtom[contact.second]);
      }
      if (atomIndex == contact.second) {
        result.insert(fragmentForAtom[contact.first]);
      }
    }
  }

  return result;
}

QList<int>
Scene::fragmentsConnectedToFragmentWithOnScreenContacts(int fragIndex) {
  QSet<int> result;

  if (_showHydrogenBonds) {
    result.unite(fragmentsConnectedToFragmentWithHydrogenBonds(fragIndex));
  }

  for (int ccIndex = 0; ccIndex <= CCMAX_INDEX; ++ccIndex) {
    if (_showCloseContacts[ccIndex]) {
      result.unite(
          fragmentsConnectedToFragmentWithCloseContacts(fragIndex, ccIndex));
    }
  }

  return result.values();
}

void Scene::setSelectStatusForAtomDoubleClick(int atomIndex) {
  if (m_periodicity == ScenePeriodicity::ZeroDimensions) {
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
    m_atomsNeedUpdate = true;
    return;
  }
  auto &atoms = crystal()->atoms();
  const auto &fragmentForAtom = crystal()->fragmentForAtom();

  Q_ASSERT(atomIndex >= 0 && atomIndex < atoms.size());

  if (atoms[atomIndex].isContactAtom()) { // handle contact atom double-click

  } else {

    Q_ASSERT(atomIndex < fragmentForAtom.size());

    // Allow selection to propagate to all fragments that form a network
    // The network is the union of all fragments connected by close contacts to
    // the double-clicked fragment
    if (hasOnScreenCloseContacts()) {
      QList<int> fragmentsToProcess, fragmentsFinished;

      fragmentsToProcess.append(fragmentForAtom[atomIndex]);

      while (fragmentsToProcess.size() > 0) {

        int currentFragment = fragmentsToProcess.takeFirst();
        crystal()->setSelectStatusForFragment(currentFragment, true);

        fragmentsFinished.append(currentFragment);

        // Find connected fragments and only add unprocessed ones to
        // fragmentsToProcess
        QList<int> connectedFragments =
            fragmentsConnectedToFragmentWithOnScreenContacts(currentFragment);
        foreach (int fragIndex, connectedFragments) {
          if (!fragmentsFinished.contains(fragIndex)) {
            fragmentsToProcess.append(fragIndex);
          }
        }
      }

    } else { // just select fragment associated with atomIndex
      int fragIndex = fragmentForAtom[atomIndex];
      atoms[atomIndex].setSelected(true);
      crystal()->setSelectStatusForFragment(
          fragIndex, !crystal()->fragmentIsSelected(fragIndex));
    }
  }
}

void Scene::selectAtomsSeparatedBySurface(bool inside) {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    if (inside)
      crystal()->selectAtomsInsideSurface(selectedSurface());
    else
      crystal()->selectAtomsOutsideSurface(selectedSurface());
  }
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
    int atomIndex;
    if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
      atomIndex = crystal()->atomsForBond()[bondIndex].first;
    } else {
      atomIndex = m_structure->atomsForBond(bondIndex).first;
    }
    setSelectStatusForAtomDoubleClick(atomIndex);
    return true;
  }
  default:
    break;
  }
  return false;
}

void Scene::handleAtomsNeedUpdate() { m_atomsNeedUpdate = true; }

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
  default: break;
  }
  return false;
}

bool Scene::processSelectionSingleClick(const QColor &color) {
    m_selection = m_selectionHandler->getSelectionFromColor(color);

    switch (m_selection.type) {
    case SelectionType::Atom: {
    size_t atom_idx = m_selection.index;
    if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
      auto &atoms = crystal()->atoms();
      Atom &atom = atoms[atom_idx];
      if (atom.isContactAtom()) {
	crystal()->completeFragmentContainingAtomIndex(atom_idx);
	m_atomsNeedUpdate = true;
	emit contactAtomExpanded();
      } else {
	atom.toggleSelected();
	m_atomsNeedUpdate = true;
      }
    } else {
      // TODO handle contact atom
      if (m_structure->testAtomFlag(atom_idx, AtomFlag::Contact)) {
	m_structure->completeFragmentContaining(atom_idx);
	m_atomsNeedUpdate = true;
	emit contactAtomExpanded();
      } else {
	m_structure->atomFlags(atom_idx) ^= AtomFlag::Selected;
	emit atomSelectionChanged();
	m_atomsNeedUpdate = true;
      }
    }
    return true;
    }
  case SelectionType::Bond: {
    size_t bond_idx = m_selection.index;
    if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
      auto &atoms = crystal()->atoms();
      const auto &atomsForBond = crystal()->atomsForBond();

      Atom &atom1 = atoms[atomsForBond[bond_idx].first];
      Atom &atom2 = atoms[atomsForBond[bond_idx].second];
      if (atom1.isSelected() != atom2.isSelected()) {
        atom1.setSelected(true);
        atom2.setSelected(true);
      } else {
        atom1.toggleSelected();
        atom2.toggleSelected();
      }
      m_atomsNeedUpdate = true;
    } else {
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
      m_atomsNeedUpdate = true;
    }
    return true ;
  }
  case SelectionType::Surface: {
    size_t surfaceIndex = m_selection.index;
    emit surfaceSelected(surfaceIndex);

    Surface *surface = surfaceHandler()->surfaceFromIndex(surfaceIndex);
    if (!surface)
      break;
    // update the selection value for the surface property
    float propertyValueAtVertex =
        surface->valueForCurrentPropertyAtVertex(m_selection.secondaryIndex);
    emit currentSurfaceFaceSelected(propertyValueAtVertex);

    // unmask faces of surfaces if necessary
    if (surface->hasMaskedFaces()) {
      surface->resetMaskedFaces();
    }
    return true;
  }
  default: break;
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
    if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
      auto &atoms = crystal()->atoms();
      const auto &fragmentForAtom = crystal()->fragmentForAtom();
      Atom &atom = atoms[atom_idx];
      if (!atom.isContactAtom()) {
        if (doubleClick) {
          crystal()->setSelectStatusForFragment(fragmentForAtom[atom_idx],
                                                true);
        } else {
          atom.toggleSelected();
          m_atomsNeedUpdate = true;
        }
        result = QVector4D(atom.pos(), atom_idx);
      }
    } else {
      if (m_structure->atomFlagsSet(atom_idx, AtomFlag::Contact))
        break;
      if (doubleClick)
        m_structure->selectFragmentContaining(atom_idx);
      else {
        m_structure->atomFlags(atom_idx) ^= AtomFlag::Selected;
      }
      emit atomSelectionChanged();
      m_atomsNeedUpdate = true;
      Vector3q pos = m_structure->atomicPositions().col(atom_idx);
      result = QVector4D(pos.x(), pos.y(), pos.z(), atom_idx);
    }
    break;
  }
  case SelectionType::Bond: {
    size_t bond_idx = m_selection.index;
    if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
      auto &atoms = crystal()->atoms();
      const auto &fragmentForAtom = crystal()->fragmentForAtom();
      const auto &atomsForBond = crystal()->atomsForBond();
      Atom &atom1 = atoms[atomsForBond[bond_idx].first];
      Atom &atom2 = atoms[atomsForBond[bond_idx].second];
      if (!atom1.isContactAtom() && !atom2.isContactAtom()) {
        if (doubleClick) {
          crystal()->setSelectStatusForFragment(
              fragmentForAtom[atomsForBond[bond_idx].first], true);
        } else {
          atom1.toggleSelected();
          atom2.toggleSelected();
          m_atomsNeedUpdate = true;
        }
        result = QVector4D((atom1.pos() + atom2.pos()) / 2.0, bond_idx);
      }
    } else {
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
      m_atomsNeedUpdate = true;
      const auto &positions = m_structure->atomicPositions();
      Vector3q pos = 0.5 * (positions.col(atomsForBond.first) +
                            positions.col(atomsForBond.second));
      result = QVector4D(pos.x(), pos.y(), pos.z(), bond_idx);
    }
  }
  case SelectionType::Surface: {
    size_t surfaceIndex = m_selection.index;
    Surface *surface = surfaceHandler()->surfaceFromIndex(surfaceIndex);
    if (!surface)
      break;
    result = QVector4D(surface->vertices()[m_selection.secondaryIndex], surfaceIndex);
  }
  default:
    break;
  }

  return result;
}

void Scene::populateSelectedAtom() {
  m_selectedAtom.index = m_selection.index;
  if (m_deprecatedCrystal) {
    const auto &atom = m_deprecatedCrystal->atoms()[m_selection.index];
    m_selectedAtom.atomicNumber = atom.element()->number();
    m_selectedAtom.label = atom.label();
    m_selectedAtom.position = atom.pos();
  } else if (m_structure) {
    m_selectedAtom.atomicNumber = m_structure->atomicNumbers()(m_selection.index);
    m_selectedAtom.label = m_structure->labels()[m_selection.index];
    const auto pos = m_structure->atomicPositions().col(m_selection.index);
    m_selectedAtom.position = QVector3D(pos.x(), pos.y(), pos.z());
  }
}

void Scene::populateSelectedBond() {
  m_selectedBond.index = m_selection.index;
  if (m_deprecatedCrystal) {
    const auto [idx_a, idx_b] =
        m_deprecatedCrystal->atomsForBond()[m_selection.index];
    {
      const auto &atom = m_deprecatedCrystal->atoms()[idx_a];
      auto &atomInfo = m_selectedBond.a;
      atomInfo.index = idx_a;
      atomInfo.atomicNumber = atom.element()->number();
      atomInfo.label = atom.label();
      atomInfo.position = atom.pos();
    }
    {
      const auto &atom = m_deprecatedCrystal->atoms()[idx_b];
      auto &atomInfo = m_selectedBond.b;
      atomInfo.index = idx_b;
      atomInfo.atomicNumber = atom.element()->number();
      atomInfo.label = atom.label();
      atomInfo.position = atom.pos();
    }
  } else if (m_structure) {
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
}

SelectionType Scene::decodeSelectionType(const QColor &color) {
  m_selection = m_selectionHandler->getSelectionFromColor(color);
  m_selectedAtom = {};
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
    Surface *surface = surfaceHandler()->surfaceFromIndex(m_selection.index);
    if (!surface) {
	m_selection.type = SelectionType::None;
    }
    break;
  }
  default:
    break;
  }
  return m_selection.type;
}

const SelectedBond &Scene::selectedBond() const { return m_selectedBond; }

void Scene::updateForPreferencesChange() {
  if (m_deprecatedCrystal) {
    m_deprecatedCrystal->updateForChangeInAtomConnectivity();
  } else if (m_structure) {
    setNeedsUpdate();
  }
}

QStringList Scene::uniqueElementSymbols() const {
  if (m_deprecatedCrystal) {
    return m_deprecatedCrystal->listOfElementSymbols();
  } else if (m_structure) {
    return m_structure->uniqueElementSymbols();
  }
  return {};
}

Surface *Scene::selectedSurface() {
  Q_ASSERT(m_selection.type == SelectionType::Surface);

  return surfaceHandler()->surfaceFromIndex(m_selection.index);
}

int Scene::selectedSurfaceIndex() {
  Q_ASSERT(m_selection.type == SelectionType::Surface);

  return m_selection.index;
}

int Scene::selectedSurfaceFaceIndex() {
  Q_ASSERT(m_selection.type == SelectionType::Surface);

  Surface *surface = surfaceHandler()->surfaceFromIndex(m_selection.index);
  Q_ASSERT(m_selection.secondaryIndex >= 0 &&
           m_selection.secondaryIndex < surface->numberOfFaces());

  return m_selection.secondaryIndex;
}

QPair<QVector3D, QVector3D> Scene::positionsForDistanceMeasurement(
    const QPair<SelectionType, int> &object1,
    const QPair<SelectionType, int> &object2) const {
  // For convenience, and to make code slightly more readable ...
  int index1 = object1.second;
  int index2 = object2.second;

  if (object1.first == SelectionType::Surface &&
      object2.first == SelectionType::Surface) {
    // Both objects selected are whole surfaces
    return m_surfaceHandler->positionsOfMinDistanceSurfaceSurface(index1,
                                                                  index2);

  } else if (object1.first == SelectionType::Surface) {
    // First object is whole surface, second is whole fragment
    int f2 = crystal()->fragmentIndexForAtomIndex(index2);
    return m_surfaceHandler->positionsOfMinDistanceFragSurface(*crystal(), f2,
                                                               index1);

  } else if (object2.first == SelectionType::Surface) {
    // First object is whole fragment, second is whole surface
    int f1 = crystal()->fragmentIndexForAtomIndex(index1);
    return m_surfaceHandler->positionsOfMinDistanceFragSurface(*crystal(), f1,
                                                               index2);

  } else {
    // Both objects selected are whole fragments
    int f1 = crystal()->fragmentIndexForAtomIndex(index1);
    int f2 = crystal()->fragmentIndexForAtomIndex(index2);
    return crystal()->positionsOfMinDistanceFragFrag(f1, f2);
  }
}
QPair<QVector3D, QVector3D>
Scene::positionsForDistanceMeasurement(const QPair<SelectionType, int> &object,
                                       const QVector3D &pos) const {
  if (object.first == SelectionType::Surface) {
    // First object is whole surface, second is a single atom or
    // single triangle
    return m_surfaceHandler->positionsOfMinDistancePosSurface(pos,
                                                              object.second);

  } else {
    // First object is whole fragment, second is single atom or
    // single triangle
    int f1 = crystal()->fragmentIndexForAtomIndex(object.second);
    return crystal()->positionsOfMinDistancePosFrag(pos, f1);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Atom Picking Names
//
////////////////////////////////////////////////////////////////////////////////////////////////
int Scene::numberOfAtoms() const {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    return m_deprecatedCrystal->numberOfAtoms();
  }
  return m_structure->numberOfAtoms();
}

bool Scene::isAtomName(int name) const {
  return (name >= atomBasename()) && (name < atomBasename() + numberOfAtoms());
}

int Scene::getAtomIndexFromAtomName(int name) const {
  Q_ASSERT(isAtomName(name));
  return name - atomBasename();
}

int Scene::atomBasename() const {
  return surfacesBasename() + numberOfFacesToDrawForAllSurfaces();
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Bond Picking Names
//
////////////////////////////////////////////////////////////////////////////////////////////////
bool Scene::isBondName(int name) const {
  return (name >= bondBasename()) && (name < bondBasename() + numberOfBonds());
}

int Scene::getBondIndexFromBondName(int name) const {
  Q_ASSERT(isBondName(name));
  return name - bondBasename();
}

int Scene::bondBasename() const { return atomBasename() + numberOfAtoms(); }

int Scene::numberOfBonds() const {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    return crystal()->numberOfBonds();
  }
  return m_structure->covalentBonds().size();
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Surface Picking Names
//
// We assign names to surfaces first, then atoms, then bonds.
// The reason is the names get stored in the select display lists and if named
// objects get
// added/deleted then we have to remake these lists.
//
// For example:
// If the atoms were assigned names before surfaces, then whenever atoms were
// added/deleted we would have
// to regenerate all the surface select displays lists because the names
// assigned to the surfaces
// would have changed. Since creating display lists (drawing) surfaces is more
// intensive than
// for atoms it makes sense to do it this way round.
//
// However if surfaces are added/deleted as well as regenerating the lists for
// the surfaces
// we also have to regenerate the lists for the atoms.
//
////////////////////////////////////////////////////////////////////////////////////////////////
bool Scene::isSurfaceName(int name) const {
  return (name >= surfacesBasename()) &&
         (name < surfacesBasename() + numberOfFacesToDrawForAllSurfaces());
}

int Scene::surfacesBasename() const { return 0; }

QPair<int, int> Scene::surfaceIndexAndFaceFromName(int name) const {
  QPair<int, int> surfaceAndFace;
  surfaceAndFace.first = -1;
  surfaceAndFace.second = -1;
  const auto &surfaceList = surfaceHandler()->surfaceList();

  if (isSurfaceName(name)) {
    name -= surfacesBasename();

    for (int s = 0; s < surfaceList.size(); ++s) {
      int nFaces = surfaceList[s]->numberOfFacesToDraw();
      if (name < nFaces) {
        surfaceAndFace.first = s;
        surfaceAndFace.second = name;
        break;
      }
      name -= nFaces;
    }
  }
  return surfaceAndFace;
}

// Used to calculate the total number of names required by all the surfaces for
// picking
int Scene::numberOfFacesToDrawForAllSurfaces() const {
  if (!m_surfaceHandler)
    return 0;
  return surfaceHandler()->numberOfFacesToDrawForAllSurfaces();
}

void Scene::setCloseContactVisible(int contactIndex, bool show) {
  Q_ASSERT(contactIndex >= 0 && contactIndex <= CCMAX_INDEX);
  _showCloseContacts[contactIndex] = show;
}

bool Scene::hasVisibleAtoms() const {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    return crystal()->hasVisibleAtoms();
  } else {
    return m_structure->numberOfAtoms() > 0;
  }
}

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
        (m_camera.projectionType() == CameraProjection::Orthographic) ? 1.0f : 0.0f;
    m_uniforms.u_normalMat = m_camera.normal();
    m_uniforms.u_cameraPosVec = m_camera.location();
    m_uniforms.u_time = std::chrono::duration<float>(time).count();
    m_uniforms.u_depthFogColor = fogColor;

    if(m_structureRenderer) {
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
    if (hasVisibleAtoms()) {
	drawSpheres(true);
	drawEllipsoids(true);
	drawCylinders(true);
	drawSurfaces(true);
        if(m_structureRenderer) {
	    m_structureRenderer->draw(true);
	}
    }
    m_uniforms.u_renderMode = storedRenderMode;
    m_uniforms.u_selectionMode = false;
}

void Scene::draw() {
  // needs to be done with a current opengl context
  if(m_structure && !m_structureRenderer) {
      m_structureRenderer = new cx::graphics::ChemicalStructureRenderer(m_structure, this);
      m_structureRenderer->setSelectionHandler(m_selectionHandler);
      m_structureRenderer->setAtomStyle(atomStyle());
      m_structureRenderer->setBondStyle(bondStyle());
  }
  updateRendererUniforms();

  if (_showUnitCellBox) {
    drawUnitCellBox();
  }

  if (hasVisibleAtoms()) {

    drawAtomsAndBonds();

    if (_showHydrogenBonds) {
      drawHydrogenBonds();
    }
    drawCloseContacts();
  }

  if (_showEnergyFramework) {
    drawEnergyFrameworks();
  }

  // must draw transparent/antialiased objects last
  drawSurfaces();

  if (hasVisibleAtoms()) {
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

#define SET_UNIFORM(uniform) \
    prog->setUniformValue(#uniform, m_uniforms.uniform)

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
  m_uniforms.u_screenGamma = settings::readSetting(settings::keys::SCREEN_GAMMA).toFloat();
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
  if (m_deprecatedCrystal == nullptr)
    return;
  m_crystalPlanes.push_back(plane);
  m_crystalPlanesNeedUpdate = true;
}

void Scene::setCrystalPlanes(const std::vector<CrystalPlane> &planes) {
  if (m_deprecatedCrystal == nullptr)
    return;
  m_crystalPlanes = planes;
  m_crystalPlanesNeedUpdate = true;
}

void Scene::materialChanged() {
  m_uniforms.u_materialMetallic =
      settings::readSetting(settings::keys::MATERIAL_METALLIC).toFloat();
  m_uniforms.u_materialRoughness =
      settings::readSetting(settings::keys::MATERIAL_ROUGHNESS).toFloat();
  m_uniforms.u_renderMode = settings::readSetting(settings::keys::MATERIAL).toInt();
}

void Scene::textSettingsChanged() {
  auto c2v = [](const QColor &c) {
    return QVector3D(c.redF(), c.greenF(), c.blueF());
  };

  m_uniforms.u_textColor = c2v(settings::readSetting(settings::keys::TEXT_COLOR).toString());
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

void Scene::drawEllipsoids(bool for_picking) {
  if (m_drawingStyle == DrawingStyle::WireFrame)
    return;

  m_ellipsoidRenderer->bind();
  setRendererUniforms(m_ellipsoidRenderer, for_picking);
  m_ellipsoidRenderer->draw();
  m_ellipsoidRenderer->release();
}

void Scene::drawSpheres(bool for_picking) {
  if (atomStyle() == AtomDrawingStyle::None ||
      atomStyle() == AtomDrawingStyle::Ellipsoid)
    return;

  Renderer * r = m_renderers["spheres"];
  r->bind();
  setRendererUniforms(r, for_picking);
  r->draw();
  r->release();
}

void Scene::drawCylinders(bool for_picking) {
  if (bondStyle() != BondDrawingStyle::Stick)
    return;
  if (false) {
    m_cylinderImpostorRenderer->bind();
    setRendererUniforms(m_cylinderImpostorRenderer, for_picking);
    m_cylinderImpostorRenderer->draw();
    m_cylinderImpostorRenderer->release();
  } else {
    m_meshCylinderRenderer->bind();
    setRendererUniforms(m_meshCylinderRenderer, for_picking);
    m_meshCylinderRenderer->draw();
    m_meshCylinderRenderer->release();
  }
}

void Scene::drawLines() {
  for (LineRenderer *lineRenderer : m_lineRenderers) {
    lineRenderer->bind();
    setRendererUniforms(lineRenderer);
    lineRenderer->draw();
    lineRenderer->release();
  }
}

void Scene::drawWireframe(bool for_picking) {
  if (for_picking || (m_drawingStyle != DrawingStyle::WireFrame))
    return;
  m_wireFrameBonds->bind();
  setRendererUniforms(m_wireFrameBonds, for_picking);
  m_wireFrameBonds->draw();
  m_wireFrameBonds->release();
}

void Scene::setThermalEllipsoidProbabilityString(const QString &probability) {
  _ellipsoidProbabilityString = probability;
  for (int i = 0; i < Atom::numThermalEllipsoidSettings; i++) {
    if (Atom::thermalEllipsoidProbabilityStrings[i] == probability) {
      _ellipsoidProbabilityScaleFactor =
          Atom::thermalEllipsoidProbabilityScaleFactors[i];
    }
  }
}

QMatrix3x3
Scene::atomThermalEllipsoidTransformationMatrix(const Atom &atom) const {
  QPair<Vector3q, Matrix3q> ampRot = atom.thermalTensorAmplitudesRotations();
  Vector3q amp = _ellipsoidProbabilityScaleFactor * ampRot.first;
  Matrix3q rot = ampRot.second;
  /* Now combine to form OpenGl transformation matrix: M = Rotation x Scaling =
   * rot x amp */
  QMatrix3x3 result;
  result.setToIdentity();
  result(0, 0) = amp(0) * rot(0, 0);
  result(1, 0) = amp(0) * rot(0, 1);
  result(2, 0) = amp(0) * rot(0, 2);

  result(0, 1) = amp(1) * rot(1, 0);
  result(1, 1) = amp(1) * rot(1, 1);
  result(2, 1) = amp(1) * rot(1, 2);

  result(0, 2) = amp(2) * rot(2, 0);
  result(1, 2) = amp(2) * rot(2, 1);
  result(2, 2) = amp(2) * rot(2, 2);
  return result;
}

/*
 \brief Skip bonds involving hydrogens if _showHydrogens is false, or if either
 atom i or j is not visible
 */
bool Scene::skipBond(int atom_i, int atom_j) {
  bool skip = false;
  const auto &atoms = crystal()->atoms();
  // if one of the following conditions is satisfied then skip = true
  skip = skip || (!_showHydrogens &&
                  (atoms[atom_i].isHydrogen() || atoms[atom_j].isHydrogen()));
  skip = skip || !(atoms[atom_i].isVisible() && atoms[atom_j].isVisible());
  return skip;
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

void Scene::setDisorderCycleHideShow(Atom &atom) {
  // todo move this to DeprecatedCrystal
  if (atom.isDisordered()) {
    if (_disorderCycleIndex <= 0) {
      atom.setVisible(true);
    } else {
      if (atom.disorderGroup() == _disorderCycleIndex) {
        atom.setVisible(true);
      } else {
        atom.setVisible(false);
      }
    }
  }
}

QColor Scene::getDisorderColor(const Atom &atom) {
  QColor color = atom.color();
  int atomDisorderGroup = atom.disorderGroup();
  if (atomDisorderGroup == 0) {
    return color;
  }

  const auto &disorderGroups = crystal()->disorderGroups();

  int index = disorderGroups.indexOf(atomDisorderGroup);
  return ColorSchemer::color(ColorScheme::Qualitative14Dark, index, 0,
                             disorderGroups.size());
}

void Scene::putBondAlongZAxis(QVector3D bondVector) {
  const GLfloat TOL = 0.00001f;

  float zComponent = bondVector.z() / bondVector.length();
  // DON'T substitute zComponent into the expression below.
  // There is a quirk on the Mac caused by qreal representations where
  // zComponent can be (slightly) > 1 thus generating a NAN with acos.
  GLfloat theta = acos(zComponent) * DEG_PER_RAD;
  GLfloat n = bondVector.toVector2D().length();
  GLfloat n1, n2;
  if (n > TOL) { // This handles the case when 0 < theta < 180 etc.
    n1 = -bondVector.y() / n;
    n2 = bondVector.x() / n;
  } else { // This handles the case when n<0.0001 and theta = 180, 360, etc.,
           // but != 0.
    n1 = 0.0;
    n2 = 1.0;
  }
  glRotatef(theta, n1, n2, 0.0);
}

void Scene::drawHydrogenBonds() {
  if (m_hydrogenBondLines == nullptr) {
    m_hydrogenBondLines = new LineRenderer();
  } else {
    m_hydrogenBondLines->clear();
  }

  QColor color =
      QColor(settings::readSetting(settings::keys::HBOND_COLOR).toString());
  double radius = contactLineThickness();
  m_hydrogenBondLines->beginUpdates();
  if (m_deprecatedCrystal) {
    const auto &atoms = crystal()->atoms();
    const auto &hbondList = crystal()->hbondList();
    const auto &hbondIntraFlag = crystal()->intramolecularHbondFlags();

    for (int i = 0; i < hbondList.size(); ++i) {
      if (hbondIntraFlag[i] && !crystal()->includeIntramolecularHBonds()) {
        continue; // skip intra hbonds if _includeIntraHBonds = false
      }
      const Atom &atom_i = atoms[hbondList[i].first];
      const Atom &atom_j = atoms[hbondList[i].second];
      cx::graphics::addDashedLineToLineRenderer(*m_hydrogenBondLines, atom_i.pos(),
                                            atom_j.pos(), radius, color);
    }
  } else {
    const auto &bonds = m_structure->hydrogenBonds();
    const auto &positions = m_structure->atomicPositions();
    for (const auto &[a, b] : bonds) {
      // skip intramolecular contacts
      if (m_structure->fragmentIndexForAtom(a) ==
          m_structure->fragmentIndexForAtom(b))
        continue;
      QVector3D pos_a(positions(0, a), positions(1, a), positions(2, a));
      QVector3D pos_b(positions(0, b), positions(1, b), positions(2, b));
      cx::graphics::addDashedLineToLineRenderer(*m_hydrogenBondLines, pos_a, pos_b,
                                            radius, color);
    }
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

  if (m_deprecatedCrystal != nullptr) {
    // TODO delete
    const auto &closeContactsTable = crystal()->closeContactsTable();
    const auto &atoms = crystal()->atoms();
    for (int ccIndex = 0; ccIndex <= CCMAX_INDEX; ++ccIndex) {
      if (!_showCloseContacts[ccIndex]) {
        continue; // Don't draw set of close contacts if not visible
      }

      QColor color = getColorForCloseContact(ccIndex);
      ContactsList contactsList = closeContactsTable[ccIndex];
      for (int i = 0; i < contactsList.size(); ++i) {
        const Atom &atom_i = atoms[contactsList[i].first];
        const Atom &atom_j = atoms[contactsList[i].second];
        cx::graphics::addDashedLineToLineRenderer(
            *m_closeContactLines, atom_i.pos(), atom_j.pos(), radius, color);
      }
    }
  } else {
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
  m_atomsNeedUpdate = true;
}

void Scene::expandAtomsWithinRadius(float radius, bool selection) {
  if (m_deprecatedCrystal) {
    if (selection)
      m_deprecatedCrystal->expandAtomsWithinRadiusOfSelectedAtoms(radius);
    else
      m_deprecatedCrystal->expandAtomsWithinRadius(radius);
  } else {
    m_structure->expandAtomsWithinRadius(radius, selection);
    m_atomsNeedUpdate = true;
  }
}

void Scene::selectAtomsOutsideRadiusOfSelectedAtoms(float radius) {

  QList<int> selectedAtoms;
  QList<int> unselectedAtoms;
  auto &atoms = crystal()->atoms();

  for (int i = 0; i < atoms.size(); i++) {
    if (atoms[i].isSelected()) {
      selectedAtoms.append(i);
    } else {
      unselectedAtoms.append(i);
    }
  }

  crystal()->unselectAllAtoms();

  for (int atom1 : selectedAtoms) {
    for (int atom2 : unselectedAtoms) {
      float distance = atoms[atom1].distanceToAtom(atoms[atom2]);
      if (distance > radius) {
        atoms[atom2].setSelected(true);
      }
    }
  }

  m_atomsNeedUpdate = true;
}

void Scene::reset() {
  if (m_deprecatedCrystal) {
    m_deprecatedCrystal->resetBondingModifications();
    m_deprecatedCrystal->resetToAsymmetricUnit();
  } else {
    m_structure->resetAtomsAndBonds();
    m_atomsNeedUpdate = true;
  }
  resetViewAndSelections();
  setSurfaceVisibilities(false);
}

int Scene::basenameForSurfaceWithIndex(int surfaceIndex) const {
  const auto &surfaceList = surfaceHandler()->surfaceList();
  Q_ASSERT(surfaceIndex < surfaceList.size());

  int basename = surfacesBasename();
  for (int s = 0; s < surfaceIndex; ++s) {
    int nFaces = surfaceList[s]->numberOfFacesToDraw();
    basename += nFaces;
  }
  return basename;
}

QPair<SelectionType, int>
Scene::selectionTypeAndIndexOfGraphicalObject(int openGlObjectName,
                                              bool firstAtomOfBond) {
  // If firstAtomForBond == true, then return the first atom index of the bond,
  // rather than the bond index.

  int index = -1;
  SelectionType selectionType = SelectionType::None;

  if (isAtomName(openGlObjectName)) {
    selectionType = SelectionType::Atom;
    index = getAtomIndexFromAtomName(openGlObjectName);
  } else if (isBondName(openGlObjectName)) {
    selectionType = SelectionType::Bond;
    if (firstAtomOfBond) {
      const auto &atomsForBond = crystal()->atomsForBond();
      index = atomsForBond[getBondIndexFromBondName(openGlObjectName)].first;
    } else {
      index = getBondIndexFromBondName(openGlObjectName);
    }
  } else if (isSurfaceName(openGlObjectName)) {
    selectionType = SelectionType::Surface;
    index = surfaceIndexAndFaceFromName(openGlObjectName).first;
  }
  return QPair<SelectionType, int>(selectionType, index);
}

void Scene::updateAtomsFromCrystal() {
  auto drawAsEllipsoid = [&](const Atom &atom) {
    if (atomStyle() != AtomDrawingStyle::Ellipsoid)
      return false;
    if (atom.isHydrogen() && !_drawHydrogenEllipsoids)
      return false;
    return true;
  };
  auto &atoms = crystal()->atoms();
  for (int i = 0; i < atoms.size(); ++i) {
    Atom &atom = atoms[i];
    if (crystal()->isDisordered()) {
      setDisorderCycleHideShow(atom);
    }
    if (atom.isVisible()) {
      if (!_showHydrogens && atom.isHydrogen()) {
        continue;
      }
      if (!_showSuppressedAtoms && atom.isSuppressed()) {
        continue;
      }
      QColor color = atom.color();
      if (atom.isContactAtom()) {
        color = color.lighter();
        // color.setAlphaF(0.75f);
      }
      if (applyDisorderColoring()) {
        color = getDisorderColor(atom);
      }
      auto pos = atom.pos();
      float radius = atom.covRadius() * 0.5;
      if (atomStyle() == AtomDrawingStyle::RoundCapped) {
        radius = bondThickness() / 2.0;
      } else if (atomStyle() == AtomDrawingStyle::VanDerWaalsSphere) {
        radius = atom.vdwRadius();
      }

      quint32 id = m_selectionHandler->add(SelectionType::Atom, i);
      QVector3D id_vec = m_selectionHandler->getColorFromId(id);

      if (drawAsEllipsoid(atom)) {
        QMatrix3x3 scales = atomThermalEllipsoidTransformationMatrix(atom);
        cx::graphics::addEllipsoidToEllipsoidRenderer(
            m_ellipsoidRenderer, pos, scales, color, id_vec, atom.isSelected());
      } else {
        cx::graphics::addSphereToEllipsoidRenderer(
            m_ellipsoidRenderer, pos, color, radius, id_vec, atom.isSelected());
      }
    }
  }
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
    CrystalPlaneGenerator generator(m_deprecatedCrystal->unitCell(), plane.hkl);
    const auto &aVector = generator.aVector();
    const auto &bVector = generator.aVector();
    const auto &origin = generator.origin();
    QVector3D qorigin(origin(0), origin(1), origin(2));
    QVector3D qa(aVector(0), aVector(1), aVector(2));
    QVector3D qb(bVector(0), bVector(1), bVector(2));
    cx::graphics::addPlaneToCrystalPlaneRenderer(*m_crystalPlaneRenderer, qorigin, qa, qb,
                                             plane.color);
  }
  m_crystalPlaneRenderer->endUpdates();
  m_crystalPlanesNeedUpdate = false;
}

void Scene::updateAtomsFromChemicalStructure() {
    m_structureRenderer->beginUpdates();
    m_structureRenderer->updateAtoms();
    m_structureRenderer->endUpdates();
}

void Scene::updateAtomsForDrawing() {
  if (!m_atomsNeedUpdate)
    return;
  if (m_ellipsoidRenderer == nullptr) {
    m_ellipsoidRenderer = new EllipsoidRenderer();
  } else {
    m_ellipsoidRenderer->clear();
  }
  if (!m_renderers.contains("spheres")) {
    m_renderers["spheres"] = new SphereImpostorRenderer();
  } else {
    m_renderers["spheres"]->clear();
    m_selectionHandler->clear(SelectionType::Atom);
  }
  m_ellipsoidRenderer->beginUpdates();
  m_renderers["spheres"]->beginUpdates();
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    updateAtomsFromCrystal();
  } else {
    updateAtomsFromChemicalStructure();
  }
  m_renderers["spheres"]->endUpdates();
  m_ellipsoidRenderer->endUpdates();
  updateBondsForDrawing();
  m_atomsNeedUpdate = false;
}

void Scene::updateBondsFromCrystal() {
  float radius = bondThickness() / 2.0;
  const auto &atoms = crystal()->atoms();
  const auto &atomsForBond = crystal()->atomsForBond();
  for (int b = 0; b < numberOfBonds(); ++b) {
    int i{atomsForBond[b].first};
    int j{atomsForBond[b].second};
    if (skipBond(i, j))
      continue;
    const Atom &atom_i = atoms[i];
    const Atom &atom_j = atoms[j];

    QVector3D pointA = atom_i.pos();
    QVector3D pointB = atom_j.pos();
    QColor colorA = atom_i.color();
    if (applyDisorderColoring()) {
      colorA = getDisorderColor(atom_i);
    }
    QColor colorB = atom_j.color();
    if (applyDisorderColoring()) {
      colorB = getDisorderColor(atom_j);
    }
    quint32 bond_id = m_selectionHandler->add(SelectionType::Bond, b);
    QVector3D id_color = m_selectionHandler->getColorFromId(bond_id);

    if (bondStyle() == BondDrawingStyle::Line) {
      cx::graphics::addLineToLineRenderer(
          *m_wireFrameBonds, pointA, 0.5 * pointA + 0.5 * pointB,
          DrawingStyleConstants::bondLineWidth, colorA);
      cx::graphics::addLineToLineRenderer(
          *m_wireFrameBonds, pointB, 0.5 * pointA + 0.5 * pointB,
          DrawingStyleConstants::bondLineWidth, colorB);
    } else {
      // Tube style
      cx::graphics::addCylinderToCylinderRenderer(
          m_cylinderImpostorRenderer, pointA, pointB, colorA, colorB, radius,
          id_color, atom_i.isSelected(), atom_j.isSelected());
      cx::graphics::addCylinderToCylinderRenderer(
          m_meshCylinderRenderer, pointA, pointB, colorA, colorB, radius,
          id_color, atom_i.isSelected(), atom_j.isSelected());
    }
  }
}

void Scene::updateBondsFromChemicalStructure() {
    m_structureRenderer->beginUpdates();
    m_structureRenderer->updateBonds();
    m_structureRenderer->endUpdates();
}

void Scene::updateBondsForDrawing() {

  if (!m_atomsNeedUpdate)
    return;
  if (m_cylinderImpostorRenderer == nullptr) {
    m_cylinderImpostorRenderer = new CylinderImpostorRenderer();
    m_meshCylinderRenderer = new CylinderRenderer();
  } else {
    m_selectionHandler->clear(SelectionType::Bond);
    m_cylinderImpostorRenderer->clear();
    m_meshCylinderRenderer->clear();
  }
  if (m_wireFrameBonds == nullptr) {
    m_wireFrameBonds = new LineRenderer();
  } else {
    m_wireFrameBonds->clear();
  }
  if (bondStyle() == BondDrawingStyle::None)
    return;
  m_cylinderImpostorRenderer->beginUpdates();
  m_meshCylinderRenderer->beginUpdates();
  m_wireFrameBonds->beginUpdates();
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    updateBondsFromCrystal();
  } else {
    updateBondsFromChemicalStructure();
  }

  m_wireFrameBonds->endUpdates();
  m_meshCylinderRenderer->endUpdates();
  m_cylinderImpostorRenderer->endUpdates();
}

void Scene::updateMeshesForDrawing() {
  if (!m_surfacesNeedUpdate)
    return;
  m_selectionHandler->clear(SelectionType::Surface);

  if (m_faceHighlights == nullptr) {
    m_faceHighlights = new LineRenderer();
  } else {
    m_faceHighlights->clear();
  }
  const auto &surfaceList = surfaceHandler()->surfaceList();
  m_faceHighlights->beginUpdates();
  for (int s = 0; s < surfaceList.size(); ++s) {
    if (m_meshRenderers.size() <= s)
      m_meshRenderers.push_back(new MeshRenderer());
    MeshRenderer *meshRenderer = m_meshRenderers[s];
    std::vector<MeshVertex> mesh_vertices, mesh_masked_vertices;
    std::vector<MeshRenderer::IndexTuple> mesh_indices, mesh_masked_indices;
    Surface *surface = surfaceList[s];
    if (surface->isParent() || !surface->isVisible())
      continue;
    surface->drawFaceHighlights(m_faceHighlights);
    const auto &verts = surface->vertices();
    const auto &vertex_normals = surface->vertexNormals();
    const auto &colors = surface->vertexColors();
    const auto &faces = surface->faces();

    quint32 id = m_selectionHandler->add(SelectionType::Surface, s);

    bool has_masked_faces = surface->hasMaskedFaces();
    QVector3D maskColor(MASKED_COLOR[0], MASKED_COLOR[1], MASKED_COLOR[2]);

    for (int i = 0; i < verts.size(); i++) {
      quint32 vertex_id = id + i;
      QVector3D selectionColor = m_selectionHandler->getColorFromId(vertex_id);
      mesh_vertices.emplace_back(
          verts[i], vertex_normals[i],
          QVector3D(colors[i][0], colors[i][1], colors[i][2]), selectionColor);
      if (has_masked_faces) {
        mesh_masked_vertices.emplace_back(
            verts[i], vertex_normals[i], maskColor, selectionColor);
      }
    }
    for (int f = 0; f < faces.size(); f++) {
      const auto &face = faces[f];
      if (surface->faceMasked(f)) {
        mesh_masked_indices.push_back({face.i, face.j, face.k});
      } else {
        mesh_indices.push_back({face.i, face.j, face.k});
      }
    }
    meshRenderer->clear();
    meshRenderer->addMesh(mesh_vertices, mesh_indices);
    if (has_masked_faces)
      meshRenderer->addMesh(mesh_masked_vertices, mesh_masked_indices);
    meshRenderer->setFrontFace(surface->frontFace());
    if (surface->isVoidSurface())
      meshRenderer->setCullFace(false, GL_BACK);
  }

  m_faceHighlights->endUpdates();

  m_surfacesNeedUpdate = false;
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
    cx::graphics::addTextToBillboardRenderer(*m_billboardTextLabels, label.second,
                                         label.first);
  }
  m_billboardTextLabels->endUpdates();
  m_labelsNeedUpdate = false;
}

void Scene::drawAtomsAndBonds() {
  updateAtomsForDrawing();
  drawSpheres();
  drawEllipsoids();
  drawCylinders();
  drawWireframe();

  if(m_structureRenderer) {
      m_structureRenderer->draw();
  }
}

void Scene::drawLabels() {
  m_billboardTextLabels->bind();
  setRendererUniforms(m_billboardTextLabels);
  m_billboardTextLabels->draw();
  m_billboardTextLabels->release();
}

void Scene::drawSurfaces(bool for_picking) {
  // TODO add surfaces to non-crystals
  if (m_periodicity == ScenePeriodicity::ZeroDimensions)
    return;
  const auto &surfaceList = surfaceHandler()->surfaceList();
  if (surfaceList.size() == 0)
    return;

  updateMeshesForDrawing();

  if (!for_picking) {
    m_faceHighlights->bind();
    setRendererUniforms(m_faceHighlights);
    m_faceHighlights->draw();
    m_faceHighlights->release();
  }

  int s = 0;
  for (MeshRenderer *meshRenderer : std::as_const(m_meshRenderers)) {
    if (s >= surfaceList.size())
      break;
    if (!surfaceList[s]->isVisible()) {
      s++;
      continue;
    }
    if (surfaceList[s]->isTransparent()) {
      meshRenderer->setAlpha(0.9f);
    } else {
      meshRenderer->setAlpha(1.0f);
    }
    meshRenderer->bind();
    setRendererUniforms(meshRenderer, for_picking);

    meshRenderer->draw();
    meshRenderer->release();
    s++;
  }
}

void Scene::setShowSurfaceInteriors(bool show) {
  surfaceHandler()->setShowSurfaceInteriors(show);
}

void Scene::drawUnitCellBox() {
  // setup unit cell lines

  if (m_unitCellLines == nullptr) {

    m_unitCellLines = new LineRenderer();
    m_unitCellLines->beginUpdates();
    QVector3D a(1, 0, 0);
    QVector3D b(0, 1, 0);
    QVector3D c(0, 0, 1);

    if (m_deprecatedCrystal) {
      const auto &unitCell = crystal()->unitCell();
      a = unitCell.aVector();
      b = unitCell.bVector();
      c = unitCell.cVector();
    } else {
      const auto &unitCell = m_structure->cellVectors();
      a = QVector3D(unitCell(0, 0), unitCell(1, 0), unitCell(2, 0));
      b = QVector3D(unitCell(0, 1), unitCell(1, 1), unitCell(2, 1));
      c = QVector3D(unitCell(0, 2), unitCell(1, 2), unitCell(2, 2));
    }
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

void Scene::drawEnergyFrameworks() {
  if (m_energyFrameworkCylinders == nullptr) {
    m_energyFrameworkCylinders = new CylinderRenderer();
  } else {
    m_energyFrameworkCylinders->clear();
  }

  if (m_energyFrameworkSpheres == nullptr) {
    m_energyFrameworkSpheres = new EllipsoidRenderer();
  } else {
    m_energyFrameworkSpheres->clear();
  }

  if (m_energyFrameworkLines == nullptr) {
    m_energyFrameworkLines = new LineRenderer();
  } else {
    m_energyFrameworkLines->clear();
  }

  if (m_energyFrameworkLabels == nullptr) {
    m_energyFrameworkLabels = new BillboardRenderer();
  } else {
    m_energyFrameworkLabels->clear();
  }
  m_energyFrameworkCylinders->beginUpdates();
  m_energyFrameworkSpheres->beginUpdates();
  m_energyFrameworkLines->beginUpdates();
  m_energyFrameworkLabels->beginUpdates();
  const auto &energyInfos = crystal()->energyInfos();
  const FragmentPairStyle style = currentEnergyFrameworkStyle();
  for (const auto &energyInfo : std::as_const(energyInfos)) {
    energyInfo.draw(style, m_energyFrameworkSpheres, m_energyFrameworkCylinders,
                    m_energyFrameworkLines, m_energyFrameworkLabels);
  }
  m_energyFrameworkCylinders->endUpdates();
  m_energyFrameworkSpheres->endUpdates();
  m_energyFrameworkLines->endUpdates();
  m_energyFrameworkLabels->endUpdates();

  m_energyFrameworkSpheres->bind();
  setRendererUniforms(m_energyFrameworkSpheres);
  m_energyFrameworkSpheres->draw();
  m_energyFrameworkSpheres->release();

  m_energyFrameworkCylinders->bind();
  setRendererUniforms(m_energyFrameworkCylinders);
  m_energyFrameworkCylinders->draw();
  m_energyFrameworkCylinders->release();

  m_energyFrameworkLines->bind();
  setRendererUniforms(m_energyFrameworkLines);
  m_energyFrameworkLines->draw();
  m_energyFrameworkLines->release();

  m_energyFrameworkLabels->bind();
  setRendererUniforms(m_energyFrameworkLabels);
  m_energyFrameworkLabels->draw();
  m_energyFrameworkLabels->release();
}

FragmentPairStyle Scene::currentEnergyFrameworkStyle() {
  auto frameworkStyles = getFrameworkStyles();
  return frameworkStyles[currentFramework()];
}

AtomDrawingStyle Scene::atomStyle() const {
  return atomStyleForDrawingStyle(m_drawingStyle);
}
BondDrawingStyle Scene::bondStyle() const {
  return bondStyleForDrawingStyle(m_drawingStyle);
}

void Scene::cycleDisorderHighlighting() {
  const auto &disorderGroups = crystal()->disorderGroups();
  if (_disorderCycleIndex == disorderGroups.size()) {
    _disorderCycleIndex = -2;
  }
  _disorderCycleIndex += 1;
  m_atomsNeedUpdate = true;
}

bool Scene::applyDisorderColoring() {
  return _highlightMode == HighlightMode::Normal && _disorderCycleIndex == -1;
}

void Scene::toggleFragmentColors() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->toggleFragmentColors();
  }
}

void Scene::colorFragmentsByEnergyPair() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->colorFragmentsByEnergyPair();
  }
}

void Scene::clearFragmentColors() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->clearFragmentColors();
  }
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
  m_atomsNeedUpdate = true;
}

void Scene::toggleDrawHydrogenEllipsoids(bool hEllipsoids) {
  _drawHydrogenEllipsoids = hEllipsoids;
  m_atomsNeedUpdate = true;
}

void Scene::setShowHydrogens(bool show) {
  _showHydrogens = show;
  m_atomsNeedUpdate = true;
}

void Scene::cycleEnergyFramework(bool cycleBackwards) {
  if (cycleBackwards) {
    if (_energyCycleIndex == minCycleIndex()) {
      _energyCycleIndex = maxCycleIndex() + 1;
    }
    _energyCycleIndex -= 1;
  } else {
    if (_energyCycleIndex == maxCycleIndex()) {
      _energyCycleIndex = minCycleIndex() - 1;
    }
    _energyCycleIndex += 1;
  }
  m_atomsNeedUpdate = true;
}

void Scene::turnOffEnergyFramework() { _showEnergyFramework = false; }

void Scene::turnOnEnergyFramework() {
  _showEnergyFramework = true;
  QElapsedTimer timer;
  timer.start();
  crystal()->updateEnergyInfo(currentFramework());
  qDebug() << "Took " << timer.elapsed() << "ms to update energy frameworks";
}

FrameworkType Scene::currentFramework() {
  return cycleToFramework[_energyCycleIndex];
}

int Scene::minCycleIndex() { return cycleToFramework.keys().first(); }

int Scene::maxCycleIndex() { return cycleToFramework.keys().last(); }

void Scene::generateAllExternalFragments() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    // TODO need to handle emit currentCrystalContentsChanged in project
    m_surfaceHandler->generateExternalFragmentsForSurface(
        m_deprecatedCrystal, selectedSurfaceIndex());
    emit sceneContentsChanged();
  }
}

void Scene::generateInternalFragment() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    // TODO need to handle emit currentCrystalContentsChanged in project
    m_surfaceHandler->generateInternalFragment(m_deprecatedCrystal,
                                               selectedSurfaceIndex());
    emit sceneContentsChanged();
  }
}

void Scene::generateExternalFragment() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    // TODO need to handle emit currentCrystalContentsChanged in project
    m_surfaceHandler->generateExternalFragmentForSurface(
        m_deprecatedCrystal, selectedSurfaceIndex(),
        selectedSurfaceFaceIndex());
    emit sceneContentsChanged();
  }
}

bool Scene::hasAllAtomsSelected() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    return m_deprecatedCrystal->hasAllAtomsSelected();
  } else {
    return m_structure->allAtomsHaveFlags(AtomFlag::Selected);
  }
}

Vector3q Scene::convertToCartesian(const Vector3q &vec) const {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    return Vector3q(vec.x() * m_deprecatedCrystal->aAxis() +
                    vec.y() * m_deprecatedCrystal->bAxis() +
                    vec.z() * m_deprecatedCrystal->cAxis());
  }
  return vec;
}

void Scene::resetOrigin() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->resetOrigin();
  } else {
    m_structure->resetOrigin();
  }
}

void Scene::translateOrigin(const Vector3q &t) {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->setOrigin(m_deprecatedCrystal->origin() + t);
  } else {
    m_structure->setOrigin(m_structure->origin() + t);
  }
}

float Scene::radius() const {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    return m_deprecatedCrystal->radius();
  } else {
    return m_structure->radius();
  }
}

void Scene::discardSelectedAtoms() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->discardSelectedAtoms();
  }
}

void Scene::resetAllAtomColors() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->resetAllAtomColors();
  }
}

void Scene::bondSelectedAtoms() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->bondSelectedAtoms();
  }
}

void Scene::unbondSelectedAtoms() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->unbondSelectedAtoms();
  }
}

void Scene::suppressSelectedAtoms() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->suppressSelectedAtoms();
  } else if (m_structure) {
    m_structure->setFlagForAtomsFiltered(AtomFlag::Suppressed,
                                         AtomFlag::Selected, true);
    m_atomsNeedUpdate = true;
  }
}

void Scene::unsuppressSelectedAtoms() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->unsuppressSelectedAtoms();
  } else if (m_structure) {
    m_structure->setFlagForAtomsFiltered(AtomFlag::Suppressed,
                                         AtomFlag::Selected, false);
    m_atomsNeedUpdate = true;
  }
}

void Scene::unsuppressAllAtoms() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->unsuppressAllAtoms();
  }
}

void Scene::setSelectStatusForSuppressedAtoms(bool status) {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    if (status)
      m_deprecatedCrystal->selectAllSuppressedAtoms();
    else {
      auto &atoms = crystal()->atoms();
      // Turn off selection before hiding suppressed atoms
      for (Atom &atom : atoms) {
        if (atom.isSuppressed()) {
          atom.setSelected(false);
        }
      }
    }
  } else if (m_structure) {
    m_structure->setFlagForAtomsFiltered(AtomFlag::Selected,
                                         AtomFlag::Suppressed, status);
    m_atomsNeedUpdate = true;
  }
}

void Scene::selectAllAtoms() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->selectAllAtoms();
  } else {
    m_structure->setFlagForAllAtoms(AtomFlag::Selected);
  }
}

void Scene::invertSelection() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->invertSelection();
  } else {
    for (int i = 0; i < m_structure->numberOfAtoms(); i++) {
      auto &flags = m_structure->atomFlags(i);
      flags ^= AtomFlag::Selected;
    }
  }
}

void Scene::deleteIncompleteFragments() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->discardIncompleteFragments();
  } else if (m_structure) {
    m_structure->deleteIncompleteFragments();
    m_atomsNeedUpdate = true;
  }
}

void Scene::completeAllFragments() {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    if (m_deprecatedCrystal->hasSelectedAtoms() &&
        m_deprecatedCrystal->hasIncompleteSelectedFragments()) {
      m_deprecatedCrystal->completeSelectedFragments();
    } else {
      m_deprecatedCrystal->completeAllFragments();
    }
  } else {
    m_structure->completeAllFragments();
    m_atomsNeedUpdate = true;
  }
}

void Scene::colorSelectedAtoms(const QColor &color) {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    m_deprecatedCrystal->colorSelectedAtoms(color);
  }
}

bool Scene::hasHydrogens() const {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    return m_deprecatedCrystal->hasHydrogens();
  } else {
    return (m_structure->atomicNumbers().array() == 1).any();
  }
}

bool Scene::hasSelectedAtoms() const {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    return m_deprecatedCrystal->hasSelectedAtoms();
  } else {
    return m_structure->anyAtomHasFlags(AtomFlag::Selected);
  }
}

bool Scene::hasSuppressedAtoms() const {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    return m_deprecatedCrystal->hasSuppressedAtoms();
  } else {
    return m_structure->anyAtomHasFlags(AtomFlag::Suppressed);
  }
}

bool Scene::hasIncompleteFragments() const {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    return m_deprecatedCrystal->hasIncompleteFragments();
  } else {
    return m_structure->hasIncompleteFragments();
  }
}

int Scene::numberOfSelectedAtoms() const {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    return m_deprecatedCrystal->selectedAtomIndices().size();
  } else {
    return m_structure->atomIndicesWithFlags(AtomFlag::Selected).size();
  }
}

bool Scene::hasAtomsWithCustomColor() const {
  if (m_periodicity == ScenePeriodicity::ThreeDimensions) {
    return m_deprecatedCrystal->hasAtomsWithCustomColor();
  } else {
    return m_structure->anyAtomHasFlags(AtomFlag::CustomColor);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Surfaces
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool Scene::setCurrentSurfaceIndex(int surfaceIndex) {
  if (surfaceHandler() != nullptr)
    return surfaceHandler()->setCurrentSurfaceIndex(surfaceIndex);
  return false;
}

bool Scene::hasSurface() const {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return false;
  return surfaceHandler()->numberOfSurfaces() > 0;
}

bool Scene::loadSurfaceData(const JobParameters &jobParams) {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return false;
  return surfaceHandler()->loadSurfaceData(m_deprecatedCrystal, jobParams);
}

void Scene::resetCurrentSurface() {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return;
  surfaceHandler()->resetCurrentSurfaceIndex();
}

void Scene::toggleVisibilityOfSurface(int surfaceIndex) {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return;
  surfaceHandler()->toggleSurfaceVisibility(surfaceIndex);
  m_surfacesNeedUpdate = true;
}

void Scene::hideSurface(int surfaceIndex) {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return;
  surfaceHandler()->setSurfaceVisibility(surfaceIndex, false);
  m_surfacesNeedUpdate = true;
}

void Scene::deleteCurrentSurface() {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return;
  surfaceHandler()->deleteCurrentSurface();
  m_surfacesNeedUpdate = true;
}

void Scene::setSurfaceVisibilities(bool visible) {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return;
  surfaceHandler()->setAllSurfaceVisibilities(visible);
  m_surfacesNeedUpdate = true;
}

Surface *Scene::currentSurface() {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return nullptr;
  return surfaceHandler()->currentSurface();
}

QStringList Scene::listOfSurfaceTitles() {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return {};
  return surfaceHandler()->surfaceTitles();
}

QVector<bool> Scene::listOfSurfaceVisibilities() {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return {};
  return surfaceHandler()->surfaceVisibilities();
}

QVector<QVector3D> Scene::listOfSurfaceCentroids() {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return {};
  return surfaceHandler()->surfaceCentroids();
}

bool Scene::hasVisibleSurfaces() {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return false;
  return surfaceHandler()->hasVisibleSurface();
}

bool Scene::hasHiddenSurfaces() {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return false;
  return surfaceHandler()->hasHiddenSurface();
}

int Scene::numberOfVisibleSurfaces() {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return 0;
  return surfaceHandler()->numberOfVisibleSurfaces();
}

int Scene::numberOfSurfaces() const {
  if (surfaceHandler() == nullptr)
    return 0;
  return surfaceHandler()->numberOfSurfaces();
}

int Scene::surfaceEquivalent(const JobParameters &jobParams) const {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return -1;
  return surfaceHandler()->equivalentSurfaceIndex(*m_deprecatedCrystal,
                                                  jobParams);
}

int Scene::propertyEquivalentToRequestedProperty(const JobParameters &jobParams,
                                                 int surfaceIndex) const {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return -1;
  return surfaceHandler()
      ->indexOfPropertyEquivalentToRequestedPropertyForSurface(surfaceIndex,
                                                               jobParams);
}

void Scene::deleteSelectedSurface() {
  if (surfaceHandler() == nullptr)
    return;
  deleteSurface(selectedSurface());
}

void Scene::deleteSurface(Surface *surface) {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return;
  surfaceHandler()->deleteSurface(surface);
  m_surfacesNeedUpdate = true;
}

void Scene::deleteFragmentContainingAtomIndex(int atomIndex) {
  if (m_deprecatedCrystal) {
    m_deprecatedCrystal->deleteFragmentContainingAtomIndex(atomIndex);
  } else {
    m_structure->deleteFragmentContainingAtomIndex(atomIndex);
    m_atomsNeedUpdate = true;
  }
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
  if (m_deprecatedCrystal) {
    bool fragmentIsSelected =
        m_deprecatedCrystal->fragmentContainingAtomIndexIsSelected(atomIndex);
    m_deprecatedCrystal->completeFragmentContainingAtomIndex(atomIndex);

    // If the fragment was completely selected before completion
    // Then select the newly added fragment atoms too.
    if (fragmentIsSelected) {
      m_deprecatedCrystal->selectFragmentContaining(atomIndex);
    }
  } else if (m_structure) {
    m_structure->completeFragmentContaining(atomIndex);
    emit atomSelectionChanged();
    m_atomsNeedUpdate = true;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Cloning
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void Scene::cloneCurrentSurfaceForSelection() {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return;
  surfaceHandler()->cloneCurrentSurfaceForSelection(*m_deprecatedCrystal);
  m_surfacesNeedUpdate = true;
}

void Scene::cloneCurrentSurfaceForAllFragments() {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return;
  surfaceHandler()->cloneCurrentSurfaceForAllFragments(*m_deprecatedCrystal);
  m_surfacesNeedUpdate = true;
}

void Scene::cloneCurrentSurfaceWithCellShifts(const QVector3D &cellLimits) {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return;
  surfaceHandler()->cloneCurrentSurfaceWithCellShifts(*m_deprecatedCrystal,
                                                      cellLimits);
  m_surfacesNeedUpdate = true;
}

Surface *Scene::cloneSurface(const Surface *parentSurface) {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return nullptr;
  return surfaceHandler()->cloneSurface(*parentSurface);
}

Surface *Scene::cloneSurfaceWithCellShift(const Surface *sourceSurface,
                                          Vector3q shift) {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return nullptr;
  Surface *surf = surfaceHandler()->cloneSurfaceWithCellShift(
      *m_deprecatedCrystal, *sourceSurface, shift);
  m_surfacesNeedUpdate = true;
  return surf;
}

Surface *Scene::cloneSurfaceForFragment(int fragmentIndex,
                                        const Surface *sourceSurface) {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return nullptr;
  Surface *surf = surfaceHandler()->cloneSurfaceForFragment(
      *m_deprecatedCrystal, *sourceSurface, fragmentIndex);
  m_surfacesNeedUpdate = true;
  return surf;
}

void Scene::cloneCurrentSurfaceForFragmentList(
    const QVector<int> fragmentIndices) {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return;
  surfaceHandler()->cloneCurrentSurfaceForFragmentList(*m_deprecatedCrystal,
                                                       fragmentIndices);
  m_surfacesNeedUpdate = true;
}

bool Scene::cloneAlreadyExists(const Surface *surface,
                               CrystalSymops crystalSymops) {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return false;
  return surfaceHandler()->cloneAlreadyExists(*m_deprecatedCrystal, *surface,
                                              crystalSymops);
}

Surface *Scene::existingClone(const Surface *surface,
                              CrystalSymops crystalSymops) {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return nullptr;
  return surfaceHandler()->existingClone(*m_deprecatedCrystal, *surface,
                                         crystalSymops);
}

bool Scene::hasFragmentsWithoutClones(Surface *surface) {
  bool result = false;
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return result;

  const Surface *sourceSurface = surface->parent();

  for (int i = 0; i < m_deprecatedCrystal->numberOfFragments(); i++) {
    CrystalSymops crystalSymops =
        m_deprecatedCrystal->calculateCrystalSymops(sourceSurface, i);

    if (crystalSymops.size() !=
        0) { // Can we clone the surface for this fragment?
      if (!cloneAlreadyExists(sourceSurface, crystalSymops)) {
        result = true;
        break;
      }
    }
  }
  return result;
}

// Should only be called by operator<<
// Assumes that children have "+ "
void Scene::rebuildSurfaceParentCloneRelationship() {
  if (m_periodicity != ScenePeriodicity::ThreeDimensions)
    return;
  surfaceHandler()->rebuildSurfaceParentCloneRelationship();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Fingerprints
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void Scene::toggleAtomsForFingerprintSelectionFilter(bool show) {
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

  m_atomsNeedUpdate = true;
}

/*! Surface features include (i) surface patches as a result of filtering and
 (ii) face highlights (red arrows) as a result
 of clicking on the fingerprint plot.
 */
void Scene::resetSurfaceFeatures() {
  if (m_deprecatedCrystal) {
    for (int i = 0; i < surfaceHandler()->numberOfSurfaces(); i++) {
      Surface *surface = surfaceHandler()->surfaceFromIndex(i);
      surface->resetFaceHighlights();
      if (surface->hasMaskedFaces()) {
        surface->resetMaskedFaces();
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Scene::exportToPovrayTextStream(QTextStream &ts) {

  ts << "#include \"colors.inc\"\n";
  ts << "#include \"glass.inc\"\n";
  ts << "#include \"glass.inc\"\n";
  ts << "#include \"stones.inc\"\n";

  QMatrix4x4 view = m_camera.view();
  QMatrix4x4 projection = m_camera.projection();
  QMatrix4x4 viewInverse = view.inverted();
  QVector3D cameraPosition = viewInverse.map(QVector3D(0, 0, 0));
  QVector3D lookAtDirection = viewInverse.mapVector(QVector3D(0, 0, -1));
  QVector3D upDirection = viewInverse.mapVector(QVector3D(0, 1, 0));
  QVector3D lookAtPoint = cameraPosition + lookAtDirection;
  float scale = m_camera.model()(0, 0);
  ts << "camera {\n";
  if (m_camera.projectionType() != CameraProjection::Perspective) {
    float fov = 2.0 * qRadiansToDegrees(atan(1.0 / projection(1, 1)));
    // Camera settings
    ts << "  perspective\n";
    ts << "  angle " << fov << "\n";
    ts << "  up <" << upDirection.x() << ", " << upDirection.y() << ", "
       << upDirection.z() << ">\n";
  } else {
    QVector3D right = m_camera.right() * scale;
    QVector3D up = m_camera.up() * scale;
    ts << "  orthographic\n";
    ts << "  right <" << right.x() << ", " << right.y() << ", " << right.z()
       << "> \n";
    ts << "  up <" << up.x() << ", " << up.y() << ", " << up.z() << "> \n";
  }
  ts << "  location <" << cameraPosition.x() << ", " << cameraPosition.y()
     << ", " << cameraPosition.z() << ">\n";
  ts << "  look_at <" << lookAtPoint.x() << ", " << lookAtPoint.y() << ", "
     << lookAtPoint.z() << ">\n";
  ts << "}\n\n";

  ts << "global_settings {\n";
  ts << "  ambient_light White\n";
  ts << "  assumed_gamma 1.0\n";
  ts << "}\n";

  for (int i = 0; i < m_uniforms.u_numLights; i++) {
    ts << "light_source {\n";
    ts << "  <" << m_uniforms.u_lightPos(i, 0) << ", " << m_uniforms.u_lightPos(i, 1)
       << ", " << m_uniforms.u_lightPos(i, 2) << ">\n";
    ts << "  color rgb <1, 1, 1>\n";
    ts << "}\n\n";
  }
  const auto &atoms = crystal()->atoms();
  for (const auto &atom : atoms) {
    float r = atom.color().redF();
    float g = atom.color().greenF();
    float b = atom.color().blueF();
    QVector3D scaledCenter = atom.pos();
    float radius = atom.covRadius();
    ts << "sphere {\n";
    ts << QString::fromStdString(fmt::format("<{:.3f},{:.3f},{:.3f}>, {:.3f}\n",
                                             scaledCenter.x(), scaledCenter.y(),
                                             scaledCenter.z(), radius));
    ts << "pigment { rgb <" << r << ", " << g << ", " << b << "> }\n";
    ts << "finish {\n ambient 0.2 diffuse 0.3 reflection .25 specular 0.5 "
          "roughness 0.15\n}\n";
    ts << "}\n";
  }

  const auto &surfaceList = surfaceHandler()->surfaceList();
  for (Surface *surface : surfaceList) {
    surface->exportToPovrayTextStream(ts, surface->surfaceName(), "1",
                                      "ambient 1");
  }
}

QDataStream &operator<<(QDataStream &ds, const Scene &scene) {
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

  return ds;
}

QDataStream &operator>>(QDataStream &ds, Scene &scene) {
  // When the drawable crystal is created, init() is called which
  // initializes all the private members. The settings::keys:: are over
  // written for all the ones read in to return the DrawableCrystal
  // to the (almost) the state it was in when it was saved.

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
  return ds;
}
