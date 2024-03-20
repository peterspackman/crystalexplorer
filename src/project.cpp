#include <QMessageBox>
#include <QtDebug>

#include "ciffile.h"
#include "confirmationbox.h"
#include "crystaldata.h"
#include "dialoghtml.h"
#include "elementdata.h"
#include "project.h"
#include "version.h"
#include "xyzfile.h"

namespace CrystalNotification {
void subscribe(Project *project, DeprecatedCrystal *crystal) {
  if (!crystal)
    return;
  QObject::connect(crystal, &DeprecatedCrystal::atomsChanged, project,
                   &Project::atomSelectionChanged);
}

void unsubscribe(Project *project, DeprecatedCrystal *crystal) {
  if (!crystal)
    return;
  QObject::disconnect(crystal, &DeprecatedCrystal::atomsChanged, project,
                      &Project::atomSelectionChanged);
}
} // namespace CrystalNotification

namespace SurfaceNotification {
void subscribe(Project *project, CrystalSurfaceHandler *handler) {
  if (!handler)
    return;
  QObject::connect(handler,
                   &CrystalSurfaceHandler::newPropertyAddedToCurrentSurface,
                   project, &Project::newPropertyAddedToCurrentSurface);
  QObject::connect(handler, &CrystalSurfaceHandler::surfaceVisibilitiesChanged,
                   project, &Project::reportSurfaceVisibilitiesChanged);
}

void unsubscribe(Project *project, CrystalSurfaceHandler *handler) {
  if (!handler)
    return;
  QObject::disconnect(handler,
                      &CrystalSurfaceHandler::newPropertyAddedToCurrentSurface,
                      project, &Project::newPropertyAddedToCurrentSurface);
  QObject::disconnect(handler,
                      &CrystalSurfaceHandler::surfaceVisibilitiesChanged,
                      project, &Project::reportSurfaceVisibilitiesChanged);
}
} // namespace SurfaceNotification

namespace SceneNotification {
void subscribe(Project *project) {
  Scene *scene = project->currentScene();
  if (!scene)
    return;
  QObject::connect(scene, &Scene::surfaceSelected, project,
                   &Project::surfaceSelected);
  QObject::connect(scene, &Scene::currentSurfaceFaceSelected, project,
                   &Project::currentSurfaceFaceSelected);
  QObject::connect(scene, &Scene::contactAtomExpanded, project,
                   &Project::currentSceneChanged);
  QObject::connect(scene, &Scene::viewChanged, project,
                   &Project::currentCrystalViewChanged);
  QObject::connect(scene, &Scene::sceneContentsChanged, project,
                   &Project::currentSceneChanged);
  QObject::connect(scene, &Scene::atomSelectionChanged, project,
                   &Project::atomSelectionChanged);
  CrystalNotification::subscribe(project, scene->crystal());
  SurfaceNotification::subscribe(project, scene->surfaceHandler());
}

void unsubscribe(Project *project) {
  Scene *scene = project->currentScene();
  if (!scene)
    return;
  QObject::disconnect(scene, &Scene::surfaceSelected, project,
                      &Project::surfaceSelected);
  QObject::disconnect(scene, &Scene::currentSurfaceFaceSelected, project,
                      &Project::currentSurfaceFaceSelected);
  QObject::disconnect(scene, &Scene::contactAtomExpanded, project,
                      &Project::currentSceneChanged);
  QObject::disconnect(scene, &Scene::viewChanged, project,
                      &Project::currentCrystalViewChanged);
  QObject::disconnect(scene, &Scene::sceneContentsChanged, project,
                      &Project::currentSceneChanged);
  QObject::disconnect(scene, &Scene::atomSelectionChanged, project,
                      &Project::atomSelectionChanged);
  CrystalNotification::unsubscribe(project, scene->crystal());
  SurfaceNotification::unsubscribe(project, scene->surfaceHandler());
}
} // namespace SceneNotification

Project::Project(QObject *parent) : QObject(parent) {
  init();
  initConnections();
}

Project::~Project() { deleteAllCrystals(); }

void Project::init() {
  m_currentSceneIndex = -1;
  m_previousSceneIndex = -1;
  _saveFilename = "";
  m_haveUnsavedChanges = false;
}

void Project::initConnections() {}

void Project::reset() { removeAllCrystals(); }

void Project::removeAllCrystals() {
  init();
  deleteAllCrystals();
  emit projectChanged(this);
  emit currentCrystalChanged(this);
  emit currentCrystalSurfacesChanged(this);
}

void Project::removeCurrentCrystal() {
  if (m_scenes.size() == 1) {
    removeAllCrystals();
  } else {
    tidyUpOutgoingScene();
    deleteCurrentCrystal();
    connectUpCurrentScene();
    setUnsavedChangesExists();
    emit currentCrystalChanged(this);
    if (currentScene()->numberOfSurfaces() == 0) {
      emit currentCrystalHasNoSurfaces();
    }
  }
}

bool Project::loadCrystalDataTonto(const QString &cxs, const QString &cif) {
  QVector<Scene *> crystalList = crystaldata::loadCrystalsFromTontoOutput(cxs, cif);
  if (crystalList.size() > 0) {
    int newCrystalIndex = m_scenes.size();
    m_scenes.append(crystalList);
    setUnsavedChangesExists();
    setCurrentCrystal(newCrystalIndex);
    return true;
  }
  return false;
}

bool Project::loadCrystalData(const JobParameters &jobParams) {
    QString cxs = jobParams.outputFilename;
    QString cif = jobParams.inputFilename;
  QVector<Scene *> crystalList = crystaldata::loadCrystalsFromTontoOutput(cxs, cif);
  if (crystalList.size() > 0) {
    int newCrystalIndex = m_scenes.size();
    m_scenes.append(crystalList);
    setUnsavedChangesExists();
    setCurrentCrystal(newCrystalIndex);
    return true;
  }
  return false;
}

bool Project::loadSurfaceData(const JobParameters &jobParams) {
  if (currentScene() && currentScene()->loadSurfaceData(jobParams)) {
    currentScene()->setSelectStatusForAllAtoms(
        false); // Now we have a surface, clear the currently selected atoms
    setUnsavedChangesExists();
    emit currentCrystalSurfacesChanged(this);
    return true;
  }
  return false;
}

Scene *Project::currentScene() {
  if (m_currentSceneIndex < m_scenes.size() && m_currentSceneIndex > -1) {
    return m_scenes[m_currentSceneIndex];
  }
  return nullptr;
}

const Scene *Project::currentScene() const {
  if (m_currentSceneIndex < m_scenes.size() && m_currentSceneIndex > -1) {
    return m_scenes[m_currentSceneIndex];
  }
  return nullptr;
}

Scene *Project::previousCrystal() {
  Q_ASSERT(m_previousSceneIndex < m_scenes.size());

  if (m_previousSceneIndex > -1) {
    return m_scenes[m_previousSceneIndex];
  }
  return nullptr;
}

void Project::setCurrentCrystal(int crystalIndex) {
  setCurrentCrystal(crystalIndex, false);
}

void Project::setCurrentCrystal(int crystalIndex, bool refresh) {
  if (crystalIndex == -1) {
    return;
  }
  if (!refresh && crystalIndex == m_currentSceneIndex) {
    return;
  }
  setCurrentCrystalUnconditionally(crystalIndex);
  setUnsavedChangesExists();
  emit currentCrystalChanged(this);
  if (!currentScene()->hasSurface()) {
    emit currentCrystalHasNoSurfaces();
  }
}

void Project::setCurrentCrystalUnconditionally(int crystalIndex) {
  tidyUpOutgoingScene();

  m_previousSceneIndex = m_currentSceneIndex;
  Q_ASSERT(crystalIndex < m_scenes.size());
  m_currentSceneIndex = crystalIndex;

  connectUpCurrentScene();
}

void Project::tidyUpOutgoingScene() {
  SceneNotification::unsubscribe(this);
  if (!currentScene()) {
    return;
  }

  DeprecatedCrystal *crystal = currentScene()->crystal();
  if (crystal) {
    // Set current surface index to -1
    // This means that when we come back to this crystal in the future
    // the Project::setCurrentSurface will always emit a signal because
    // 'changed' will always be true.
    currentScene()->resetCurrentSurface();

    // turn off contact atoms when moving to a different crystal
    if (currentScene()->crystal()->hasAnyVdwContactAtoms()) {
      removeContactAtoms();
      emit contactAtomsTurnedOff();
    }
  }
}

void Project::connectUpCurrentScene() { SceneNotification::subscribe(this); }

QList<ScenePeriodicity> Project::scenePeriodicities() const {
  QList<ScenePeriodicity> result;
  for (const auto *scene : std::as_const(m_scenes)) {
    result.append(scene->periodicity());
  }
  return result;
}

QStringList Project::sceneTitles() {
  QStringList result;
  for (const auto *scene : std::as_const(m_scenes)) {
    result.append(scene->title());
  }
  return result;
}

void Project::toggleShowCurrentSurfaceInterior(bool show) {
  Surface *surface = currentScene()->surfaceHandler()->currentSurface();
  if (surface) {
    surface->setShowInterior(show);
    setUnsavedChangesExists();
    currentScene()->setNeedsUpdate();
    emit currentSurfacePropertyChanged();
  }
}

void Project::toggleShowSurfaceInteriors(bool show) {
  foreach (Scene *scene, m_scenes) {
    scene->setShowSurfaceInteriors(show);
  }
  setUnsavedChangesExists();
  currentScene()->setNeedsUpdate();
  emit currentSurfacePropertyChanged();
}

void Project::cycleDisorderHighlighting() {
  if (currentScene()) {
    currentScene()->cycleDisorderHighlighting();
    emit currentSceneChanged();
  }
}

void Project::cycleEnergyFramework(bool cycleBackwards) {
  Scene *crystal = currentScene();
  if (crystal && crystal->crystal()->hasInteractionEnergies()) {
    crystal->cycleEnergyFramework(cycleBackwards);
    crystal->crystal()->updateEnergyInfo(crystal->currentFramework());
    emit currentSceneChanged();
  }
}

void Project::updateEnergyFramework() {
  Scene *crystal = currentScene();
  if (crystal && crystal->crystal()->hasInteractionEnergies()) {
    crystal->crystal()->updateEnergyInfo(crystal->currentFramework());
    emit currentSceneChanged();
  }
}

void Project::turnOffEnergyFramework() {
  Scene *crystal = currentScene();
  if (crystal) {
    crystal->turnOffEnergyFramework();
    emit currentSceneChanged();
  }
}

void Project::updateEnergyTheoryForEnergyFramework(EnergyTheory theory) {
  Scene *crystal = currentScene();
  if (crystal) {
    crystal->crystal()->setEnergyTheoryForEnergyFramework(
        theory, crystal->currentFramework());
    emit currentSceneChanged();
  }
}

void Project::toggleVisibilityOfSurface(int surfaceIndex) {
  currentScene()->toggleVisibilityOfSurface(surfaceIndex);
  setUnsavedChangesExists();
  emit currentSurfaceVisibilityChanged(this);
}

void Project::hideSurface(int surfaceIndex) {
  Q_ASSERT(currentScene());
  currentScene()->hideSurface(surfaceIndex);
  setUnsavedChangesExists();
  emit surfaceVisibilitiesChanged(this);
}

void Project::setCurrentSurface(int surfaceIndex) {
    auto * scene = currentScene();
    if(!scene) return;
  bool changed = scene->setCurrentSurfaceIndex(surfaceIndex);
  if (changed) {
    setUnsavedChangesExists();
    emit currentSurfaceChanged(currentScene()->currentSurface());
  }
}

void Project::setCurrentPropertyForCurrentSurface(int property) {
  Surface *surface = currentScene()->currentSurface();

  bool propertyChanged = surface->setCurrentProperty(property);

  if (propertyChanged) {
    setUnsavedChangesExists();
    emit currentPropertyChanged(surface->currentProperty());
  }
}

void Project::updatePropertyRangeForCurrentSurface(float minValue,
                                                   float maxValue) {
  Surface *currentSurface = currentScene()->currentSurface();

  if (currentSurface) {
    currentSurface->setCurrentPropertyRange(minValue, maxValue);
    setUnsavedChangesExists();
    currentScene()->setNeedsUpdate();
    emit currentSurfacePropertyChanged();
  }
}

void Project::updateCurrentCrystalContents() {
  if (!currentScene())
    return;
  currentScene()->updateForPreferencesChange();
  setUnsavedChangesExists();
  emit currentSceneChanged();
}

void Project::updateAllCrystalsForChangeInElementData() {
  bool shouldEmit = false;
  for (Scene *scene : std::as_const(m_scenes)) {
    scene->setNeedsUpdate();
    shouldEmit = true;
  }
  if (shouldEmit) {
    setUnsavedChangesExists();
    emit currentSceneChanged();
  }
}

void Project::setSurfaceTransparency(bool transparency) {
  if (currentScene()->hasSurface()) {
    currentScene()->currentSurface()->setTransparent(transparency);
    setUnsavedChangesExists();
    emit currentSurfaceTransparencyChanged();
  }
}

void Project::removeContactAtoms() {
  Q_ASSERT(currentScene());
  currentScene()->crystal()->removeVdwContactAtoms();
  emit currentSceneChanged();
}

void Project::generateCells(QPair<QVector3D, QVector3D> cellLimits) {
  Q_ASSERT(currentScene());

  currentScene()->generateCells(cellLimits);
  setUnsavedChangesExists();
  emit currentSceneChanged();
}

void Project::deleteAllCrystals() {
  foreach (Scene *crystal, m_scenes) {
    delete crystal;
  }
  m_scenes.clear();
}

void Project::deleteCurrentCrystal() {
  Scene *crystal = m_scenes.takeAt(m_currentSceneIndex);
  delete crystal;
  if (m_scenes.size() == 0) {
    m_currentSceneIndex = -1;
    m_previousSceneIndex = -1;
  } else {
    if (m_previousSceneIndex > m_currentSceneIndex) {
      --m_previousSceneIndex;
    }
    if (m_currentSceneIndex == m_scenes.size()) {
      --m_currentSceneIndex;
    }
  }
}

void Project::deleteCurrentSurface() {
  currentScene()->deleteCurrentSurface();

  setUnsavedChangesExists();

  emit currentCrystalSurfacesChanged(this);
  emit currentSurfaceChanged(currentScene()->currentSurface());
}

void Project::confirmAndDeleteSurface(int) {
  Q_ASSERT(currentScene());

  QString description =
      currentScene()->selectedSurface()->symmetryDescription();
  if (ConfirmationBox::confirmSurfaceDeletion(false, description)) {
    currentScene()->deleteSelectedSurface();
    setUnsavedChangesExists();
    emit currentCrystalSurfacesChanged(this);
  }
}

bool Project::saveToFile(QString filename) {
  bool success = false;

  QFile outFile(filename);
  if (outFile.open(QIODevice::WriteOnly)) {
    QDataStream ds(&outFile);
    ds << *this;
    outFile.close();
    _saveFilename = filename;
    m_haveUnsavedChanges = false;
    success = true;
    emit projectSaved();
  }
  return success;
}

bool Project::loadChemicalStructureFromXyzFile(const QString &filename) {
  XYZFile xyzReader;
  bool success = xyzReader.readFromFile(filename);
  if (!success)
    return false;

  Scene *scene = new Scene(xyzReader);
  scene->setTitle(QFileInfo(filename).baseName());
  int newSceneIndex = m_scenes.size();
  m_scenes.append(scene);
  setUnsavedChangesExists();
  setCurrentCrystal(newSceneIndex);
  return true;
}

bool Project::loadCrystalStructuresFromCifFile(const QString &filename) {
  CifFile cifReader;
  bool success = cifReader.readFromFile(filename);
  if (!success)
    return false;

  int newSceneIndex = -1;
  for (int i = 0; i < cifReader.numberOfCrystals(); i++) {
    CrystalStructure *tmp = new CrystalStructure();
    tmp->setOccCrystal(cifReader.getCrystalStructure(i));
    Scene *scene = new Scene(tmp);
    scene->setTitle(QFileInfo(filename).baseName());
    if (i == 0) {
      newSceneIndex = m_scenes.size();
      m_scenes.append(scene);
    }
  }

  if (newSceneIndex > -1) {
    setUnsavedChangesExists();
    setCurrentCrystal(newSceneIndex);
    emit currentCrystalChanged(this);
  }

  return true;
}

bool Project::loadFromFile(QString filename) {
  bool success = false;

  QFile inFile(filename);
  if (inFile.open(QIODevice::ReadOnly)) {
    QDataStream ds(&inFile);
    reset();
    ds >> *this;
    inFile.close();
    if (m_currentSceneIndex > -1) {
      _saveFilename = filename;

      // We read in a .cxp file which sets _previousCrystal and _currentCrystal
      // to
      // how it was when the project was saved but...
      // we need to pretend the current crystal has changed
      // (i) from the projects points of view so it sets up all the connections
      // for the current crystal etc.
      // (ii) from the point of the view of the rest of the program (glwindow,
      // crystal controller) so they
      //      show the correct data for the current crystal.
      // In order to achieve (i) we have to lie about the index of the current
      // crystal and pass
      // the index of crystal we want to be current to setCurrentCrystal()
      // In order to achieve (ii) is simply a matter of emitting the
      // currentCrystalChanged signal
      setCurrentCrystal(m_currentSceneIndex, true);
      emit currentCrystalChanged(this);

      if (currentScene()->hasSurface()) {
        emit currentSurfaceChanged(currentScene()->currentSurface());
      }

      m_haveUnsavedChanges = false;
      emit projectChanged(this);

      success = true;
    }
  }
  return success;
}

void Project::completeFragmentsForCurrentCrystal() {
  if (!currentScene()) {
    return;
  }

  currentScene()->completeAllFragments();
  setUnsavedChangesExists();
  emit currentSceneChanged();
}

void Project::toggleUnitCellAxes(bool state) {
  if (currentScene()) {
    currentScene()->setUnitCellBoxVisible(state);
    setUnsavedChangesExists();
    emit currentSceneChanged();
  }
}

void Project::toggleMultipleUnitCellBoxes(bool state) {
  if (currentScene()) {
    currentScene()->enableMultipleUnitCellBoxes(state);
    setUnsavedChangesExists();
    emit currentSceneChanged();
  }
}

void Project::toggleAtomicLabels(bool state) {
  if (currentScene()) {
    currentScene()->setAtomicLabelsVisible(state);
    setUnsavedChangesExists();
    emit currentSceneChanged();
  }
}

void Project::toggleHydrogenAtoms(bool state) {
  if (currentScene()) {
    currentScene()->setShowHydrogens(state);
    emit currentSceneChanged();
  }
}

void Project::toggleSuppressedAtoms(bool state) {
  if (currentScene()) {
    currentScene()->setShowSuppressedAtoms(state);
    emit currentSceneChanged();
  }
}

bool Project::currentHasSelectedAtoms() const {
  if (currentScene()) {
    return currentScene()->hasSelectedAtoms();
  }
  return false;
}

bool Project::currentHasSurface() const {
  const Scene *scene = currentScene();
  return scene->hasSurface();
}

void Project::toggleCloseContacts(bool state) {
  if (currentScene()) {
    currentScene()->setShowCloseContacts(state);
    emit currentSceneChanged();
  }
}

void Project::toggleHydrogenBonds(bool state) {
  if (currentScene()) {
    currentScene()->setHydrogenBondsVisible(state);
    emit currentSceneChanged();
  }
}

void Project::toggleSurfaceCaps(bool state) {
  Surface *surface = currentScene()->currentSurface();
  if (surface != nullptr) {
    surface->setCapsVisible(state);
    setUnsavedChangesExists();
    emit currentSurfaceChanged(surface);
  }
}

bool Project::previouslySaved() { return (!_saveFilename.isEmpty()); }

QString Project::saveFilename() { return _saveFilename; }

void Project::updateNonePropertiesForAllCrystals() {
  foreach (Scene *crystal, m_scenes) {
    crystal->updateNoneProperties();
  }
  setUnsavedChangesExists();
  currentScene()->setNeedsUpdate();
  // This may not be true but force a redraw anyway
  emit currentSurfacePropertyChanged();
}

void Project::updateHydrogenBondsForCurrent(QString donor, QString acceptor,
                                            double distanceCriteria,
                                            bool includeIntraHBonds) {
  if (currentScene() != nullptr) {
    currentScene()->crystal()->updateHBondList(
        donor, acceptor, distanceCriteria, includeIntraHBonds);
    emit currentSceneChanged();
  }
}

void Project::toggleCC1(bool show) { toggleCloseContact(CC1_INDEX, show); }

void Project::toggleCC2(bool show) { toggleCloseContact(CC2_INDEX, show); }

void Project::toggleCC3(bool show) { toggleCloseContact(CC3_INDEX, show); }

void Project::toggleCloseContact(int ccIndex, bool show) {
  if (currentScene() != nullptr) {
    currentScene()->setCloseContactVisible(ccIndex, show);
    emit currentSceneChanged();
  }
}

void Project::updateCloseContactsForCurrent(int contactIndex, QString x,
                                            QString y,
                                            double distanceCriteria) {
  if (currentScene() != nullptr) {
    currentScene()->crystal()->updateCloseContactWithIndex(contactIndex, x, y,
                                                           distanceCriteria);
    emit currentSceneChanged();
  }
}

void Project::reportSurfaceVisibilitiesChanged() {
  setUnsavedChangesExists();
  emit surfaceVisibilitiesChanged(this);
}

void Project::removeIncompleteFragmentsForCurrentCrystal() {
  if (currentScene()) {
    currentScene()->crystal()->discardIncompleteFragments();
    setUnsavedChangesExists();
    emit currentSceneChanged();
  }
}

void Project::removeSelectedAtomsForCurrentCrystal() {
  if (currentScene()) {
    currentScene()->crystal()->discardSelectedAtoms();
    setUnsavedChangesExists();
    emit currentSceneChanged();
  }
}

void Project::resetCurrentCrystal() {
  Scene *scene = currentScene();
  if (scene) {
    scene->reset();
    setUnsavedChangesExists();
    emit currentCrystalReset();
  }
}

QString Project::projectFileVersion() const { return CX_VERSION; }

QString Project::projectFileCompatibilityVersion() const {
  return CX_VERSION;
}

void Project::setUnsavedChangesExists() {
  m_haveUnsavedChanges = true;
  emit projectChanged(this);
}

void Project::showAtomsWithinRadius(float radius,
                                    bool generateClusterForSelection) {
  if (!currentScene()) {
    return;
  }

  currentScene()->expandAtomsWithinRadius(radius, generateClusterForSelection);
  emit currentSceneChanged();
}

void Project::toggleAtomsForFingerprintSelectionFilter(bool show) {
  if (currentScene() && currentScene()->currentSurface()) {
    currentScene()->toggleAtomsForFingerprintSelectionFilter(show);
    // emit currentCrystalContentsChanged();
  } else {
    // No current surface!
    Q_ASSERT(false);
  }
}

void Project::suppressSelectedAtoms() {
  if (currentScene()) {
    currentScene()->suppressSelectedAtoms();
    setUnsavedChangesExists();
    emit currentSceneChanged();
  }
}

void Project::unsuppressSelectedAtoms() {
  if (currentScene()) {
    currentScene()->crystal()->unsuppressSelectedAtoms();
    setUnsavedChangesExists();
    emit currentSceneChanged();
  }
}

void Project::selectAllAtoms() {
  if (currentScene()) {
    currentScene()->crystal()->selectAllAtoms();
    emit currentSceneChanged();
  }
}

void Project::selectSuppressedAtoms() {
  if (currentScene()) {
    currentScene()->crystal()->selectAllSuppressedAtoms();
    emit currentSceneChanged();
  }
}

void Project::selectAtomsOutsideRadiusOfSelectedAtoms(float radius) {
  if (currentScene()) {
    currentScene()->selectAtomsOutsideRadiusOfSelectedAtoms(radius);
    emit currentSceneChanged();
  }
}

void Project::selectAtomsInsideCurrentSurface() {
  if (currentScene() && currentScene()->currentSurface()) {
    currentScene()->selectAtomsSeparatedBySurface(true);
    emit currentSceneChanged();
  }
}

void Project::selectAtomsOutsideCurrentSurface() {
  if (currentScene() && currentScene()->currentSurface()) {
    currentScene()->selectAtomsSeparatedBySurface(false);
    emit currentSceneChanged();
  }
}

void Project::invertSelection() {
  if (currentScene()) {
    currentScene()->crystal()->invertSelection();
    emit currentSceneChanged();
  }
}

void Project::removeAllMeasurements() {
  foreach (Scene *scene, m_scenes) {
    scene->removeAllMeasurements();
  }
}

void Project::addMonomerEnergyToCurrent(const MonomerEnergy &m) {
  if (currentScene()) {
    currentScene()->crystal()->addMonomerEnergy(m);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &datastream, const Project &project) {
    qDebug() << "pos before" << datastream.device()->pos();
  datastream << project.projectFileVersion();
    qDebug() << "pos after" << datastream.device()->pos();

  ElementData::writeToStream(datastream);
  qDebug() << "Wrote element data";

  qsizetype nCrystals = project.m_scenes.size();
  qDebug() << "Writing " << nCrystals << "crystals";
  datastream << nCrystals;
  for (qsizetype i = 0; i < nCrystals; ++i) {
    datastream << *project.m_scenes[i];
  }
  datastream << project.m_currentSceneIndex;
  datastream << project.m_previousSceneIndex;

  return datastream;
}

QDataStream &operator>>(QDataStream &ds, Project &project) {
  // It's feasible that the contents of a project file changes over time
  // It's too difficult at present to provide the ability to read numerous
  // versions of project files. The (slightly brutal) solution is to only
  // allow project files created by a particular version to load them.
  QString projectFileVersion;
  ds >> projectFileVersion;

  if (projectFileVersion.toFloat() <
      project.projectFileCompatibilityVersion().toFloat()) {
    QString title = "CrystalExplorer Error";
    QString msg =
        "Unable to open project file (Version: " + projectFileVersion +
        ")\n\nOnly CrystalExplorer " +
        project.projectFileCompatibilityVersion() +
        " or newer projects can be opened.";
    QMessageBox::critical(0, title, msg);
    return ds;
  }

  ElementData::readFromStream(ds);

  project.deleteAllCrystals(); // Before reading in new crystals lets clear out
                               // old ones

  qsizetype nCrystals{0};

  ds >> nCrystals;

  for (qsizetype i = 0; i < nCrystals; ++i) {
    Scene *crystal = new Scene();
    ds >> *crystal;
    project.m_scenes.append(crystal);
  }

  ds >> project.m_currentSceneIndex;
  ds >> project.m_previousSceneIndex;


  Q_ASSERT(project.m_currentSceneIndex >= 0 &&
           project.m_currentSceneIndex < project.m_scenes.size());
  Q_ASSERT(project.m_previousSceneIndex >= -1 &&
           project.m_previousSceneIndex < project.m_scenes.size());

  // TODO fix title
  return ds;
}
