#include "crystalsurfacehandler.h"
#include "deprecatedcrystal.h"
#include "surfacedata.h"

CrystalSurfaceHandler::CrystalSurfaceHandler() {}

CrystalSurfaceHandler::~CrystalSurfaceHandler() {
  QVectorIterator<Surface *> surfaceIterator(m_surfaces);
  while (surfaceIterator.hasNext()) {
    delete surfaceIterator.next();
  }
}

Surface *CrystalSurfaceHandler::currentSurface() const {
  return surfaceFromIndex(m_currentSurfaceIndex);
}

Surface *CrystalSurfaceHandler::surfaceFromIndex(int index) const {
  if (index < 0 || index >= numberOfSurfaces())
    return nullptr;
  return m_surfaces[index];
}

void CrystalSurfaceHandler::setShowSurfaceInteriors(bool visible) {
  for (int s = 0; s < m_surfaces.size(); ++s) {
    m_surfaces[s]->setShowInterior(visible);
  }
}

void CrystalSurfaceHandler::updateAllSurfaceNoneProperties() {
  for (int s = 0; s < m_surfaces.size(); ++s) {
    m_surfaces[s]->updateNoneProperty();
  }
}

int CrystalSurfaceHandler::numberOfFacesToDrawForAllSurfaces() const {
  int total = 0;
  foreach (Surface *surface, m_surfaces) {
    total += surface->numberOfFacesToDraw();
  }
  return total;
}

QVector<int> CrystalSurfaceHandler::generateInternalFragment(
    DeprecatedCrystal *crystal, int surfaceIndex, bool selectFragment) const {
  Surface *sourceSurface = surfaceFromIndex(surfaceIndex);
  if (!sourceSurface)
    return {};
  Q_ASSERT(sourceSurface->isHirshfeldBased());

  QVector<int> diAtoms;

  foreach (AtomId atomId, sourceSurface->diAtoms()) {
    int atomIndex = crystal->generateFragmentFromAtomIdAssociatedWithASurface(
        sourceSurface, atomId);
    diAtoms << atomIndex;
    if (selectFragment) {
      crystal->setSelectStatusForAllAtoms(false);
      crystal->setSelectStatusForFragment(crystal->_fragmentForAtom[atomIndex],
                                          true);
    }
  }
  return diAtoms;
}

void CrystalSurfaceHandler::generateExternalFragmentForSurface(
    DeprecatedCrystal *crystal, int surfaceIndex, int faceIndex,
    bool selectFragment) const {
  Surface *sourceSurface = surfaceFromIndex(surfaceIndex);
  if (!sourceSurface)
    return;
  Q_ASSERT(sourceSurface->isHirshfeldBased());

  int atomIndex = crystal->generateFragmentFromAtomIdAssociatedWithASurface(
      sourceSurface, sourceSurface->outsideAtomIdForFace(faceIndex));
  if (selectFragment) {
    crystal->setSelectStatusForAllAtoms(false);
    crystal->setSelectStatusForFragment(crystal->_fragmentForAtom[atomIndex],
                                        true);
  }
}

QVector<int> CrystalSurfaceHandler::generateExternalFragmentsForSurface(
    DeprecatedCrystal *crystal, int surfaceIndex, bool selectFragments) const {
  Surface *sourceSurface = surfaceFromIndex(surfaceIndex);
  if (!sourceSurface)
    return {};
  Q_ASSERT(sourceSurface->isHirshfeldBased());

  QVector<int> deAtoms;

  crystal->setSelectStatusForAllAtoms(false);

  foreach (AtomId atomId, sourceSurface->deAtoms()) {
    int atomIndex = crystal->generateFragmentFromAtomIdAssociatedWithASurface(
        sourceSurface, atomId);
    deAtoms << atomIndex;
    if (selectFragments) {
      crystal->setSelectStatusForFragment(crystal->_fragmentForAtom[atomIndex],
                                          true);
    }
  }
  return deAtoms;
}

bool CrystalSurfaceHandler::setCurrentSurfaceIndex(int surfaceIndex) {
  if (surfaceIndex == -1 || surfaceIndex == m_currentSurfaceIndex) {
    return false;
  }
  Q_ASSERT(surfaceIndex < numberOfSurfaces());
  m_currentSurfaceIndex = surfaceIndex;
  return true;
}

void CrystalSurfaceHandler::resetCurrentSurfaceIndex() {
  m_currentSurfaceIndex = -1;
}

void CrystalSurfaceHandler::toggleSurfaceVisibility(int surfaceIndex) {
  Surface *surface = surfaceFromIndex(surfaceIndex);
  if (surface) {
    surface->toggleVisible();
    bool visible = surface->isVisible();
    if (surface->isParent()) {
      for (Surface *s : surface->clones()) {
        s->setVisible(visible);
      }
    }
  }
}

void CrystalSurfaceHandler::setSurfaceVisibility(int surfaceIndex,
                                                 bool visible) {
  Surface *surface = surfaceFromIndex(surfaceIndex);
  if (surface) {
    surface->setVisible(visible);
    if (surface->isParent()) {
      for (Surface *s : surface->clones()) {
        s->setVisible(visible);
      }
    }
  }
}

void CrystalSurfaceHandler::deleteCurrentSurface() {
  Surface *surface = currentSurface();

  if (surface->isParent()) {
    foreach (Surface *s, surface->clones()) {
      deleteSurface(s);
    }
  }

  // delete potential parent last
  deleteSurface(surface);
}

void CrystalSurfaceHandler::setAllSurfaceVisibilities(bool visible) {
  for (Surface *surface : std::as_const(m_surfaces)) {
    surface->setVisible(visible);
  }
  emit surfaceVisibilitiesChanged();
}

QStringList CrystalSurfaceHandler::surfaceTitles() const {
  QStringList surfaceTitles;
  for (const Surface *surface : std::as_const(m_surfaces)) {
    QString title;

    if (surface->isParent()) {
      QString name = surface->surfaceName();
      QString description = surface->surfaceDescription();
      if (!description.isEmpty()) {
        title = name + " [" + description + "]";
      }
    } else { // symmetry clones
      title = surface->symmetryDescription();
    }

    surfaceTitles << title;
  }
  return surfaceTitles;
}

QVector<bool> CrystalSurfaceHandler::surfaceVisibilities() const {
  QVector<bool> result;
  for (const Surface *surface : m_surfaces) {
    result.append(surface->isVisible());
  }
  return result;
}

QVector<QVector3D> CrystalSurfaceHandler::surfaceCentroids() const {
  QVector<QVector3D> result;
  for (const Surface *surface : m_surfaces) {
    result.append(surface->centroid());
  }
  return result;
}

bool CrystalSurfaceHandler::hasVisibleSurface() const {
  for (const Surface *surface : m_surfaces) {
    if (surface->isVisible())
      return true;
  }
  return false;
}

bool CrystalSurfaceHandler::hasHiddenSurface() const {
  for (const Surface *surface : m_surfaces) {
    if (!surface->isVisible())
      return true;
  }
  return false;
}

int CrystalSurfaceHandler::numberOfVisibleSurfaces() const {
  int n = 0;
  for (const Surface *surface : m_surfaces) {
    // Need to account for:
    // 1. Parent is never drawn so 'n' can only increases for child surfaces.
    // 2. Even if the child is visible, it still may not be visible depending on
    // the parents visibility status.
    // 3. The child has be visible
    bool cond1 = !surface->isParent();
    bool cond2 = surface->parent()->isVisible();
    bool cond3 = surface->isVisible();

    if (cond1 && cond2 && cond3) {
      n++;
    }
  }
  return n;
}

int CrystalSurfaceHandler::equivalentSurfaceIndex(
    const DeprecatedCrystal &crystal, const JobParameters &jobParams) const {
  int result = -1; // -1 signals not found

  for (int i = 0; i < numberOfSurfaces(); ++i) {
    const JobParameters &existingSurfaceJobParams =
        m_surfaces[i]->jobParameters();

    bool paramsEquivalent = jobParams.equivalentTo(existingSurfaceJobParams);
    bool fragmentsEquivalent = true;
    if (jobParams.surfaceType != IsosurfaceDetails::Type::CrystalVoid) {
      fragmentsEquivalent = crystal.fragmentAtomsAreSymmetryRelated(
          existingSurfaceJobParams.atoms, jobParams.atoms);
    }

    if (paramsEquivalent && fragmentsEquivalent) {
      result = i;
      break;
    }
  }
  return result;
}

int CrystalSurfaceHandler::
    indexOfPropertyEquivalentToRequestedPropertyForSurface(
        int surfaceIndex, const JobParameters &jobParams) const {
  const Surface *surface = surfaceFromIndex(surfaceIndex);
  if (!surface)
    return false;

  QVector<IsosurfacePropertyDetails::Type> surfTypes =
      surface->listOfPropertyTypes();

  return surfTypes.indexOf(
      jobParams.requestedPropertyType); // returns -1 if not found
}

void CrystalSurfaceHandler::deleteSurface(Surface *surface) {
  bool found = m_surfaces.removeOne(surface);
  Q_ASSERT(found);

  surface->reportDeletionToParent(); // tell parent we are gonna be deleted if
                                     // it's a child
  delete surface;

  if (m_currentSurfaceIndex == m_surfaces.size()) {
    m_currentSurfaceIndex -= 1;
  }

  if (m_surfaces.size() == 0) {
    m_currentSurfaceIndex = -1;
  }
}

Surface *
CrystalSurfaceHandler::cloneSurface(const Surface &parentSurface) const {
  Surface *surface = new Surface(parentSurface);
  surface->cloneInit(&parentSurface);
  return surface;
}

Surface *CrystalSurfaceHandler::cloneSurfaceWithCellShift(
    const DeprecatedCrystal &crystal, const Surface &source,
    const Vector3q &shift) const {
  const SymopId SYMOP_ID = 0; // always identity when cell shifting

  Surface *surface = nullptr;

  CrystalSymops crystalSymops;
  crystalSymops[SYMOP_ID] = shift;

  // has surface already been cloned for fragment?
  if (!cloneAlreadyExists(crystal, source, crystalSymops)) {
    surface = new Surface(source);
    surface->symmetryTransform(&source, crystal.spaceGroup(),
                               crystal.unitCell(), SYMOP_ID,
                               crystalSymops[SYMOP_ID]);
  } else {
    // If the surface already exists, ensure it's visible to the user some
    // feedback
    // sourceSurface->setVisible(true);
    existingClone(crystal, source, crystalSymops)->setVisible(true);
  }
  return surface;
}

Surface *
CrystalSurfaceHandler::cloneSurfaceForFragment(const DeprecatedCrystal &crystal,
                                               const Surface &source,
                                               int fragmentIndex) const {
  Surface *surface = nullptr;

  CrystalSymops crystalSymops =
      crystal.calculateCrystalSymops(&source, fragmentIndex);
  if (crystalSymops.size() > 0) { // Can we clone the surface for this fragment?
    // has surface already been cloned for fragment?
    if (!cloneAlreadyExists(crystal, source, crystalSymops)) {
      surface = new Surface(source);
      SymopId symopId = crystalSymops.keys()[0];
      Vector3q shift = crystalSymops[symopId];
      surface->symmetryTransform(&source, crystal.spaceGroup(),
                                 crystal.unitCell(), symopId, shift);
    } else {
      // If the surface already exists, ensure it's visible to the user some
      // feedback
      // sourceSurface->setVisible(true); // this should be the parent
      existingClone(crystal, source, crystalSymops)->setVisible(true);
    }
  }
  return surface;
}

bool CrystalSurfaceHandler::cloneAlreadyExists(
    const DeprecatedCrystal &crystal, const Surface &surface,
    const CrystalSymops &crystalSymops) const {
  return existingClone(crystal, surface, crystalSymops) != nullptr;
}

Surface *
CrystalSurfaceHandler::existingClone(const DeprecatedCrystal &crystal,
                                     const Surface &surface,
                                     const CrystalSymops &crystalSymops) const {
  for (Surface *clone : surface.clones()) {
    SymopId symopId = clone->symopId();
    if (crystalSymops.contains(symopId)) {
      QVector<float> relativeShift = clone->relativeShift();
      Vector3q shift;
      for (int i = 0; i < 3; ++i) {
        shift[i] = relativeShift[i];
      }
      if (crystal.isSameShift(crystalSymops[symopId], shift)) {
        return clone;
      }
    }
  }
  return nullptr; // not found
}

void CrystalSurfaceHandler::cloneCurrentSurfaceForSelection(
    DeprecatedCrystal &crystal) {
  Surface *surface = currentSurface();
  if (!surface)
    return;

  QVector<int> fragmentIndices = crystal.fragmentIndicesOfSelection();
  cloneCurrentSurfaceForFragmentList(crystal, fragmentIndices);
  crystal.setSelectStatusForAllAtoms(false);
}

void CrystalSurfaceHandler::cloneCurrentSurfaceForAllFragments(
    DeprecatedCrystal &crystal) {
  Surface *surface = currentSurface();
  if (!surface)
    return;

  QVector<int> fragmentIndices(crystal.numberOfFragments());
  std::iota(fragmentIndices.begin(), fragmentIndices.end(), 0);
  cloneCurrentSurfaceForFragmentList(crystal, fragmentIndices);
}

void CrystalSurfaceHandler::cloneCurrentSurfaceForFragmentList(
    DeprecatedCrystal &crystal, const QVector<int> &fragmentIndices) {
  if (fragmentIndices.size() == 0) {
    return; // nothing to do
  }

  int numAddedSurfaces = 0;
  const Surface *parentSurface = currentSurface()->parent();

  // Find the fragments that don't sit inside a (sub)surface of the current
  // surface
  // and clone the parent surface.

  for (const auto &fragmentIndex : fragmentIndices) {
    Surface *surface =
        cloneSurfaceForFragment(crystal, *parentSurface, fragmentIndex);

    if (surface) {
      // the surface might inherit invisibility from parent -- override this
      surface->setVisible(true);
      m_surfaces.insert(m_currentSurfaceIndex + 1, surface);
      numAddedSurfaces++;
    }
  }

  if (numAddedSurfaces > 0) {
    // Force parent to visible to ensure clones are visible too
    // parentSurface->setVisible(true);

    // We have to regenerate all the display lists to ensure correct naming of
    // atoms and bonds.
    emit surfacesChanged();
  }
}

void CrystalSurfaceHandler::cloneCurrentSurfaceWithCellShifts(
    DeprecatedCrystal &crystal, const QVector3D &cellLimits) {
  int numAddedSurfaces = 0;

  foreach (Vector3q cellShift, crystal.cellShiftsFromCellLimits(cellLimits)) {
    Surface *surface = cloneSurfaceWithCellShift(
        crystal, *currentSurface()->parent(), cellShift);
    if (surface) {
      m_surfaces.insert(m_currentSurfaceIndex + 1, surface);
      numAddedSurfaces++;
    }
  }

  // We have to regenerate all the display lists to ensure correct naming of
  // atoms and bonds.
  if (numAddedSurfaces > 0) {
    emit surfacesChanged();
  }
}

bool CrystalSurfaceHandler::loadSurfaceData(DeprecatedCrystal *crystal,
                                            const JobParameters &jobParams) {
  bool success = false;

  if (jobParams.onlyReadRequestedProperty) {
    SurfacePropertyProxy propertyProxy =
        SurfaceData::getRequestedPropertyData(jobParams);

    if (!propertyProxy.first.isEmpty()) {
      int surfaceIndex = equivalentSurfaceIndex(*crystal, jobParams);
      if (surfaceIndex != -1) {
        Surface *surface = surfaceFromIndex(surfaceIndex);
        Q_ASSERT(surface->isParent()); // The first matching surface must be a
                                       // parent since the clones are list
                                       // *after* their parent

        surface->addAdditionalProperty(propertyProxy.first,
                                       propertyProxy.second);
        emit newPropertyAddedToCurrentSurface();

        // Make the parent and at least first clone visible
        surface->setVisible(true);
        surface->clones()[0]->setVisible(true);
        emit surfaceVisibilitiesChanged();
        success = true;
      } else {
        // Didn't find surface to add property to.
        // This should happen because we only do onlyReadRequestedProperty when
        // there is a surface to add to
        // and there's no way it can disappear in the meantime.
        Q_ASSERT(false);
      }
    }
  } else { // read the whole surface in and then add it
    Surface *surface = SurfaceData::getData(jobParams);
    if (surface != nullptr) {

      if (surface->isHirshfeldBased()) {
        crystal->addFragmentPatchProperty(surface);
      }

      m_surfaces.append(surface);
      m_surfaces.append(cloneSurface(*surface));
      // We have to regenerate all the display lists to ensure correct naming of
      // atoms and bonds.
      emit surfacesChanged();
      success = true;
    }
  }

  return success;
}

void CrystalSurfaceHandler::rebuildSurfaceParentCloneRelationship() {
  int i = 0;
  while (i < m_surfaces.size()) {
    Surface *parent = m_surfaces[i];
    Q_ASSERT(parent->isParent());

    i += 1;
    Surface *surface = m_surfaces[i];
    while (!surface->isParent()) {
      surface->cloneInit(parent, true);
      ++i;
      if (i == m_surfaces.size()) {
        break;
      }
      surface = m_surfaces[i];
    }
  }
}

QPair<QVector3D, QVector3D>
CrystalSurfaceHandler::positionsOfMinDistanceFragSurface(
    const DeprecatedCrystal &crystal, int fragIndex, int surfaceIndex) const {
  const auto &atomsForFragment = crystal.m_atomsForFragment;
  const auto &atoms = crystal.atoms();
  if (atomsForFragment.size() > 0 && m_surfaces.size() > 0) {
    const Surface *surface = surfaceFromIndex(surfaceIndex);
    if (atomsForFragment[fragIndex].size() > 0) {
      int nearestAtom = atomsForFragment[fragIndex][0];
      const Atom &atom = atoms[nearestAtom];
      QVector3D nearestSurfacePosition =
          surface->posClosestToExternalPosition(atom.pos());
      float minDistance =
          (nearestSurfacePosition - atoms[nearestAtom].pos()).lengthSquared();
      for (int i = 1; i < atomsForFragment[fragIndex].size(); i++) {
        int atomIdx = atomsForFragment[fragIndex][i];
        const Atom &atom = atoms[atomIdx];
        QVector3D pos = surface->posClosestToExternalPosition(atom.pos());
        float distance = (pos - atom.pos()).lengthSquared();
        if (minDistance > distance) {
          minDistance = distance;
          nearestAtom = atomIdx;
          nearestSurfacePosition = pos;
        }
      }
      return QPair<QVector3D, QVector3D>(nearestSurfacePosition,
                                         atoms[nearestAtom].pos());
    }
  }
  return QPair<QVector3D, QVector3D>(QVector3D(0, 0, 0), QVector3D(0, 0, 0));
}

QPair<QVector3D, QVector3D>
CrystalSurfaceHandler::positionsOfMinDistanceSurfaceSurface(
    int surfaceIndex1, int surfaceIndex2) const {
  if (m_surfaces.size() > 0) {
    return m_surfaces[surfaceIndex1]->positionsOfMinimumDistance(
        m_surfaces[surfaceIndex2]);
  }
  return QPair<QVector3D, QVector3D>(QVector3D(0, 0, 0), QVector3D(0, 0, 0));
}

QPair<QVector3D, QVector3D>
CrystalSurfaceHandler::positionsOfMinDistancePosSurface(
    const QVector3D &pos, int surfaceIndex) const {
  if (m_surfaces.size() > 0) {
    return QPair<QVector3D, QVector3D>(
        pos, m_surfaces[surfaceIndex]->posClosestToExternalPosition(pos));
  }
  return QPair<QVector3D, QVector3D>(QVector3D(0, 0, 0), QVector3D(0, 0, 0));
}

QDataStream &operator<<(QDataStream &ds, const CrystalSurfaceHandler &handler) {

  quint32 nSurfaces = handler.numberOfSurfaces();
  ds << nSurfaces;
  for (quint32 i = 0; i < nSurfaces; ++i) {
    ds << *handler.m_surfaces[i];
  }
  ds << handler.m_currentSurfaceIndex;

  return ds;
}

QDataStream &operator>>(QDataStream &ds, CrystalSurfaceHandler &handler) {

  quint32 nSurfaces;
  ds >> nSurfaces;
  for (quint32 i = 0; i < nSurfaces; ++i) {
    Surface *surface = new Surface();
    ds >> *surface;
    handler.m_surfaces.append(surface);
  }

  ds >> handler.m_currentSurfaceIndex;

  return ds;
}
