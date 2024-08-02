#include <QMessageBox>
#include <QtDebug>

#include "dynamicstructure.h"
#include "ciffile.h"
#include "confirmationbox.h"
#include "dialoghtml.h"
#include "elementdata.h"
#include "pdbfile.h"
#include "project.h"
#include "version.h"
#include "xyzfile.h"
#include "globals.h"
#include "crystalclear.h"

namespace SceneNotification {
void subscribe(Project *project) {
  Scene *scene = project->currentScene();
  if (!scene)
    return;
  QObject::connect(scene, &Scene::contactAtomExpanded, project,
                   &Project::currentSceneChanged);
  QObject::connect(scene, &Scene::viewChanged, project,
                   &Project::currentCrystalViewChanged);
  QObject::connect(scene, &Scene::sceneContentsChanged, project,
                   &Project::currentSceneChanged);
  QObject::connect(scene, &Scene::atomSelectionChanged, project,
                   &Project::atomSelectionChanged);
  QObject::connect(scene, &Scene::structureChanged, project,
                   &Project::structureChanged);
  QObject::connect(scene, &Scene::clickedSurface, project,
                   &Project::clickedSurface);
  QObject::connect(scene, &Scene::clickedSurfacePropertyValue, project,
                   &Project::clickedSurfacePropertyValue);
}

void unsubscribe(Project *project) {
  Scene *scene = project->currentScene();
  if (!scene)
    return;
  QObject::disconnect(scene, &Scene::contactAtomExpanded, project,
                      &Project::currentSceneChanged);
  QObject::disconnect(scene, &Scene::viewChanged, project,
                      &Project::currentCrystalViewChanged);
  QObject::disconnect(scene, &Scene::sceneContentsChanged, project,
                      &Project::currentSceneChanged);
  QObject::disconnect(scene, &Scene::atomSelectionChanged, project,
                      &Project::atomSelectionChanged);
  QObject::disconnect(scene, &Scene::structureChanged, project,
                      &Project::structureChanged);
  QObject::disconnect(scene, &Scene::clickedSurface, project,
                      &Project::clickedSurface);
  QObject::disconnect(scene, &Scene::clickedSurfacePropertyValue, project,
                      &Project::clickedSurfacePropertyValue);
}
} // namespace SceneNotification

Project::Project(QObject *parent) : QAbstractItemModel(parent) {
  init();
  initConnections();
}

Project::~Project() { deleteAllCrystals(); }

void Project::init() {
  m_sceneKindIcons[ScenePeriodicity::ZeroDimensions] =
      QIcon(":/images/molecule_icon.png");
  m_sceneKindIcons[ScenePeriodicity::ThreeDimensions] =
      QIcon(":/images/crystal_icon.png");

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
  emit selectedSceneChanged(m_currentSceneIndex);
}

void Project::removeCurrentCrystal() {
  if (m_scenes.size() == 1) {
    removeAllCrystals();
  } else {
    tidyUpOutgoingScene();
    deleteCurrentCrystal();
    connectUpCurrentScene();
    setUnsavedChangesExists();
    emit selectedSceneChanged(m_currentSceneIndex);
  }
}

ChemicalStructure *Project::currentStructure() {
  auto *scene = currentScene();
  if (scene) {
    return scene->chemicalStructure();
  }
  return nullptr;
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
  emit selectedSceneChanged(m_currentSceneIndex);
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
}

void Project::connectUpCurrentScene() { SceneNotification::subscribe(this); }

QStringList Project::sceneTitles() {
  QStringList result;
  for (const auto *scene : std::as_const(m_scenes)) {
    result.append(scene->title());
  }
  return result;
}

void Project::cycleDisorderHighlighting() {
  if (currentScene()) {
    currentScene()->cycleDisorderHighlighting();
    emit currentSceneChanged();
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

  TrajFile trajReader;
  bool success = trajReader.readFromFile(filename);
  if (!success) {
    qDebug() << "Failed reading from" << filename;
    return false;
  }

  const auto &frames = trajReader.frames();
  qDebug() << "Read" << trajReader.frames().size() << "frames" << success;
  if(frames.size() == 0) return false;
  if(frames.size() == 1) {
    const auto &xyzReader = frames[0];
    auto * structure = new ChemicalStructure();
    structure->setObjectName(xyzReader.getComment());
    structure->setAtoms(xyzReader.getAtomSymbols(), xyzReader.getAtomPositions());
    structure->updateBondGraph();
    Scene *scene = new Scene(structure);
    scene->setTitle(QFileInfo(filename).baseName());
    int position = m_scenes.size();
    beginInsertRows(QModelIndex(), position, position);
    m_scenes.append(scene);
    endInsertRows();
    setUnsavedChangesExists();
    setCurrentCrystal(position);
  }
  else {
    auto * structure = new DynamicStructure();
    int frameNumber = 1;
    for(const auto &frame: frames) {
      auto * s = new ChemicalStructure();
      s->setObjectName(frame.getComment());
      s->setAtoms(frame.getAtomSymbols(), frame.getAtomPositions());
      s->setProperty("frame", frameNumber++);
      s->updateBondGraph();
      structure->addFrame(s);
    }
    Scene *scene = new Scene(structure);
    scene->setTitle(QFileInfo(filename).baseName());
    int position = m_scenes.size();
    beginInsertRows(QModelIndex(), position, position);
    m_scenes.append(scene);
    endInsertRows();
    setUnsavedChangesExists();
    setCurrentCrystal(position);
  }
  return true;
}

bool Project::loadCrystalStructuresFromPdbFile(const QString &filename) {
  PdbFile pdbReader;
  bool success = pdbReader.readFromFile(filename);
  if (!success)
    return false;

  int position = -1;
  for (int i = 0; i < pdbReader.numberOfCrystals(); i++) {
    CrystalStructure *tmp = new CrystalStructure();
    tmp->setOccCrystal(pdbReader.getCrystalStructure(i));
    // tmp->setFileContents(pdbReader.getCrystalCifContents(i));
    // tmp->setName(pdbReader.getCrystalName(i));
    Scene *scene = new Scene(tmp);
    scene->setTitle(QFileInfo(filename).baseName());
    if (i == 0) {
      position = m_scenes.size();
      beginInsertRows(QModelIndex(), position, position);
      m_scenes.append(scene);
      endInsertRows();
    }
  }

  if (position > -1) {
    setUnsavedChangesExists();
    setCurrentCrystal(position);
    emit selectedSceneChanged(position);
  }

  return true;
}

bool Project::loadCrystalClearJson(const QString &filename) {
  CrystalStructure *crystal = io::loadCrystalClearJson(filename);
  Scene *scene = new Scene(crystal);
  scene->setTitle(QFileInfo(filename).baseName());
  int position = m_scenes.size();
  beginInsertRows(QModelIndex(), position, position);
  m_scenes.append(scene);
  endInsertRows();

  if (position > -1) {
    setUnsavedChangesExists();
    setCurrentCrystal(position);
    emit selectedSceneChanged(position);
  }
  return true;
}

bool Project::loadCrystalStructuresFromCifFile(const QString &filename) {
  CifFile cifReader;
  bool success = cifReader.readFromFile(filename);
  if (!success)
    return false;

  int position = -1;
  for (int i = 0; i < cifReader.numberOfCrystals(); i++) {
    CrystalStructure *tmp = new CrystalStructure();
    tmp->setOccCrystal(cifReader.getCrystalStructure(i));
    tmp->setFileContents(cifReader.getCrystalCifContents(i));
    tmp->setName(cifReader.getCrystalName(i));
    Scene *scene = new Scene(tmp);
    scene->setTitle(QFileInfo(filename).baseName());
    if (i == 0) {
      position = m_scenes.size();
      beginInsertRows(QModelIndex(), position, position);
      m_scenes.append(scene);
      endInsertRows();
    }
  }

  if (position > -1) {
    setUnsavedChangesExists();
    setCurrentCrystal(position);
    emit selectedSceneChanged(position);
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
      emit selectedSceneChanged(m_currentSceneIndex);
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
  emit showMessage("Complete all fragments");
}

void Project::toggleUnitCellAxes(bool state) {
  if (currentScene()) {
    currentScene()->setUnitCellBoxVisible(state);
    setUnsavedChangesExists();
    emit currentSceneChanged();
    emit showMessage(state ? "Show unit cell axes" : "Hide unit cell axes");
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
    emit showMessage(state ? "Show hydrogen atoms" : "Hide hydrogen atoms");
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

void Project::toggleCloseContacts(bool state) {
  if (currentScene()) {
    currentScene()->setShowCloseContacts(state);
    emit currentSceneChanged();
    emit showMessage(state ? "Show close contacts" : "Hide close contacts");
  }
}

void Project::toggleHydrogenBonds(bool state) {
  if (currentScene()) {
    currentScene()->setHydrogenBondsVisible(state);
    emit currentSceneChanged();
    emit showMessage(state ? "Show hydrogen bonds" : "Hide hydrogen bonds");
  }
}

bool Project::previouslySaved() { return (!_saveFilename.isEmpty()); }

QString Project::saveFilename() { return _saveFilename; }

void Project::updateHydrogenBondCriteria(HBondCriteria criteria) {
  qDebug() << "updateHydrogenBondsCriteria";
  auto * scene = currentScene();
  if (scene) {
    scene->updateHydrogenBondCriteria(criteria);
    emit currentSceneChanged();
  }
}

void Project::updateCloseContactsCriteria(int contactIndex, CloseContactCriteria criteria) {
  qDebug() << "updateCloseContactsForCurrent";
  auto * scene = currentScene();
  if (scene) {
    scene->updateCloseContactsCriteria(contactIndex, criteria);
    emit currentSceneChanged();
  }
}


void Project::frameworkOptionsChanged(FrameworkOptions options) {
  auto * scene = currentScene();
  qDebug() << "Project::frameworkOptionsChanged";
  if (scene) {
    scene->setFrameworkOptions(options);
    emit currentSceneChanged();
  }
}

void Project::removeIncompleteFragmentsForCurrentCrystal() {
  if (currentScene()) {
    currentScene()->deleteIncompleteFragments();
    setUnsavedChangesExists();
    emit currentSceneChanged();
  }
}

void Project::removeSelectedAtomsForCurrentCrystal() {
  if (currentScene()) {
    currentScene()->deleteSelectedAtoms();
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

QString Project::projectFileCompatibilityVersion() const { return CX_VERSION; }

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
  // TODO
  qDebug() << "toggleAtomsForFingerprintSelectionFilter";
  /*
  if (currentScene() && currentScene()->currentSurface()) {
    currentScene()->toggleAtomsForFingerprintSelectionFilter(show);
    // emit currentCrystalContentsChanged();
  } else {
    // No current surface!
    Q_ASSERT(false);
  }
  */
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
    currentScene()->unsuppressSelectedAtoms();
    setUnsavedChangesExists();
    emit currentSceneChanged();
  }
}

void Project::selectAllAtoms() {
  if (currentScene()) {
    currentScene()->setSelectStatusForAllAtoms(true);
    emit currentSceneChanged();
  }
}

void Project::selectSuppressedAtoms() {
  if (currentScene()) {
    currentScene()->setSelectStatusForSuppressedAtoms(true);
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
  qDebug() << "selectAtomsInsideCurrentSurface";
  /*
  if (currentScene() && currentScene()->currentSurface()) {
    currentScene()->selectAtomsSeparatedBySurface(true);
    emit currentSceneChanged();
  }
  */
}

void Project::selectAtomsOutsideCurrentSurface() {
  qDebug() << "selectAtomsInsideCurrentSurface";
  /*
  if (currentScene() && currentScene()->currentSurface()) {
    currentScene()->selectAtomsSeparatedBySurface(false);
    emit currentSceneChanged();
  }
  */
}

void Project::invertSelection() {
  if (currentScene()) {
    currentScene()->invertSelection();
    emit currentSceneChanged();
  }
}

void Project::removeAllMeasurements() {
  for (auto *scene : m_scenes) {
    if (scene) {
      scene->removeAllMeasurements();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &datastream, const Project &project) {
  qDebug() << "Load project";
  /*
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
  */

  return datastream;
}

QDataStream &operator>>(QDataStream &ds, Project &project) {
  /*
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
  */
  return ds;
}

int Project::rowCount(const QModelIndex &parent) const {
  return m_scenes.size();
}

QVariant Project::headerData(int section, Qt::Orientation orientation,
                             int role) const {
  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  if (orientation == Qt::Horizontal) {
    return tr("Structure");
  }
  // Optionally handle vertical headers or return QVariant() if not needed
  return QVariant();
}

int Project::columnCount(const QModelIndex &parent) const { return 1; }

QVariant Project::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_scenes.size())
    return QVariant();

  // Retrieve the Scene* directly from the index's internal pointer:
  Scene *scene = static_cast<Scene *>(index.internalPointer());
  if (!scene)
    return QVariant();

  if (role == Qt::DisplayRole) {
    return scene->title();
  } else if (role == Qt::DecorationRole) {
    auto kind = scene->periodicity();
    if (m_sceneKindIcons.contains(kind)) {
      return m_sceneKindIcons[kind];
    }
  }

  return QVariant();
}

QModelIndex Project::index(int row, int column,
                           const QModelIndex &parent) const {
  if (parent.isValid() || row < 0 || row >= m_scenes.size() || column != 0) {
    return QModelIndex();
  }
  return createIndex(row, column, static_cast<void *>(m_scenes.at(row)));
}

QModelIndex Project::parent(const QModelIndex &index) const {
  return QModelIndex();
}

void Project::onSelectionChanged(const QItemSelection &selected,
                                 const QItemSelection &deselected) {
  Q_UNUSED(deselected); // If not used

  QModelIndexList indexes = selected.indexes();
  if (!indexes.isEmpty()) {
    // Single selection mode assumed, take the first selected index
    QModelIndex currentIndex = indexes.first();
    setCurrentCrystal(currentIndex.row());
  }
}

bool Project::hasFrames() {
  auto * structure = currentStructure();
  if(!structure) return false;

  auto *dynamicStructure = qobject_cast<DynamicStructure*>(structure);
  if(!dynamicStructure) return false;
  return true;
}

int Project::nextFrame(bool forward) {
  auto * structure = currentStructure();
  if(!structure) return 0;

  int current = structure->getCurrentFrameIndex();
  if(forward) current++;
  else current--;
  return setCurrentFrame(current);
}

int Project::setCurrentFrame(int current) {
  auto * structure = currentStructure();
  if(!structure) return 0;

  int count = structure->frameCount();
  current = std::clamp(current, 0, count - 1);
  structure->setCurrentFrameIndex(current);
  emit currentSceneChanged();
  emit showMessage(QString("Show frame %1").arg(current));
  return current;
}

