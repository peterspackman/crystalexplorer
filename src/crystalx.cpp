#include <QAction>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>
#include <QRegularExpression>
#include <QTextBrowser>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>
#include <QtDebug>

#include "aboutcrystalexplorerdialog.h"
#include "confirmationbox.h"
#include "crystalx.h"
#include "dialoghtml.h"
#include "elastic_tensor_results.h"
#include "elementdata.h"
#include "gltf_exporter.h"
#include "isosurface_calculator.h"
#include "load_wavefunction.h"
#include "mathconstants.h"
#include "crystalclear.h"
#include "occelastictensortask.h"
#include "occelattask.h"
#include "pair_energy_calculator.h"
#include "plane.h"
#include "planeinstance.h"
#include "elastic_fit_io.h"
#include "save_pair_energy_json.h"
#include "settings.h"
#include "slabstructure.h"
#include "surface_cut_generator.h"
#include "ply_writer.h"
#include "wavefunction_calculator.h"

Crystalx::Crystalx() : QMainWindow() {
  setupUi(this);
  init();
}

void Crystalx::init() {
  if (!readElementData()) {
    exit(1); // if we can't read element data we can't continue
  }

  project = new Project(this);

  m_taskManager = new TaskManager();
  m_taskManagerWidget = new TaskManagerWidget(m_taskManager);
  initMainWindow();
  initStatusBar();
  initMenus();
  initGlWindow();
  setupDragAndDrop();
  initFingerprintWindow();
  initInfoViewer();
  createToolbars();
  createDockWidgets();
  initConnections();
  updateWindowTitle();
  initPreferencesDialog();
  initCloseContactsDialog();
  initSurfaceActions();
  updateCrystalActions();

  // Enable experimental features based on settings
  bool experimentalEnabled = settings::readSetting(settings::keys::ENABLE_EXPERIMENTAL_FEATURE_FLAG).toBool();
  enableExperimentalFeatures(experimentalEnabled);

  updateWorkingDirectories(".");
  m_exportDialog = new ExportDialog(this);
}

void Crystalx::initStatusBar() {
  _jobProgress = new QProgressBar(this);
  _jobCancel = new QToolButton(this);
  _jobCancel->setIcon(QPixmap(":/images/cross.png")
                          .scaledToWidth(24, Qt::SmoothTransformation));

  statusBar()->addPermanentWidget(_jobProgress);
  statusBar()->addPermanentWidget(_jobCancel);
  _jobProgress->setVisible(false);
  _jobCancel->setVisible(false);
}

bool Crystalx::readElementData() {
  bool success;

  QString filename =
      settings::readSetting(settings::keys::ELEMENTDATA_FILE).toString();
  QFileInfo fileInfo(filename);
  bool useJmolColors =
      settings::readSetting(settings::keys::USE_JMOL_COLORS).toBool();

  success = ElementData::getData(filename, useJmolColors);
  if (!success) {
    QMessageBox::critical(0, "Critical Error",
                          "CrystalExplorer can't read file :\n" +
                              fileInfo.fileName() +
                              "\n\nPlease reinstall CrystalExplorer.");
  }

  return success;
}

bool Crystalx::resetElementData() {
  bool useJmolColors =
      settings::readSetting(settings::keys::USE_JMOL_COLORS).toBool();
  return ElementData::resetAll(useJmolColors);
}

void Crystalx::initMainWindow() {
  resize(
      settings::readSetting(settings::keys::MAIN_WINDOW_SIZE).value<QSize>());
}

void Crystalx::initMenus() {
  createRecentFileActionsAndAddToFileMenu();
  updateRecentFileActions(
      settings::readSetting(settings::keys::FILE_HISTORY_LIST).toStringList());
  addExitOptionToFileMenu();
}

void Crystalx::createRecentFileActionsAndAddToFileMenu() {
  for (int i = 0; i < MAXHISTORYSIZE; ++i) {
    recentFileActions[i] = new QAction(this);
    recentFileActions[i]->setVisible(false);
    connect(recentFileActions[i], &QAction::triggered, this,
            &Crystalx::fileFromHistoryChosen);
    openRecentMenu->addAction(recentFileActions[i]);
  }
  openRecentMenu->addSeparator();
  _clearRecentFileAction = new QAction(this);
  _clearRecentFileAction->setText("Clear Recent Files");
  _clearRecentFileAction->setEnabled(false);
  connect(_clearRecentFileAction, &QAction::triggered, this,
          &Crystalx::clearFileHistory);
  openRecentMenu->addAction(_clearRecentFileAction);
}

void Crystalx::clearFileHistory() {
  QStringList recentFileHistory;
  settings::writeSetting(settings::keys::FILE_HISTORY_LIST, recentFileHistory);
  updateRecentFileActions(recentFileHistory);
}

void Crystalx::addExitOptionToFileMenu() {
  fileMenu->addSeparator();

  quitAction = new QAction(tr("E&xit"), this);
  quitAction->setShortcut(tr("Ctrl-Q"));
  quitAction->setStatusTip(tr("Exit CrystalExplorer"));
  fileMenu->addAction(quitAction);
  connect(quitAction, &QAction::triggered, this, &Crystalx::quit);
}

void Crystalx::createToolbars() {
  viewToolbar = new ViewToolbar(this);
  addToolBar(Qt::BottomToolBarArea, viewToolbar);
  connect(glWindow, &GLWindow::transformationMatrixChanged, this,
          &Crystalx::handleTransformationMatrixUpdate);
  connect(glWindow, &GLWindow::scaleChanged, viewToolbar,
          &ViewToolbar::setScale);
  connect(viewToolbar, &ViewToolbar::rotateAboutX, glWindow,
          &GLWindow::rotateAboutX);
  connect(viewToolbar, &ViewToolbar::rotateAboutY, glWindow,
          &GLWindow::rotateAboutY);
  connect(viewToolbar, &ViewToolbar::rotateAboutZ, glWindow,
          &GLWindow::rotateAboutZ);
  connect(viewToolbar, &ViewToolbar::scaleChanged, glWindow,
          &GLWindow::rescale);
  connect(viewToolbar, &ViewToolbar::viewDirectionChanged, glWindow,
          &GLWindow::viewMillerDirection);
  connect(viewToolbar, &ViewToolbar::recenterScene, glWindow,
          &GLWindow::recenterScene);
}

void Crystalx::handleTransformationMatrixUpdate() {
  Scene *scene = glWindow->currentScene();
  if (scene != nullptr) {
    auto o = scene->orientation();
    auto e = o.eulerAngles();
    viewToolbar->setRotations(e.x, e.y, e.z);
    auto t = o.transformationMatrix();
    occ::Vec3 cameraDirection(t.data()[2], t.data()[6], t.data()[10]);
    auto inverse = scene->inverseCellMatrix();
    occ::Vec3 miller = inverse * cameraDirection;
    float minD = 1.0f;
    for (size_t i = 0; i < 3; i++) {
      if (std::abs(miller(i)) < 0.001)
        continue;
      minD = std::fminf(std::abs(miller(i)), minD);
    }
    miller /= minD;
    viewToolbar->setMillerViewDirection(miller.x(), miller.y(), miller.z());
  }
}

void Crystalx::initGlWindow() {
  glWindow = new GLWindow(this);
  glWindow->setFormat(QSurfaceFormat::defaultFormat());
  setCentralWidget(glWindow);
}

void Crystalx::initFingerprintWindow() {
  fingerprintWindow = new FingerprintWindow(this);
}

void Crystalx::initInfoViewer() {
  infoViewer = new InfoViewer(this);
  connect(infoViewer, &InfoViewer::tabChangedTo, this, &Crystalx::updateInfo);
  connect(infoViewer, &InfoViewer::infoViewerClosed, this,
          &Crystalx::tidyUpAfterInfoViewerClosed);

  connect(infoViewer, &InfoViewer::energyColorSchemeChanged, this,
          &Crystalx::handleEnergyColorSchemeChanged);

  connect(infoViewer, &InfoViewer::elasticTensorRequested, this,
          &Crystalx::calculateElasticTensor);

  // Connect surface selection changes to update info viewer
  connect(project, &Project::surfaceSelectionChanged, infoViewer,
          &InfoViewer::updateInfoViewerForSurfaceChange);
}

void Crystalx::createDockWidgets() {
  createProjectControllerDockWidget();
  createChildPropertyControllerDockWidget();
}

void Crystalx::createChildPropertyControllerDockWidget() {
  childPropertyController = new ChildPropertyController();
  childPropertyControllerDockWidget = new QDockWidget(tr("Properties"));
  childPropertyControllerDockWidget->setObjectName(
      "childPropertyControllerDockWidget");
  childPropertyControllerDockWidget->setWidget(childPropertyController);
  childPropertyControllerDockWidget->setAllowedAreas(Qt::RightDockWidgetArea);
  childPropertyControllerDockWidget->setFeatures(
      QDockWidget::NoDockWidgetFeatures);
  childPropertyControllerDockWidget->adjustSize();
  addDockWidget(Qt::RightDockWidgetArea, childPropertyControllerDockWidget);
  childPropertyController->setEnabled(false);

  connect(childPropertyController, &ChildPropertyController::showFingerprint,
          this, &Crystalx::displayFingerprint);

  connect(project, &Project::clickedSurfacePropertyValue,
          childPropertyController,
          &ChildPropertyController::setSelectedPropertyValue);

  connect(childPropertyController,
          &ChildPropertyController::frameworkOptionsChanged, project,
          &Project::frameworkOptionsChanged);

  connect(projectController, &ProjectController::childSelectionChanged,
          [&](QModelIndex index) {
            auto *obj = projectController->getChild<QObject>(index);
            childPropertyController->setCurrentObject(obj);

            // Handle frame setting separately since it's project-specific
            if (auto *structure = qobject_cast<ChemicalStructure *>(obj)) {
              int frame = 0;
              auto prop = structure->property("frame");
              if (prop.isValid())
                frame = prop.toInt();
              project->setCurrentFrame(frame);
            }
          });
  connect(childPropertyController,
          &ChildPropertyController::meshSelectionChanged, this,
          &Crystalx::handleMeshSelectionChanged);
  connect(childPropertyController,
          &ChildPropertyController::generateSlabRequested, this,
          &Crystalx::generateSlabFromPlane);
  connect(childPropertyController,
          &ChildPropertyController::elasticTensorSelectionChanged, this,
          &Crystalx::handleElasticTensorSelectionChanged);
  connect(childPropertyController,
          &ChildPropertyController::exportCurrentSurface, this,
          &Crystalx::exportCurrentSurface);
  connect(childPropertyController,
          &ChildPropertyController::colorBarVisibilityChanged,
          [this](bool show, QString colorMapName, double minValue, double maxValue, QString label) {
            if (show) {
              glWindow->showColorBar(colorMapName, minValue, maxValue, label);
            } else {
              glWindow->hideColorBar();
            }
          });
}

void Crystalx::createProjectControllerDockWidget() {
  projectController = new ProjectController(project, this);
  projectControllerDockWidget = new QDockWidget(tr("Structures"));
  projectControllerDockWidget->setObjectName("projectControllerDockWidget");
  projectControllerDockWidget->setWidget(projectController);
  projectControllerDockWidget->setAllowedAreas(Qt::RightDockWidgetArea);
  projectControllerDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
  projectControllerDockWidget->adjustSize();
  // projectControllerDockWidget->setFocus();

  connect(project, &Project::surfaceSelectionChanged, projectController,
          &ProjectController::handleChildSelectionChange);

  addDockWidget(Qt::RightDockWidgetArea, projectControllerDockWidget);
}

void Crystalx::initConnections() {
  initMenuConnections();

  // Project connections - project changed in some way
  connect(project, &Project::projectModified, projectController,
          &ProjectController::handleProjectModified);

  connect(project, &Project::structureChanged, this,
          &Crystalx::handleStructureChange);

  connect(m_taskManager, &TaskManager::busyStateChanged, this,
          &Crystalx::handleBusyStateChange);

  // Project connections - current crystal changed in some way
  connect(project, &Project::sceneSelectionChanged, projectController,
          &ProjectController::handleSceneSelectionChange);
  connect(project, &Project::sceneSelectionChanged, this,
          &Crystalx::handleSceneSelectionChange);
  connect(project, &Project::sceneSelectionChanged,
          [&](int) { glWindow->setCurrentCrystal(project); });
  connect(project, &Project::projectSaved, this, &Crystalx::updateWindowTitle);
  connect(project, &Project::projectModified, this,
          &Crystalx::updateWindowTitle);
  connect(project, &Project::sceneSelectionChanged, this,
          &Crystalx::updateWindowTitle);
  connect(project, &Project::sceneSelectionChanged, this,
          &Crystalx::handleAtomSelectionChanged);
  connect(project, &Project::sceneSelectionChanged, this,
          &Crystalx::updateMenuOptionsForScene);
  connect(project, &Project::sceneSelectionChanged, this,
          &Crystalx::updateCloseContactOptions);
  connect(project, &Project::sceneSelectionChanged, this,
          &Crystalx::allowCloneSurfaceAction);
  connect(project, &Project::sceneSelectionChanged, this,
          &Crystalx::updateCrystalActions);
  connect(project, &Project::sceneSelectionChanged, infoViewer,
          &InfoViewer::updateInfoViewerForCrystalChange);
  connect(project, &Project::sceneSelectionChanged, glWindow,
          &GLWindow::redraw);
  connect(project, &Project::sceneContentChanged, glWindow, &GLWindow::redraw);
  connect(project, &Project::projectModified, glWindow, &GLWindow::redraw);
  connect(project, &Project::sceneSelectionChanged, this,
          &Crystalx::allowCloneSurfaceAction);
  connect(project, &Project::currentCrystalReset, glWindow,
          &GLWindow::resetViewAndRedraw);
  connect(project, &Project::currentCrystalReset, this,
          &Crystalx::handleAtomSelectionChanged);
  connect(project, &Project::atomSelectionChanged, this,
          &Crystalx::handleAtomSelectionChanged);
  connect(project, &Project::contactAtomsTurnedOff, this,
          &Crystalx::uncheckContactAtomsAction);

  // Crystal controller connections
  connect(projectController, &ProjectController::structureSelectionChanged,
          project, QOverload<int>::of(&Project::setCurrentCrystal));

  // Fingerprint window connections
  connect(fingerprintWindow, &FingerprintWindow::surfaceFeatureChanged,
          glWindow, &GLWindow::updateSurfacesForFingerprintWindow);

  // GLWindow connections (other connections made elsewhere in Crystalx)
  /*
   * TODO
  connect(glWindow, &GLWindow::surfaceHideRequest, project,
          &Project::hideSurface);
  connect(glWindow, &GLWindow::surfaceDeleteRequest, project,
          &Project::confirmAndDeleteSurface);

  */
  connect(glWindow, &GLWindow::resetCurrentCrystal, project,
          &Project::resetCurrentCrystal);
  connect(glWindow, &GLWindow::contextualFilterAtoms, project,
          &Project::filterAtomsForCurrentScene);

  connect(glWindow, &GLWindow::atomLabelOptionsChanged, project,
          &Project::atomLabelOptionsChanged);
  connect(glWindow, &GLWindow::loadWavefunctionRequested, this,
          &Crystalx::handleLoadWavefunctionAction);

  connect(m_taskManager, &TaskManager::taskComplete, this,
          &Crystalx::taskManagerTaskComplete);
  connect(m_taskManager, &TaskManager::taskError, this,
          &Crystalx::taskManagerTaskError);
  connect(m_taskManager, &TaskManager::taskAdded, this,
          &Crystalx::taskManagerTaskAdded);
  connect(m_taskManager, &TaskManager::taskRemoved, this,
          &Crystalx::taskManagerTaskRemoved);

  initActionGroups();
}

void Crystalx::uncheckContactAtomsAction() {
  toggleContactAtomsAction->blockSignals(true);
  toggleContactAtomsAction->setChecked(false);
  toggleContactAtomsAction->blockSignals(false);
}

void Crystalx::initMenuConnections() {
  // File menu
  connect(fileNewAction, &QAction::triggered, this, &Crystalx::newProject);
  connect(fileOpenAction, &QAction::triggered, this, &Crystalx::openFile);
  connect(fileSaveAction, &QAction::triggered, this, &Crystalx::saveProject);
  connect(fileSaveAsAction, &QAction::triggered, this,
          &Crystalx::saveProjectAs);
  connect(preferencesAction, &QAction::triggered, this,
          &Crystalx::showPreferencesDialog);

  // Import menu
  connect(importElasticTensorAction, &QAction::triggered, this,
          &Crystalx::showElasticTensorImportDialog);

  connect(actionExport_As, &QAction::triggered, this, &Crystalx::exportAs);
  connect(quickExportAction, &QAction::triggered, this,
          &Crystalx::quickExportCurrentGraphics);
  connect(exportGeometryAction, &QAction::triggered, this,
          &Crystalx::handleExportCurrentGeometry);
  connect(exportToGLTFAction, &QAction::triggered, this,
          &Crystalx::handleExportToGLTF);

  // Scene menu
  connect(animateAction, &QAction::toggled, this, &Crystalx::setAnimateScene);
  connect(backgroundColorAction, &QAction::triggered, glWindow,
          &GLWindow::getNewBackgroundColor);
  connect(orientationSaveAsAction, &QAction::triggered, glWindow,
          &GLWindow::saveOrientation);
  connect(orientationSwitchToAction, &QAction::triggered, glWindow,
          &GLWindow::switchToOrientation);

  // Display menu
  initMoleculeStyles();
  connect(showUnitCellAxesAction, &QAction::toggled, project,
          &Project::toggleUnitCellAxes);
  connect(enableMultipleUnitCellBoxesAction, &QAction::toggled, project,
          &Project::toggleMultipleUnitCellBoxes);

  connect(showAtomicLabelsAction, &QAction::triggered, this,
          &Crystalx::handleAtomLabelActions);
  connect(showFragmentLabelsAction, &QAction::triggered, this,
          &Crystalx::handleAtomLabelActions);

  connect(showHydrogenAtomsAction, &QAction::toggled, project,
          &Project::toggleHydrogenAtoms);
  connect(showSuppressedAtomsAction, &QAction::toggled, project,
          &Project::toggleSuppressedAtoms);
  connect(cycleDisorderHighlightingAction, &QAction::triggered, project,
          &Project::cycleDisorderHighlighting);
  connect(energyFrameworksAction, &QAction::triggered, this,
          &Crystalx::showEnergyFrameworkDialog);

  connect(togglePairHighlightingAction, &QAction::toggled, this,
          &Crystalx::togglePairInteractionHighlighting);

  connect(selectAllAtomsAction, &QAction::triggered, project,
          &Project::selectAllAtoms);
  connect(selectsAtomsInsideCurrentSurfaceAction, &QAction::triggered, project,
          &Project::selectAtomsInsideCurrentSurface);
  connect(selectAtomsOutsideCurrentSurfaceAction, &QAction::triggered, project,
          &Project::selectAtomsOutsideCurrentSurface);
  connect(selectAtomsOutsideRadiusAction, &QAction::triggered, this,
          &Crystalx::selectAtomsOutsideRadius);
  connect(selectSuppressedAtomsAction, &QAction::triggered, project,
          &Project::selectSuppressedAtoms);
  connect(removeIncompleteFragmentsAction, &QAction::triggered, project,
          &Project::removeIncompleteFragmentsForCurrentCrystal);
  connect(removeSelectedAtomsAction, &QAction::triggered, [this]() {
    project->filterAtomsForCurrentScene(AtomFlag::Selected, true);
  });
  connect(suppressSelectedAtomsAction, &QAction::triggered, project,
          &Project::suppressSelectedAtoms);
  connect(unsuppressSelectedAtomsAction, &QAction::triggered, project,
          &Project::unsuppressSelectedAtoms);
  connect(invertSelectionAction, &QAction::triggered, project,
          &Project::invertSelection);

  connect(project, &Project::showMessage, glWindow,
          &GLWindow::showMessageOnGraphicsView);

  // NB hbondOptionsAction and closeContactOptionsAction are connected in
  // initCloseContactsDialog
  connect(depthFadingOptionsAction, &QAction::triggered, this,
          &Crystalx::showDepthFadingOptions);
  connect(clippingOptionsAction, &QAction::triggered, this,
          &Crystalx::showClippingOptions);
  connect(clearCurrentCrystalAction, &QAction::triggered, this,
          &Crystalx::clearCurrent);
  connect(clearAllCrystalsAction, &QAction::triggered, this,
          &Crystalx::clearAll);
  connect(resetCrystalAction, &QAction::triggered, project,
          &Project::resetCurrentCrystal);

  // Actions menu
  connect(toggleContactAtomsAction, &QAction::toggled, project,
          &Project::toggleCloseContacts);
  connect(completeFragmentsAction, &QAction::triggered, project,
          &Project::completeFragmentsForCurrentCrystal);
  connect(showAtomsWithinRadiusAction, &QAction::triggered, this,
          &Crystalx::setShowAtomsWithinRadius);
  connect(generateSurfaceAction, &QAction::triggered, this,
          &Crystalx::getSurfaceParametersFromUser);
  connect(createPlaneAction, &QAction::triggered, this,
          &Crystalx::showPlaneDialog);
  connect(crystalCutsAction, &QAction::triggered, this,
          &Crystalx::showCrystalCutDialog);
  connect(generateCellsAction, &QAction::triggered, this,
          &Crystalx::generateSlab);
  connect(cloneSurfaceAction, &QAction::triggered, this,
          &Crystalx::cloneSurface);
  connect(calculateEnergiesAction, &QAction::triggered, this,
          &Crystalx::showEnergyCalculationDialog);
  connect(calculateLatticeEnergyAction, &QAction::triggered, this,
          &Crystalx::showLatticeEnergyDialog);
  connect(setFragmentChargesAction, &QAction::triggered, this,
          &Crystalx::setFragmentStates);

  // Help menu
  connect(helpAboutAction, &QAction::triggered, this,
          &Crystalx::helpAboutActionDialog);
  connect(crystalExplorerWebsiteAction, &QAction::triggered, this,
          &Crystalx::gotoCrystalExplorerWebsite);
  connect(howtoCiteCrystalExplorerWebsiteAction, &QAction::triggered, this,
          &Crystalx::gotoHowToCiteCrystalExplorer);

  // Just in toolbars at the moment
  connect(selectAction, &QAction::triggered, this,
          &Crystalx::resetSelectionMode);
  connect(infoAction, &QAction::triggered, this, &Crystalx::showInfoViewer);
  connect(actionShowTaskManager, &QAction::triggered, this,
          &Crystalx::showTaskManagerWidget);

  connect(generateWavefunctionAction, &QAction::triggered, this,
          &Crystalx::handleGenerateWavefunctionAction);
  connect(loadWavefunctionAction, &QAction::triggered, this,
          &Crystalx::handleLoadWavefunctionAction);

  // animation frames
  connect(nextFrameAction, &QAction::triggered,
          [this]() { project->nextFrame(true); });
  connect(previousFrameAction, &QAction::triggered,
          [this]() { project->nextFrame(false); });
}

void Crystalx::initCloseContactsDialog() {
  m_closeContactDialog = new CloseContactDialog();
  connect(m_closeContactDialog, &CloseContactDialog::hbondCriteriaChanged,
          project, &Project::updateHydrogenBondCriteria);
  connect(m_closeContactDialog, &CloseContactDialog::hbondsToggled, project,
          &Project::toggleHydrogenBonds);

  connect(hbondOptionsAction, &QAction::triggered, m_closeContactDialog,
          &CloseContactDialog::showDialogWithHydrogenBondTab);
  connect(closeContactOptionsAction, &QAction::triggered, m_closeContactDialog,
          &CloseContactDialog::showDialogWithCloseContactsTab);

  connect(m_closeContactDialog,
          &CloseContactDialog::closeContactsSettingsChanged, project,
          &Project::updateCloseContactsCriteria);
}

void Crystalx::updateCrystalActions() {
  bool enable = project->currentScene();
  bool hasCrystalStructure =
      enable && qobject_cast<CrystalStructure *>(
                    project->currentScene()->chemicalStructure()) != nullptr;

  completeFragmentsAction->setEnabled(enable);
  generateCellsAction->setEnabled(enable);
  toggleContactAtomsAction->setEnabled(enable);
  showAtomsWithinRadiusAction->setEnabled(enable);
  generateWavefunctionAction->setEnabled(enable);
  loadWavefunctionAction->setEnabled(enable);

  distanceAction->setEnabled(enable);
  angleAction->setEnabled(enable);
  dihedralAction->setEnabled(enable);
  OutOfPlaneBendAction->setEnabled(enable);
  InPlaneBendAction->setEnabled(enable);
  calculateEnergiesAction->setEnabled(enable);
  infoAction->setEnabled(enable);

  // Crystal cuts only enabled when there's a crystal structure
  crystalCutsAction->setEnabled(hasCrystalStructure);
}

void Crystalx::initSurfaceActions() {
  generateSurfaceAction->setEnabled(false);
  cloneSurfaceAction->setEnabled(false);
}

void Crystalx::enableExperimentalFeatures(bool enable) {
  experimentalAction->setEnabled(enable);
  experimentalAction->setVisible(enable);
  infoViewer->enableExperimentalFeatures(enable);
}

void Crystalx::gotoCrystalExplorerWebsite() {
  QDesktopServices::openUrl(QUrl(cx::globals::url));
}
void Crystalx::gotoHowToCiteCrystalExplorer() {
  QDesktopServices::openUrl(QUrl(cx::globals::citationUrl));
}

void Crystalx::setShowAtomsWithinRadius() {
  if (project->currentScene()) {
    bool generateClusterForSelection = project->currentHasSelectedAtoms();

    QString title = "Show atoms within a radius...";

    QString label;
    QString msgStart = QString("Show atoms within a ") +
                       DialogHtml::bold(QString("radius (") +
                                        cx::globals::angstromSymbol + ")");
    if (generateClusterForSelection) {
      label = DialogHtml::paragraph(msgStart + " of the selected atoms");
    } else {
      label = DialogHtml::paragraph(msgStart + " of the " +
                                    DialogHtml::bold("all") + " atoms");
      QString selectionMsg =
          "If you only want to generate a radial cluster for some atoms";
      selectionMsg += DialogHtml::linebreak();
      selectionMsg += "then select them first before choosing this option.";
      label +=
          DialogHtml::paragraph(DialogHtml::font(selectionMsg, "2", "gray"));
    }

    bool ok;
    // SHOW ATOMS WITHIN RADIUS MAX = 50.0, MIN = 0.0
    double radius = QInputDialog::getDouble(this, title, label, 3.8, 0.0, 50.0,
                                            2, &ok, Qt::Tool);
    if (ok) {
      project->showAtomsWithinRadius(static_cast<float>(radius),
                                     generateClusterForSelection);
    }
  }
}

void Crystalx::selectAtomsOutsideRadius() {
  if (project->currentScene()) {
    bool ok;
    QString title = "Select Atoms";
    QString label = QString("Select atoms outside a <b>radius (") +
                    cx::globals::angstromSymbol +
                    ")</b> of the currently selected atoms:";
    double radius = QInputDialog::getDouble(this, title, label, 5.0, 0.0, 25.0,
                                            2, &ok, Qt::Tool);
    if (ok) {
      project->selectAtomsOutsideRadiusOfSelectedAtoms(radius);
    }
  }
}

void Crystalx::setAnimateScene(bool animate) {
  if (_animationSettingsDialog == 0) {
    _animationSettingsDialog = new AnimationSettingsDialog();
    _animationSettingsDialog->reset();
    connect(_animationSettingsDialog,
            &AnimationSettingsDialog::animationSettingsChanged, glWindow,
            &GLWindow::setAnimationSettings);
    connect(_animationSettingsDialog,
            &AnimationSettingsDialog::animationToggled, this,
            &Crystalx::toggleAnimation);
  }
  if (animate) {
    _animationSettingsDialog->setVisible(animate);
  }
}

void Crystalx::toggleAnimation(bool animate) {
  glWindow->setAnimateScene(animate);
  animateAction->setChecked(animate);
}

void Crystalx::clearCurrent() { project->deleteCurrentStructure(); }

void Crystalx::clearAll() { project->deleteAllStructures(); }

void Crystalx::generateSlab() {
  Q_ASSERT(project->currentScene());

  // Determine periodicity based on current structure
  int periodicDimensions = 3; // Default to 3D
  auto structure = project->currentScene()->chemicalStructure();
  if (structure) {
    auto structureType = structure->structureType();
    switch (structureType) {
      case ChemicalStructure::StructureType::Cluster:
        periodicDimensions = 0;
        break;
      case ChemicalStructure::StructureType::Wire:
        periodicDimensions = 1;
        break;
      case ChemicalStructure::StructureType::Surface:
        periodicDimensions = 2;
        break;
      case ChemicalStructure::StructureType::Crystal:
        periodicDimensions = 3;
        break;
    }
  }

  bool ok{false};
  auto slabOptions = CellLimitsDialog::getSlabGenerationOptions(
      0, "Generate slab", QString(), periodicDimensions, ok);

  if (ok) {
    m_savedSlabGenerationOptions =
        slabOptions; // save cell limits for use by cloneVoidSurface
    project->generateSlab(slabOptions);
  }
}

void Crystalx::generateSlabFromPlane(int h, int k, int l, double offset) {
  Q_ASSERT(project->currentScene());

  Scene *scene = project->currentScene();
  if (!scene) {
    qDebug() << "No current scene for slab generation";
    return;
  }

  auto *crystal = qobject_cast<CrystalStructure *>(scene->chemicalStructure());
  if (!crystal) {
    qDebug() << "Current structure is not a crystal - cannot create slab";
    return;
  }

  // Create and show the crystal cut dialog
  auto *dialog = new CrystalCutDialog(this);
  dialog->setMillerIndices(h, k, l);
  dialog->setInitialOffset(offset);
  dialog->setCrystalStructure(crystal);

  // Connect the dialog's signal to actually create the slab
  connect(dialog, &CrystalCutDialog::slabCutRequested,
          [this, crystal](const SlabCutOptions &options) {
            // For surface cuts, d-spacing units map directly to fractional
            // units (i.e., 1.0 d = 1.0 fractional unit along the surface
            // normal)
            double fractionalOffset = options.offset;

            // Use the existing surface cut generation function
            SlabStructure *slab = cx::crystal::generateSurfaceCut(
                crystal, options.h, options.k, options.l, fractionalOffset,
                options.thickness);

            if (!slab) {
              qDebug() << "Failed to generate slab from plane";
              return;
            }

            // Set a descriptive title
            QString title = QString("Slab (%1,%2,%3) offset=%4d depth=%5Å")
                                .arg(options.h)
                                .arg(options.k)
                                .arg(options.l)
                                .arg(options.offset, 0, 'f', 2)
                                .arg(options.thickness, 0, 'f', 1);

            // Add the slab structure to the project
            project->addSlabStructure(slab, title);

            qDebug() << "Created slab structure:" << title;
          });

  dialog->show();
}

void Crystalx::helpAboutActionDialog() {
  auto dialog = new AboutCrystalExplorerDialog(this);
  dialog->exec();
}

void Crystalx::initMoleculeStyles() {
  // QActionGroup* moleculeStyleGroup = new QActionGroup(toolBar);
  const auto availableDrawingStyles = {
      DrawingStyle::Tube,        DrawingStyle::BallAndStick,
      DrawingStyle::SpaceFill,   DrawingStyle::WireFrame,
      DrawingStyle::Ortep,       DrawingStyle::Centroid,
      DrawingStyle::CenterOfMass};
  for (const auto &drawingStyle : availableDrawingStyles) {
    QString moleculeStyleString = drawingStyleLabel(drawingStyle);
    m_drawingStyleLabelToDrawingStyle[moleculeStyleString] = drawingStyle;
    if (drawingStyle == DrawingStyle::Ortep) {
      const QStringList probs{"0.50", "0.90", "0.99"};
      m_thermalEllipsoidMenu = new QMenu(moleculeStyleString);
      for (int i = 0; i < probs.size(); i++) {
        QAction *action = new QAction(this);
        action->setCheckable(true);
        action->setText(probs[i]);
        m_thermalEllipsoidMenu->addAction(action);
        moleculeStyleActions.append(action);
        connect(action, &QAction::triggered, this,
                &Crystalx::setEllipsoidStyleWithProbabilityForCurrent);
      }
      m_drawHEllipsoidsAction = new QAction(this);
      m_drawHEllipsoidsAction->setCheckable(true);
      m_drawHEllipsoidsAction->setChecked(true);
      m_drawHEllipsoidsAction->setText("Draw H Ellipsoids");
      m_thermalEllipsoidMenu->addSeparator();
      m_thermalEllipsoidMenu->addAction(m_drawHEllipsoidsAction);
      connect(m_drawHEllipsoidsAction, &QAction::toggled, this,
              &Crystalx::toggleDrawHydrogenEllipsoids);
      optionsMoleculeStylePopup->addMenu(m_thermalEllipsoidMenu);
    } else {
      QAction *action = new QAction(this);
      action->setCheckable(true);
      action->setText(moleculeStyleString);
      action->setShortcut(drawingStyleKeySequence(drawingStyle));
      optionsMoleculeStylePopup->addAction(action);
      moleculeStyleActions.append(action);
      connect(action, &QAction::triggered, this,
              &Crystalx::setMoleculeStyleForCurrent);
    }
  }
}

void Crystalx::initActionGroups() {
  // init action group for rotate, translate and scale buttons
  QActionGroup *rotTransScaleGroup = new QActionGroup(toolBar);
  rotTransScaleGroup->addAction(actionTranslateAction);
  rotTransScaleGroup->addAction(actionRotateAction);
  rotTransScaleGroup->addAction(actionScaleAction);
  connect(rotTransScaleGroup, &QActionGroup::triggered, this,
          &Crystalx::processRotTransScaleAction);

  // init action for distance, angle and dihedral buttons
  QActionGroup *measurementGroup = new QActionGroup(toolBar);
  measurementGroup->addAction(distanceAction);
  measurementGroup->addAction(minDistanceAction);
  measurementGroup->addAction(angleAction);
  measurementGroup->addAction(dihedralAction);
  measurementGroup->addAction(OutOfPlaneBendAction);
  measurementGroup->addAction(InPlaneBendAction);
  measurementGroup->addAction(undoLastMeasurementAction);
  undoLastMeasurementAction->setEnabled(false);
  connect(measurementGroup, &QActionGroup::triggered, this,
          &Crystalx::processMeasurementAction);
}

void Crystalx::processRotTransScaleAction(QAction *action) {
  MouseMode mode{rotate};
  if (action == actionTranslateAction) {
    mode = translate;
  }
  if (action == actionRotateAction) {
    mode = rotate;
  }
  if (action == actionScaleAction) {
    mode = zoom;
  }
  glWindow->setMouseMode(mode);
}

void Crystalx::processMeasurementAction(QAction *action) {
  SelectionMode mode{SelectionMode::Pick};

  if (action == undoLastMeasurementAction) {
    glWindow->undoLastMeasurement();
    if (!glWindow->hasMeasurements()) {
      resetSelectionMode();
    }
  } else {
    if (action == distanceAction) {
      mode = SelectionMode::Distance;
    }
    if (action == angleAction) {
      mode = SelectionMode::Angle;
    }
    if (action == dihedralAction) {
      mode = SelectionMode::Dihedral;
    }
    if (action == OutOfPlaneBendAction) {
      mode = SelectionMode::OutOfPlaneBend;
    }
    if (action == InPlaneBendAction) {
      mode = SelectionMode::InPlaneBend;
    }
    glWindow->setSelectionMode(mode);
    selectAction->setEnabled(true);
    undoLastMeasurementAction->setEnabled(true);
  }
}

void Crystalx::resetSelectionMode() {
  glWindow->setSelectionMode(SelectionMode::Pick);
  project->removeAllMeasurements();
  distanceAction->setChecked(false);
  minDistanceAction->setChecked(false);
  angleAction->setChecked(false);
  dihedralAction->setChecked(false);
  OutOfPlaneBendAction->setChecked(false);
  InPlaneBendAction->setChecked(false);
  undoLastMeasurementAction->setChecked(false);
  undoLastMeasurementAction->setEnabled(false);
  selectAction->setEnabled(false);

  glWindow->redraw();
}

void Crystalx::openFile() {
  const QString FILTER = "CIF, CIF2, Project File, XYZ file (*." +
                         CIF_EXTENSION + " *." + PROJECT_EXTENSION + " *." +
                         CIF2_EXTENSION + " *." + XYZ_FILE_EXTENSION +
                         " *.pdb" + " *.json" + " *.gin" + ")";
  QStringList filenames = QFileDialog::getOpenFileNames(
      0, tr("Open File"), QDir::currentPath(), FILTER);

  for (const auto &filename : std::as_const(filenames)) {
    openFilename(filename);
  }
}

void Crystalx::fileFromHistoryChosen() {
  QAction *action = qobject_cast<QAction *>(sender());
  if (action) {
    QString filename = action->data().toString();
    openFilename(filename);
  }
}

void Crystalx::openFilename(QString filename) {
  if (QFile::exists(filename)) {
    addFileToHistory(filename);
    loadExternalFileData(filename);
  } else {
    QMessageBox::information(this, "Unable to open file",
                             "The file " + filename + " does not exist!");
    removeFileFromHistory(filename);
  }
}

void Crystalx::removeFileFromHistory(const QString &filename) {
  QStringList recentFileHistory =
      settings::readSetting(settings::keys::FILE_HISTORY_LIST).toStringList();

  if (recentFileHistory.contains(filename)) {
    recentFileHistory.removeOne(filename); // file is already in history so we
    // remove it so that we can...
  }
  settings::writeSetting(settings::keys::FILE_HISTORY_LIST, recentFileHistory);
  updateRecentFileActions(recentFileHistory);
}

void Crystalx::addFileToHistory(const QString &filename) {
  QStringList recentFileHistory =
      settings::readSetting(settings::keys::FILE_HISTORY_LIST).toStringList();

  if (recentFileHistory.contains(filename)) {
    recentFileHistory.removeOne(filename); // file is already in history so we
    // remove it so that we can...
  }
  recentFileHistory.push_front(filename); // ...add filename to start history
  while (recentFileHistory.size() >
         MAXHISTORYSIZE) { // Limit the history count to MAXHISTORYSIZE entries.
    recentFileHistory.removeLast();
  }

  settings::writeSetting(settings::keys::FILE_HISTORY_LIST, recentFileHistory);

  updateRecentFileActions(recentFileHistory);
}

void Crystalx::updateRecentFileActions(QStringList recentFileHistory) {
  int i = 0;
  while (i < MAXHISTORYSIZE) {
    recentFileActions[i]->setVisible(false);
    ++i;
  }

  int fileHistorySize = recentFileHistory.size();

  for (i = 0; i < fileHistorySize; ++i) {
    QFileInfo fileInfo(recentFileHistory[i]);

    QString filename = fileInfo.fileName();
    QString text = tr("&%1 %2").arg(i + 1).arg(filename);
    recentFileActions[i]->setText(text);
    recentFileActions[i]->setData(recentFileHistory[i]);
    recentFileActions[i]->setVisible(true);
  }

  _clearRecentFileAction->setEnabled(fileHistorySize > 0);
}

void Crystalx::updateWorkingDirectories(const QString &filename) {
  QFileInfo fileInfo(filename);
  QDir::setCurrent(fileInfo.absolutePath());
}

void Crystalx::loadExternalFileData(QString filename) {
  updateWorkingDirectories(filename);
  qDebug() << "Load external data from" << filename;

  QFileInfo fileInfo(filename);
  QString extension = fileInfo.suffix().toLower();

  if (filename.endsWith("cg_results.json")) {
    showStatusMessage(
        QString("Loading crystal clear output from %1").arg(filename));
    project->loadCrystalClearJson(filename);
  }
  if (filename.endsWith("elat_results.json")) {
    showStatusMessage(QString("Loading occ elat output from %1").arg(filename));
    project->loadCrystalClearJson(filename);
  } else if (filename.endsWith("surface.json")) {
    showStatusMessage(QString("Loading crystal surface from %1").arg(filename));
    project->loadCrystalClearSurfaceJson(filename);
  } else if (extension == CIF_EXTENSION || extension == CIF2_EXTENSION) {
    processCif(filename);
  } else if (extension == "pdb") {
    processPdb(filename);
  } else if (filename.endsWith(PROJECT_EXTENSION)) {
    loadProject(filename);
  } else if (extension == XYZ_FILE_EXTENSION) {
    loadXyzFile(filename);
  } else if (extension == "gin") {
    qDebug() << "Loading gulp input file: " << filename;
    showStatusMessage(QString("Loading gulp input file from %1").arg(filename));
    project->loadGulpInputFile(filename);
  }
}

void Crystalx::loadXyzFile(const QString &filename) {
  qDebug() << "Loading xyz file: " << filename;
  // must be done outside lambda, filename must be copied.
  showStatusMessage(QString("Loading xyz file from %1").arg(filename));
  project->loadChemicalStructureFromXyzFile(filename);
}

void Crystalx::loadProject(QString filename) {
  // Don't reopen the same project if there are no unsaved changes
  if (filename == project->saveFilename() && !project->hasUnsavedChanges()) {
    return;
  }

  // Are there changes to the current project to be saved?
  if (!closeProjectConfirmed()) {
    return;
  }

  if (project->loadFromFile(filename)) {
    setBusy(false);
  } else {
    QMessageBox::information(this, "Unable to open project",
                             "Unable to open the project: " + filename);
  }
}

/*!
 \brief Uses Tonto to process a CIF into a .cxc file.
 \param filename The filename of the CIF to process

 When Tonto is finished the slot tontoJobFinished slot is called.
 */
void Crystalx::processCif(QString &filename) {
  qDebug() << "Loading CIF file: " << filename;
  // must be done outside lambda, filename must be copied.
  showStatusMessage(QString("Loading CIF file from %1").arg(filename));
  project->loadCrystalStructuresFromCifFile(filename);
}

void Crystalx::processPdb(QString &filename) {
  qDebug() << "Loading CIF file: " << filename;
  // must be done outside lambda, filename must be copied.
  showStatusMessage(QString("Loading PDB file from %1").arg(filename));
  project->loadCrystalStructuresFromPdbFile(filename);
}

void Crystalx::handleBusyStateChange(bool busy) { setBusy(busy); }

void Crystalx::jobRunning() { setBusy(true); }

void Crystalx::jobCancelled(QString message) {
  showStatusMessage(message);
  setBusy(false);
}

void Crystalx::setBusy(bool busy) {
  setBusyIcon(busy);
  disableActionsWhenBusy(busy);
  projectController->setEnabled(!busy);
  viewToolbar->showCalculationRunning(busy);
  _jobCancel->setVisible(busy);
  if (!busy) {
    _jobProgress->setVisible(busy);
  }
}

void Crystalx::setBusyIcon(bool busy) {
  if (busy) {
    setWindowIcon(QPixmap(":images/CrystalExplorerBusy.png"));
  } else {
    setWindowIcon(QPixmap(":images/CrystalExplorer.png"));
  }
}

void Crystalx::disableActionsWhenBusy(bool busy) {
  fileOpenAction->setEnabled(!busy);

  if (project->currentScene()) {
    // These actions have extra conditions for enabling (e.g. selected atoms)
    enableGenerateSurfaceAction(!busy);
    enableCloneSurfaceAction(!busy);
    enableCalculateEnergiesAction(!busy);
  }
}

bool Crystalx::overrideBondLengths() {
  return settings::readSetting(settings::keys::XH_NORMALIZATION).toBool();
}

void Crystalx::showStatusMessage(QString message) {
  statusBar()->showMessage(message, STATUSBAR_MSG_DELAY);
}

void Crystalx::updateStatusMessage(QString s) { statusBar()->showMessage(s); }

void Crystalx::clearStatusMessage() { statusBar()->clearMessage(); }

void Crystalx::updateProgressBar(int current_step, int max_steps) {
  if (current_step >= max_steps) {
    _jobProgress->setVisible(false);
  } else if (max_steps >= 1) {
    _jobProgress->setVisible(true);
    _jobProgress->setMaximum(max_steps);
    _jobProgress->setValue(current_step);
  }
}

void Crystalx::getSurfaceParametersFromUser() {
  Scene *scene = project->currentScene();
  if (!scene)
    return;
  auto *structure = scene->chemicalStructure();
  if (!structure)
    return;

  // Secret option to allow the reading of surface files
  // In general this is a bad idea because the surface file
  // doesn't contain all the information about how the surface
  // was generated. All we don't check the surface was generated
  // for the same crystal.
  if (m_surfaceGenerationDialog == nullptr) {
    m_surfaceGenerationDialog = new SurfaceGenerationDialog(this);
    m_surfaceGenerationDialog->setModal(true);
    connect(m_surfaceGenerationDialog,
            &SurfaceGenerationDialog::surfaceParametersChosenNew, this,
            &Crystalx::generateSurface);
    connect(m_surfaceGenerationDialog,
            &SurfaceGenerationDialog::surfaceParametersChosenNeedWavefunction,
            this, &Crystalx::generateSurfaceRequiringWavefunction);
  }
  auto atomIndices = structure->atomsWithFlags(AtomFlag::Selected);
  m_surfaceGenerationDialog->setAtomIndices(atomIndices);
  m_surfaceGenerationDialog->setStructure(structure);

  m_surfaceGenerationDialog->setNumberOfElectronsForCalculation(
      structure->atomicNumbersForIndices(atomIndices).sum());
  auto candidates = structure->wavefunctionsAndTransformsForAtoms(atomIndices);
  m_surfaceGenerationDialog->setSuitableWavefunctions(candidates);
  m_surfaceGenerationDialog->show();
}

void Crystalx::showPlaneDialog() {
  Scene *scene = project->currentScene();
  if (!scene)
    return;
  auto *structure = scene->chemicalStructure();
  if (!structure)
    return;

  if (m_planeDialog == nullptr) {
    m_planeDialog = new PlaneDialog(this);
    m_planeDialog->setModal(true);
    connect(m_planeDialog, &QDialog::accepted, [this]() {
      Scene *scene = project->currentScene();
      if (!scene)
        return;
      auto *structure = scene->chemicalStructure();
      if (!structure)
        return;

      // Create the plane with the structure as parent
      Plane *plane = m_planeDialog->createPlane(structure);

      // Plane instances are now created automatically by PlaneDialog based on
      // configured offsets
    });
  }

  m_planeDialog->show();
}

void Crystalx::showCrystalCutDialog() {
  Scene *scene = project->currentScene();
  if (!scene)
    return;
  auto *crystalStructure =
      qobject_cast<CrystalStructure *>(scene->chemicalStructure());
  if (!crystalStructure)
    return;

  if (m_crystalCutDialog == nullptr) {
    m_crystalCutDialog = new CrystalCutDialog(this);
    m_crystalCutDialog->setModal(true);
    connect(m_crystalCutDialog, &QDialog::accepted, this,
            &Crystalx::handleCrystalCutDialogAccepted);
  }

  // Set the current structure in the dialog if needed
  m_crystalCutDialog->setCrystalStructure(crystalStructure);
  m_crystalCutDialog->show();
}

void Crystalx::handleCrystalCutDialogAccepted() {
  Scene *scene = project->currentScene();
  if (!scene)
    return;
  auto *crystalStructure =
      qobject_cast<CrystalStructure *>(scene->chemicalStructure());
  if (!crystalStructure)
    return;

  // Get the cut parameters from the dialog and generate the slab directly
  auto options = m_crystalCutDialog->getSlabOptions();

  // Use the same logic as generateSlabFromPlane but without showing another
  // dialog
  SlabStructure *slab = cx::crystal::generateSurfaceCut(
      crystalStructure, options.h, options.k, options.l, options.offset,
      options.thickness);

  if (!slab) {
    qDebug() << "Failed to generate slab from crystal cut dialog";
    return;
  }

  // Set a descriptive title and add to project (following the same pattern as
  // generateSlabFromPlane)
  QString title = QString("Slab (%1,%2,%3) offset=%4d depth=%5Å")
                      .arg(options.h)
                      .arg(options.k)
                      .arg(options.l)
                      .arg(options.offset, 0, 'f', 2)
                      .arg(options.thickness, 0, 'f', 1);

  // Add the slab structure to the project
  project->addSlabStructure(slab, title);
}

void Crystalx::generateSurface(isosurface::Parameters parameters) {
  auto calc = new volume::IsosurfaceCalculator(this);
  calc->setTaskManager(m_taskManager);
  Scene *scene = project->currentScene();
  parameters.structure = scene->chemicalStructure();
  calc->start(parameters);
}

void Crystalx::generateSurfaceRequiringWavefunction(
    isosurface::Parameters parameters, wfn::Parameters wfn_parameters) {
  Scene *scene = project->currentScene();
  if (!scene)
    return;
  auto *structure = scene->chemicalStructure();
  if (!structure)
    return;
  qDebug() << "In generateSurfaceRequiringWavefunction";

  if (wfn_parameters.accepted) {
    generateSurface(parameters);
    return;
  }

  qDebug() << "Generate new wavefunction";
  // NEW Wavefunction
  wfn_parameters = Crystalx::getWavefunctionParametersFromUser(
      m_surfaceGenerationDialog->atomIndices(), wfn_parameters.charge,
      wfn_parameters.multiplicity);
  wfn_parameters.structure = structure;
  // Still not valid
  if (!wfn_parameters.accepted)
    return;
  qDebug() << "Make calculator";
  WavefunctionCalculator *wavefunctionCalc = new WavefunctionCalculator();
  wavefunctionCalc->setTaskManager(m_taskManager);

  connect(wavefunctionCalc, &WavefunctionCalculator::calculationComplete, this,
          [parameters, this, wavefunctionCalc]() {
            auto params_tmp = parameters;
            params_tmp.wfn = wavefunctionCalc->getWavefunction();
            qDebug() << "Wavefunction set to: " << params_tmp.wfn;

            generateSurface(params_tmp);
            wavefunctionCalc->deleteLater();
          });

  wavefunctionCalc->start(wfn_parameters);
}

void Crystalx::showLoadingMessageBox(QString msg) {
  if (loadingMessageBox == 0) {
    loadingMessageBox = new QMessageBox(this);
  }
  loadingMessageBox->setText(msg);
  loadingMessageBox->setStandardButtons(QMessageBox::NoButton);
  loadingMessageBox->setIcon(QMessageBox::Information);
  loadingMessageBox->show();
}

void Crystalx::hideLoadingMessageBox() { loadingMessageBox->hide(); }

wfn::Parameters Crystalx::getWavefunctionParametersFromUser(
    const std::vector<GenericAtomIndex> &atoms, int charge, int multiplicity) {
  auto *structure = project->currentStructure();
  if (!structure)
    return {};

  if (!wavefunctionCalculationDialog) {
    wavefunctionCalculationDialog = new WavefunctionCalculationDialog(this);
  }

  qDebug() << atoms.size() << "Atoms for wavefunction";

  wavefunctionCalculationDialog->setAtomIndices(atoms);
  wavefunctionCalculationDialog->setCharge(charge);
  wavefunctionCalculationDialog->setMultiplicity(multiplicity);

  if (wavefunctionCalculationDialog->exec() == QDialog::Accepted) {
    auto params = wavefunctionCalculationDialog->getParameters();
    params.accepted = true;
    return params;
  }

  return {};
}

void Crystalx::handleGenerateWavefunctionAction() {
  auto structure = project->currentStructure();
  if (structure) {
    auto params = getWavefunctionParametersFromUser(
        structure->atomsWithFlags(AtomFlag::Selected), 0, 1);
    if (params.accepted)
      generateWavefunction(params);
  }
}

void Crystalx::handleLoadWavefunctionAction() {
  auto *structure = project->currentStructure();
  if (!structure) {
    QMessageBox::warning(
        this, tr("Load Wavefunction"),
        tr("No structure loaded. Please load a structure first."));
    return;
  }

  auto selectedAtoms = structure->atomsWithFlags(AtomFlag::Selected);
  if (selectedAtoms.empty()) {
    QMessageBox::warning(this, tr("Load Wavefunction"),
                         tr("No atoms selected. Please select atoms first."));
    return;
  }

  const QString FILTER = tr(
      "Wavefunction Files (*.molden *.molden.input *.fchk *.json *.wfn *.wfx)");
  QString filename = QFileDialog::getOpenFileName(
      this, tr("Load Wavefunction File"), QDir::currentPath(), FILTER);

  if (filename.isEmpty()) {
    return;
  }

  auto *wavefunction = io::loadWavefunction(filename);
  if (wavefunction) {
    // Show warning about unchecked atom mapping
    QMessageBox::StandardButton reply = QMessageBox::warning(
        this, tr("Load Wavefunction - Warning"),
        tr("Wavefunction loaded successfully.\n\n"
           "⚠️  WARNING: Atom mapping has NOT been validated.\n\n"
           "The wavefunction will be associated with the currently selected atoms, "
           "but the correspondence between wavefunction atoms and structure atoms "
           "has not been checked. This may result in incorrect surface properties "
           "or molecular orbital visualizations if the atoms don't match.\n\n"
           "Please verify that:\n"
           "• The number of selected atoms matches the wavefunction\n"
           "• The atom types and positions correspond correctly\n"
           "• The molecular geometry is consistent\n\n"
           "Do you want to proceed?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    
    if (reply == QMessageBox::No) {
      delete wavefunction;
      return;
    }
    
    // Prompt user for method and basis set
    QDialog methodDialog(this);
    methodDialog.setWindowTitle(tr("Wavefunction Method and Basis Set"));
    methodDialog.setModal(true);
    
    QVBoxLayout *layout = new QVBoxLayout(&methodDialog);
    
    // Add instruction label
    QLabel *instructionLabel = new QLabel(tr("Please specify the method and basis set used for this wavefunction:"), &methodDialog);
    layout->addWidget(instructionLabel);
    
    // Method input
    QHBoxLayout *methodLayout = new QHBoxLayout();
    QLabel *methodLabel = new QLabel(tr("Method:"), &methodDialog);
    QLineEdit *methodEdit = new QLineEdit("b3lyp", &methodDialog);
    methodLayout->addWidget(methodLabel);
    methodLayout->addWidget(methodEdit);
    layout->addLayout(methodLayout);
    
    // Basis set input
    QHBoxLayout *basisLayout = new QHBoxLayout();
    QLabel *basisLabel = new QLabel(tr("Basis Set:"), &methodDialog);
    QLineEdit *basisEdit = new QLineEdit("def2-svp", &methodDialog);
    basisLayout->addWidget(basisLabel);
    basisLayout->addWidget(basisEdit);
    layout->addLayout(basisLayout);
    
    // Add some common suggestions as labels
    QLabel *suggestionsLabel = new QLabel(tr("Common methods: B3LYP, PBE0, M06-2X, MP2, CCSD(T)\n"
                                             "Common basis sets: def2-SVP, def2-TZVP, 6-31G(d,p), cc-pVDZ"), &methodDialog);
    suggestionsLabel->setStyleSheet("color: #666; font-size: 10pt;");
    suggestionsLabel->setWordWrap(true);
    layout->addWidget(suggestionsLabel);
    
    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &methodDialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &methodDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &methodDialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    // Show dialog and get result
    if (methodDialog.exec() != QDialog::Accepted) {
      delete wavefunction;
      return;
    }
    
    QString userMethod = methodEdit->text().trimmed();
    QString userBasis = basisEdit->text().trimmed();
    
    // Validate input
    if (userMethod.isEmpty()) {
      userMethod = "Unknown";
    }
    if (userBasis.isEmpty()) {
      userBasis = "Unknown";
    }
    
    // Create a basic parameters object for the loaded wavefunction
    wfn::Parameters params;
    params.structure = structure;
    params.atoms = selectedAtoms;
    params.method = userMethod;
    params.basis = userBasis;
    params.accepted = true;

    wavefunction->setParameters(params);
    wavefunction->setObjectName(QFileInfo(filename).baseName());
    wavefunction->setParent(structure);

    showStatusMessage(
        tr("Wavefunction loaded successfully from: %1").arg(filename));
  } else {
    QMessageBox::critical(
        this, tr("Load Wavefunction"),
        tr("Failed to load wavefunction from file: %1").arg(filename));
  }
}

/*!
 This routine gets called when a surface needs or the user asks for a new
 wavefunction calculation
 It works in tandem with Crystal::backToSurfaceGeneration
 */
void Crystalx::generateWavefunction(wfn::Parameters parameters) {
  auto *structure = project->currentStructure();
  Q_ASSERT(structure);

  // TODO
  // Check if the wavefunction calculation duplicates an existing wavefunction
  // If YES, check with the user whether they want to continue anyway
  // (They may want to do this if they are going to edit the input file and ask
  // for special options.)
  bool generateWavefunction = true;
  /*
  auto wfn = crystal->wavefunctionMatchingParameters(newJobParams);
  if (wfn) {
    QString wavefunctionDescription = (*wfn).description();
    QString question =
        QString("Wavefunction %1 already exists.\n\nDo you want to continue "
                "with the wavefunction calculation?\nNote: This will replace "
                "the existing wavefunction.")
            .arg(wavefunctionDescription);
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Wavefunction Calculation", question,
                                  QMessageBox::Yes | QMessageBox::No);
    generateWavefunction = (reply == QMessageBox::Yes);
  }
  */

  if (!generateWavefunction)
    return;

  parameters.structure = structure;
  WavefunctionCalculator *calc = new WavefunctionCalculator();
  calc->setTaskManager(m_taskManager);
  calc->start(parameters);
}

void Crystalx::showCifFile() {
  auto *structure = project->currentScene()->chemicalStructure();
  if (structure) {
    QString filename = structure->filename();
    viewFile(filename, 800, 600, true);
  }
}

void Crystalx::viewFile(QString filename, int width, int height,
                        bool syntaxHighlight) {
  // Check to see file exists
  if (!QFile::exists(filename)) {
    return;
  }

  if (fileWindow == 0) {
    fileWindow = new QWidget(0);
    fileWindow->setWindowFlags(Qt::Tool);
    fileViewer = new QTextEdit(fileWindow);
    fileViewerLayout = new QVBoxLayout;
  }

  fileWindow->setWindowTitle(filename);
  fileViewer->setAcceptRichText(false);

  QFile file(filename);
  if (file.open(QIODevice::ReadOnly)) {
    QTextStream ts(&file);
    if (syntaxHighlight) {
      QString text;
      while (!ts.atEnd()) {
        QString lineOfText = ts.readLine();
        lineOfText = lineOfText + "<br>";
        lineOfText = lineOfText.replace(" ", "&nbsp;");
        /* Colorhighlighting of lines with special words */
        /* These could be made static and individualised for different file
         * formats: CIF, stdin, stdout */
        /* These could also be made user settable */
        colorHighlightHtml(lineOfText, "data_", "magenta");
        colorHighlightHtml(lineOfText, "loop_", "red");
        colorHighlightHtml(lineOfText, "_symmetry_space_group_name", "blue");
        colorHighlightHtml(lineOfText, "_symmetry_cell_setting", "blue");
        colorHighlightHtml(lineOfText, "_cell_length", "blue");
        colorHighlightHtml(lineOfText, "_cell_angle", "blue");
        text += lineOfText;
      }
      fileViewer->setHtml(text);
    } else {
      fileViewer->setText(ts.readAll());
    }

    fileViewer->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    fileViewer->setMinimumHeight(height);
    fileViewer->setMinimumWidth(width);

    fileViewerLayout->addWidget(fileViewer);
    fileWindow->setLayout(fileViewerLayout);
    fileWindow->show();
  } else {
    QMessageBox::warning(this, "Error", "Unable to display file: " + filename);
  }
}

void Crystalx::colorHighlightHtml(QString &lineOfText, QString regExp,
                                  QString htmlColor) {
  if (lineOfText.trimmed().contains(QRegularExpression(regExp))) {
    lineOfText =
        "<font color=\"" + htmlColor + "\"><b>" + lineOfText + "</b></font>";
  }
}

void Crystalx::updateWindowTitle() {
  QString title{cx::globals::mainWindowTitle};
  if (project->previouslySaved()) {
    QFileInfo fi(project->saveFilename());
    title += " - " + fi.fileName();
    if (project->hasUnsavedChanges()) {
      title += "*";
    }
  } else {
    if (project->currentScene()) {
      title += " - " + project->currentScene()->title();
    } else {
      title += " - Untitled";
    }
  }

  setWindowTitle(title);
}

// Called when the current crystal changes or when the atom selection changes
void Crystalx::handleAtomSelectionChanged() {
  enableGenerateSurfaceAction(true);
  enableCalculateEnergiesAction(true);
  updateInfo(infoViewer->currentTab());
}

void Crystalx::handleMeshSelectionChanged() {
  allowCloneSurfaceAction();
  auto scene = project->currentScene();
  if (!scene)
    return;

  auto *structure = scene->chemicalStructure();
  if (!structure)
    return;

  // Try mesh instance first, then direct mesh
  auto *meshInstance = childPropertyController->getCurrentMeshInstance();
  if (meshInstance) {
    scene->setSelectedSurface(meshInstance);
    auto index = structure->treeModel()->indexFromObject(meshInstance);
    emit scene->clickedSurface(index);
  } else {
    // Handle direct mesh selection - find first MeshInstance child
    auto *mesh = childPropertyController->getCurrentMesh();
    if (mesh) {
      // Look for the first MeshInstance child of this mesh
      MeshInstance *firstInstance = nullptr;
      for (auto *child : mesh->children()) {
        if (auto *instance = qobject_cast<MeshInstance *>(child)) {
          firstInstance = instance;
          break;
        }
      }

      if (firstInstance) {
        scene->setSelectedSurface(firstInstance);
        auto index = structure->treeModel()->indexFromObject(firstInstance);
        emit scene->clickedSurface(index);
      } else {
        // Fallback: emit signal for the mesh itself
        auto index = structure->treeModel()->indexFromObject(mesh);
        emit scene->clickedSurface(index);
      }
    }
  }
}

void Crystalx::handleElasticTensorSelectionChanged() {
  if (infoViewer) {
    auto *currentTensor = childPropertyController->getCurrentElasticTensor();
    infoViewer->elasticTensorInfoDocument->updateElasticTensor(currentTensor);
  }
}

void Crystalx::enableGenerateSurfaceAction(bool enable) {
  if (!project->currentScene())
    return;
  bool reallyEnable = enable && project->currentScene()->hasSelectedAtoms();
  generateSurfaceAction->setEnabled(reallyEnable);
}

// Called when current surface changes or when the current crystal contents
// changes
void Crystalx::allowCloneSurfaceAction() {
  enableCloneSurfaceAction(true);
  enableCalculateEnergiesAction(true);
}

void Crystalx::enableCloneSurfaceAction(bool enable) {
  if (!project->currentScene())
    return;
  if (!project->currentScene()->chemicalStructure())
    return;
  Mesh *mesh = childPropertyController->getCurrentMesh();
  bool reallyEnable = enable && (mesh != nullptr);
  cloneSurfaceAction->setEnabled(reallyEnable);
}

// Called when current surface changes or when the current crystal contents
// changes
void Crystalx::allowCalculateEnergiesAction() {
  enableCalculateEnergiesAction(true);
}

void Crystalx::enableCalculateEnergiesAction(bool enable) {
  auto *scene = project->currentScene();
  if (!scene)
    return;
  auto *structure = scene->chemicalStructure();
  if (!structure)
    return;

  QString tooltip = "Calculate pairwise interaction energies...";

  const bool incompleteFragments = structure->hasIncompleteSelectedFragments();
  if (incompleteFragments)
    tooltip += "\nComplete all fragments to enable this action.";
  const size_t selectedFragmentCount = structure->selectedFragments().size();
  if (selectedFragmentCount < 1)
    tooltip += "\nSelect one or more fragments to enable this action.";

  bool selectionOk = (!incompleteFragments) && (selectedFragmentCount >= 1);
  bool reallyEnable = enable && selectionOk;

  calculateEnergiesAction->setToolTip(tooltip);
  calculateEnergiesAction->setEnabled(reallyEnable);
}

/// When the current crystal changes and we are showing
/// the Close Contact Dialog we need to update
/// the Comboboxes. The comboboxes need to reflect the chemical elements
/// present in the structure.
void Crystalx::updateCloseContactOptions() {
  auto *scene = project->currentScene();
  if (!scene)
    return;
  auto *structure = scene->chemicalStructure();
  if (!structure)
    return;

  QStringList elements = structure->uniqueElementSymbols();
  QStringList hydrogenDonors = structure->uniqueHydrogenDonorElements();
  m_closeContactDialog->updateDonorsAndAcceptors(elements, hydrogenDonors);
}

void Crystalx::displayFingerprint() {
  passCurrentCrystalToFingerprintWindow();
  fingerprintWindow->show();
}

/*!
 Ugly hack. This routine gets called when the current surface is changed.
 Previously the new surface was passed to the fingerprint window.
 However the fingerprint window/plot needs not just the current
 surface but the current crystal because we need access to the
 atoms for fingerprint filtering.
 */
void Crystalx::passCurrentCrystalToFingerprintWindow() {
  Mesh *mesh = childPropertyController->getCurrentMesh();
  fingerprintWindow->setMesh(mesh);
  fingerprintWindow->setScene(project->currentScene());
  //  skwolff:  Idea for multiple fingerprint windows.
  //	FingerprintWindow* fw = new FingerprintWindow(this);
  //	fw->setCrystal(project->currentCrystal());
  //	fw->show();
}

void Crystalx::setMoleculeStyleForCurrent() {
  Scene *scene = project->currentScene();
  if (scene) {
    QAction *action = qobject_cast<QAction *>(sender());
    DrawingStyle drawingStyle =
        m_drawingStyleLabelToDrawingStyle[action->text()];
    scene->setDrawingStyle(drawingStyle);
    glWindow->redraw();
    showStatusMessage("Set molecule style to " + action->text());
  }
  updateMenuOptionsForScene();
}

void Crystalx::setEllipsoidStyleWithProbabilityForCurrent() {
  Scene *scene = project->currentScene();
  if (scene) {
    QAction *action = qobject_cast<QAction *>(sender());
    DrawingStyle drawingStyle = DrawingStyle::Ortep;
    scene->setDrawingStyle(drawingStyle);
    scene->updateThermalEllipsoidProbability(action->text().toDouble());
    glWindow->redraw();
  }
  updateMenuOptionsForScene();
}

void Crystalx::toggleDrawHydrogenEllipsoids(bool draw) {
  Scene *scene = project->currentScene();
  if (scene) {
    scene->toggleDrawHydrogenEllipsoids(draw);
    glWindow->redraw();
  }
  // updateMenuOptionsForCrystal();
}

void Crystalx::updateMenuOptionsForScene() {
  Scene *scene = project->currentScene();
  if (scene) {
    m_thermalEllipsoidMenu->setEnabled(true);
    QString moleculeStyleString = drawingStyleLabel(scene->drawingStyle());
    if (scene->drawingStyle() == DrawingStyle::Ortep) {
      moleculeStyleString =
          QString::number(scene->getThermalEllipsoidProbability(), 'f', 2);
    }
    foreach (QAction *action, moleculeStyleActions) {
      action->setChecked(action->text() == moleculeStyleString);
    }
    m_drawHEllipsoidsAction->setChecked(scene->drawHydrogenEllipsoids());
    showUnitCellAxesAction->setChecked(scene->showCells());
    auto labelOpts = scene->atomLabelOptions();
    showAtomicLabelsAction->setChecked(labelOpts.showAtoms);
    showFragmentLabelsAction->setChecked(labelOpts.showFragment);
    showHydrogenAtomsAction->setChecked(scene->showHydrogenAtoms());
  }
}

void Crystalx::handleAtomLabelActions() {
  AtomLabelOptions opts;
  opts.showAtoms = showAtomicLabelsAction->isChecked();
  opts.showFragment = showFragmentLabelsAction->isChecked();
  glWindow->handleAtomLabelOptionsChanged(opts);
}

void Crystalx::newProject() {
  if (closeProjectConfirmed()) {
    glWindow->pauseRendering();
    project->reset();
    if (childPropertyController)
      childPropertyController->reset();
    glWindow->setCurrentCrystal(project);
    glWindow->resumeRendering();
  }
}

void Crystalx::saveProject() {
  if (project->previouslySaved()) {
    project->saveToFile(project->saveFilename());
  } else {
    saveProjectAs();
  }
}

void Crystalx::saveProjectAs() {
  if (project->currentScene()) {
    QString filter = "CrystalExplorer Project(*." + PROJECT_EXTENSION + ")";
    QString filename = QFileDialog::getSaveFileName(
        0, tr("Save Project"), suggestedProjectFilename(), filter);

    if (!filename.isEmpty()) {
      bool success = project->saveToFile(filename);
      if (success) {
        addFileToHistory(filename);
        showStatusMessage("Saved project to " + filename);
      }
    }
  }
}

void Crystalx::exportAs() {
  if (!project->currentScene())
    return;

  QFileInfo fi(project->currentScene()->title());
  QString suggestedFilename = fi.baseName() + ".png";

  QImage previewImage = glWindow->renderToImage(1);
  m_exportDialog->updateImage(previewImage);

  // TODO fix this
  if (m_exportDialog->currentFilePath().isEmpty()) {
    m_exportDialog->updateFilePath(suggestedFilename);
  }

  // Only update the background color if it hasn't been set before
  if (!m_exportDialog->currentBackgroundColor().isValid()) {
    m_exportDialog->updateBackgroundColor(glWindow->backgroundColor());
  }

  if (m_exportDialog->exec() == QDialog::Accepted) {
    exportCurrentGraphics(m_exportDialog->currentFilePath());
  }
}

void Crystalx::quickExportCurrentGraphics() {
  if (m_exportDialog->currentFilePath().isEmpty()) {
    // If no previous export, call the full export dialog
    exportAs();
    return;
  }

  QFileInfo fi(m_exportDialog->currentFilePath());
  QString baseFilename = "frame";

  // Increment the export counter and create a new filename
  m_exportCounter++;
  QString newFilename =
      QString("%1_%2.png").arg(baseFilename).arg(m_exportCounter);
  QString fullPath = fi.dir().filePath(newFilename);

  exportCurrentGraphics(fullPath);
}

void Crystalx::exportCurrentGraphics(const QString &filename) {
  bool success = false;

  if (filename.toLower().endsWith(".png")) {
    QImage img =
        glWindow->exportToImage(m_exportDialog->currentResolutionScale(),
                                m_exportDialog->currentBackgroundColor());
    qDebug() << "Exporting image with scale factor"
             << m_exportDialog->currentResolutionScale() << "resolution"
             << img.size();
    success = img.save(filename);
  } else {
    QFile outputFile(filename);
    outputFile.open(QIODevice::WriteOnly);
    if (outputFile.isOpen()) {
      QTextStream outStream(&outputFile);
      success = glWindow->renderToPovRay(outStream);
    }
  }

  if (success) {
    showStatusMessage("Saved current graphics state to " + filename);
    // Update the dialog's file path for future exports
    m_exportDialog->updateFilePath(filename);
  } else {
    showStatusMessage("Failed to export current graphics state to " + filename);
  }
}

void Crystalx::handleExportCurrentGeometry() {
  if (!project->currentScene())
    return;

  QFileInfo fi(project->currentScene()->title());
  QString suggestedFilename = fi.baseName() + "_current.xyz";

  QString filter = "XYZ Files (*.xyz)";
  QString filename = QFileDialog::getSaveFileName(
      0, tr("Export current geometry"), suggestedFilename, filter);

  if (!filename.isEmpty()) {
    bool success = project->exportCurrentGeometryToFile(filename);
    if (success) {
      addFileToHistory(filename);
      showStatusMessage("Export geometry to" + filename);
    }
  }
}

void Crystalx::handleExportToGLTF() {
  if (!project->currentScene())
    return;

  auto* structure = project->currentScene()->chemicalStructure();
  if (!structure) {
    showStatusMessage("No chemical structure available for export");
    return;
  }

  QFileInfo fi(project->currentScene()->title());
  QString suggestedFilename = fi.baseName() + ".glb";

  QString filter = "Binary GLTF Files (*.glb);;GLTF Files (*.gltf)";
  QString filename = QFileDialog::getSaveFileName(
      this, tr("Export to GLTF"), suggestedFilename, filter);

  if (!filename.isEmpty()) {
    cx::core::GLTFExporter exporter;
    cx::core::GLTFExporter::ExportOptions options;

    // Set binary format based on file extension
    options.binaryFormat = filename.endsWith(".glb", Qt::CaseInsensitive);

    // Use scene export to get current display state including framework
    bool success = exporter.exportScene(project->currentScene(), filename, options);
    if (success) {
      showStatusMessage("Exported structure to " + filename);
    } else {
      showStatusMessage("Failed to export structure to " + filename);
    }
  }
}

QString Crystalx::suggestedProjectFilename() {
  QString filename;
  if (project->previouslySaved()) {
    filename = project->saveFilename();
  } else {
    auto *scene = project->currentScene();
    if (!scene)
      return filename;
    auto *structure = scene->chemicalStructure();
    if (!structure)
      return filename;

    QFileInfo fi(structure->filename());
    filename = fi.baseName() + "." + PROJECT_EXTENSION;
  }

  return filename;
}

void Crystalx::initPreferencesDialog() {
  if (preferencesDialog == nullptr) {
    preferencesDialog = new PreferencesDialog();
    connect(preferencesDialog, &PreferencesDialog::resetElementData, this,
            &Crystalx::resetElementData);
    connect(preferencesDialog,
            &PreferencesDialog::redrawCrystalForPreferencesChange, project,
            &Project::updateCurrentCrystalContents);
    // TODO fix None property color
    connect(preferencesDialog, &PreferencesDialog::nonePropertyColorChanged,
            project, &Project::updateCurrentCrystalContents);

    connect(preferencesDialog,
            &PreferencesDialog::redrawCloseContactsForPreferencesChange,
            glWindow, &GLWindow::redraw);
    connect(preferencesDialog,
            &PreferencesDialog::glwindowBackgroundColorChanged, glWindow,
            &GLWindow::updateBackgroundColor);
    connect(glWindow, &GLWindow::backgroundColorChanged, preferencesDialog,
            &PreferencesDialog::updateGlwindowBackgroundColor);
    connect(preferencesDialog, &PreferencesDialog::faceHighlightColorChanged,
            glWindow, &GLWindow::redraw);
    connect(preferencesDialog, &PreferencesDialog::setOpenglProjection,
            glWindow, &GLWindow::setPerspective);
    connect(preferencesDialog, &PreferencesDialog::selectionColorChanged,
            glWindow, &GLWindow::redraw);
    connect(preferencesDialog, &PreferencesDialog::screenGammaChanged, glWindow,
            &GLWindow::screenGammaChanged);
    connect(preferencesDialog, &PreferencesDialog::materialChanged, glWindow,
            &GLWindow::materialChanged);
    connect(preferencesDialog, &PreferencesDialog::lightSettingsChanged,
            glWindow, &GLWindow::lightSettingsChanged);
    connect(preferencesDialog, &PreferencesDialog::textSettingsChanged,
            glWindow, &GLWindow::textSettingsChanged);
    connect(preferencesDialog, &PreferencesDialog::debugVisualizationChanged,
            glWindow, &GLWindow::setDebugVisualizationEnabled);
    connect(preferencesDialog, &PreferencesDialog::glDepthTestEnabledChanged,
            glWindow, &GLWindow::updateDepthTest);
    connect(preferencesDialog, &PreferencesDialog::targetFramerateChanged,
            glWindow, &GLWindow::updateTargetFramerate);
    connect(preferencesDialog, &PreferencesDialog::experimentalFeaturesChanged,
            this, &Crystalx::enableExperimentalFeatures);
  }
}

void Crystalx::showPreferencesDialog() {
  if (preferencesDialog == 0) {
    initPreferencesDialog();
  }
  preferencesDialog->show();
  if (preferencesDialog->windowState() & Qt::WindowMinimized) {
    preferencesDialog->setWindowState(preferencesDialog->windowState() &
                                      ~Qt::WindowMinimized);
  }
  preferencesDialog->raise();
  preferencesDialog->activateWindow();
}

bool Crystalx::closeProjectConfirmed() {
  bool confirmed = true;

  if (project->hasUnsavedChanges()) {
    QMessageBox msgBox;
    msgBox.setText(
        "Do you want to save the changes to this project before closing?");
    msgBox.setInformativeText("If you don't, your changes will be lost.");
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard |
                              QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    int ret = msgBox.exec();

    switch (ret) {
    case QMessageBox::Save: // Save was clicked
      saveProject();
      break;
    case QMessageBox::Discard: // Don't Save was clicked
      break;
    case QMessageBox::Cancel: // Cancel was clicked
      confirmed = false;
      break;
    default:
      Q_ASSERT(false);
      break;
    }
  }

  if (confirmed) {
    infoViewer->hide();
  }

  return confirmed;
}

void Crystalx::quit() {
  settings::writeSetting(settings::keys::MAIN_WINDOW_SIZE, this->size());
  if (closeProjectConfirmed()) {
    QApplication::exit(0);
  }
}

void Crystalx::closeEvent(QCloseEvent *event) {
  quit();
  // If it makes it past the quit, because of user cancelling, then...
  event->ignore();
}

void Crystalx::showDepthFadingOptions() {
  initDepthFadingAndClippingDialog();
  depthFadingAndClippingDialog->showDialogWithDepthFadingTab();
}

void Crystalx::showClippingOptions() {
  initDepthFadingAndClippingDialog();
  depthFadingAndClippingDialog->showDialogWithClippingTab();
}

void Crystalx::initDepthFadingAndClippingDialog() {
  Q_ASSERT(glWindow != 0);

  if (depthFadingAndClippingDialog == nullptr) {
    depthFadingAndClippingDialog = new DepthFadingAndClippingDialog();
    connect(depthFadingAndClippingDialog,
            &DepthFadingAndClippingDialog::depthFadingSettingsChanged, glWindow,
            &GLWindow::updateDepthFading);
    connect(depthFadingAndClippingDialog,
            &DepthFadingAndClippingDialog::frontClippingPlaneChanged, glWindow,
            &GLWindow::updateFrontClippingPlane);
  }
}

void Crystalx::exportCurrentSurface() {
  auto *meshInstance = childPropertyController->getCurrentMeshInstance();
  if (!meshInstance) {
    QMessageBox::warning(this, "Export Surface",
                        "No surface selected. Please select a surface to export.");
    return;
  }

  Mesh *mesh = meshInstance->mesh();
  if (!mesh) {
    return;
  }

  QString suggestedName = mesh->objectName();
  if (suggestedName.isEmpty()) {
    suggestedName = "surface";
  }
  suggestedName += ".ply";

  QString filename = QFileDialog::getSaveFileName(
      this, "Export Surface", suggestedName,
      "PLY Files (*.ply);;All Files (*)");

  if (filename.isEmpty()) {
    return;
  }

  // Extract vertex colors from current renderer state
  std::vector<float> vertexColors;
  Scene *scene = project->currentScene();
  if (scene) {
    auto exportData = scene->getExportData();
    // Find the matching mesh in export data
    for (const auto &exportMesh : exportData.meshes()) {
      if (!exportMesh.colors.empty()) {
        vertexColors = exportMesh.colors;
        break; // Use first mesh with colors (should be the current one)
      }
    }
  }

  // Prepare metadata
  nlohmann::json metadata;
  metadata["description"] = mesh->objectName().toStdString();
  const auto &attr = mesh->attributes();
  metadata["kind"] = isosurface::kindToString(attr.kind).toStdString();
  metadata["isovalue"] = attr.isovalue;
  metadata["separation"] = attr.separation / occ::units::BOHR_TO_ANGSTROM;

  // Write using PlyWriter
  bool success =
      cx::io::PlyWriter::writeToFile(mesh, filename, vertexColors, metadata);

  if (success) {
    showStatusMessage(QString("Surface exported to %1").arg(filename));
  } else {
    QMessageBox::critical(
        this, "Export Failed",
        QString("Failed to export surface to:\n%1").arg(filename));
  }
}

void Crystalx::cloneSurface() {
  Scene *scene = project->currentScene();
  if (!scene) {
    qDebug() << "Clone surface called with no current scene";
    return;
  }

  Mesh *mesh = childPropertyController->getCurrentMesh();
  if (!mesh) {
    qDebug() << "Clone surface called with no current mesh";
    showStatusMessage("No surface selected for cloning");
    return;
  }

  auto *structure = scene->chemicalStructure();
  if (!structure) {
    qDebug() << "Clone surface called with no current structure";
    return;
  }

  showStatusMessage("Cloning surface...");

  int clonedCount = 0;
  auto selectedAtoms = structure->atomsWithFlags(AtomFlag::Selected);
  if (selectedAtoms.size() > 0) {
    auto *instance =
        MeshInstance::newInstanceFromSelectedAtoms(mesh, selectedAtoms);
    if (instance) {
      clonedCount++;
      qDebug() << "Cloned surface: " << instance;
    }
  } else {
    for (const auto &[fragIndex, fragment] : structure->getFragments()) {
      auto idxs = structure->atomIndicesForFragment(fragIndex);
      if (idxs.size() < 1)
        continue;
      auto *instance = MeshInstance::newInstanceFromSelectedAtoms(mesh, idxs);
      if (instance) {
        clonedCount++;
        qDebug() << "Cloned surface: " << instance;
      }
    }
  }

  // Ensure the scene updates to show the new surfaces
  scene->handleSurfacesNeedUpdate();
  glWindow->redraw();

  // Provide feedback to the user
  if (clonedCount > 0) {
    showStatusMessage(QString("Cloned %1 surface%2")
                          .arg(clonedCount)
                          .arg(clonedCount == 1 ? "" : "s"));
  } else {
    showStatusMessage("No surfaces were cloned");
  }
}

void Crystalx::showEnergyCalculationDialog() {
  qDebug() << "Show Energy calculation dialog";
  Scene *scene = project->currentScene();
  if (!scene)
    return;
  auto *structure = scene->chemicalStructure();
  if (!structure)
    return;

  // Energy calculations are not supported for slab structures
  if (structure->structureType() == ChemicalStructure::StructureType::Surface) {
    QMessageBox::information(this, "Energy Calculation", 
                           "Energy calculations are not available for slab structures.\n\n"
                           "Energy calculations are designed for analyzing intermolecular "
                           "interactions in 3D crystal structures and are not applicable "
                           "to 2D periodic slab structures.");
    return;
  }

  auto completeFragments = structure->completedFragments();
  qDebug() << "Complete fragments:" << completeFragments.size();
  auto selectedFragments = structure->selectedFragments();
  qDebug() << "Selected fragments:" << selectedFragments.size();

  const char *propName = "fragmentStatesSetByUser";
  const QVariant setByUser = structure->property(propName);

  if (!setByUser.isValid() || !setByUser.toBool()) {
    bool success = getFragmentStatesIfMultipleFragments(structure);
    qDebug() << "Success" << success;
    if (!success) {
      return; // User doesn't want us to continue so early return;
    }
    structure->setProperty(propName, true);
  }

  if (completeFragments.size() == 1) {
    const float CLUSTER_RADIUS = 3.8f; // angstroms
    QString question = QString("No pairs of fragments found.\n\nDo you want to "
                               "calculate interaction energies for a %1%2 "
                               "cluster around the selected fragment?")
                           .arg(CLUSTER_RADIUS, 0, 'f', 1)
                           .arg(cx::globals::angstromSymbol);
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Interaction Energy Calculation");
    msgBox.setText(question);
    msgBox.setIconPixmap(QIcon(":/images/radial_cluster.png").pixmap(64, 64));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() == QMessageBox::Yes) {
      project->showAtomsWithinRadius(CLUSTER_RADIUS, true);
      project->completeFragmentsForCurrentCrystal();
      completeFragments = structure->completedFragments();
      selectedFragments = structure->selectedFragments();
    } else {
      return; // User doesn't want us to continue so early return
    }
  }

  if (m_energyCalculationDialog == nullptr) {
    m_energyCalculationDialog = new EnergyCalculationDialog(this);

    connect(m_energyCalculationDialog,
            &EnergyCalculationDialog::energyParametersChosen, this,
            &Crystalx::calculatePairEnergies, Qt::UniqueConnection);
  }
  m_energyCalculationDialog->setChemicalStructure(structure);

  if (selectedFragments.size() > 0 && completeFragments.size() > 1) {
    m_energyCalculationDialog->show();
  } else {
    QString baseMessage = "Unable to calculate interaction "
                          "energies.\nCrystalExplorer can handle the following "
                          "cases:\n\n";
    QString cond1 = "1. One molecule on-screen, none selected.\n";
    QString cond2 =
        "2. Multiple molecules on-screen, central fragment selected.\n";
    QString cond3 = "3. A pair of selected fragments.";
    QString errorMessage = baseMessage + cond1 + cond2 + cond3;
    QMessageBox::warning(this, "Error", errorMessage);
  }
}

void Crystalx::showElasticTensorImportDialog() {
  if (!m_elasticTensorDialog) {
    m_elasticTensorDialog = new ElasticTensorDialog(this);
    
    connect(m_elasticTensorDialog, &QDialog::accepted, this, [this]() {
      // Get the elastic tensor results from the dialog
      ElasticTensorResults* tensorResults = m_elasticTensorDialog->elasticTensorResults();
      if (tensorResults) {
        Scene* scene = project->currentScene();
        if (scene) {
          ChemicalStructure* structure = scene->chemicalStructure();
          if (structure) {
            // Add the elastic tensor as a child of the chemical structure
            tensorResults->setParent(structure);

            // Show info viewer with the new tensor selected
            infoViewer->elasticTensorInfoDocument->updateElasticTensor(tensorResults);
            infoViewer->setTab(InfoType::ElasticTensor);
            infoViewer->show();

            statusBar()->showMessage(QString("Imported elastic tensor: %1").arg(tensorResults->name()), 3000);
          } else {
            QMessageBox::warning(this, "Import Error",
              "No chemical structure available. Please load a structure first.");
          }
        } else {
          QMessageBox::warning(this, "Import Error",
            "No scene available. Please create or load a project first.");
        }
      }
    });
  }
  
  m_elasticTensorDialog->show();
  m_elasticTensorDialog->raise();
  m_elasticTensorDialog->activateWindow();
}

void Crystalx::calculatePairEnergiesWithExistingWavefunctions(
    pair_energy::EnergyModelParameters modelParameters) {
  qDebug() << "Pairs needed:" << modelParameters.pairs.size();
  qDebug() << "Wavefunctions assumed to exist";
  Scene *scene = project->currentScene();
  if (!scene)
    return;
  auto *structure = scene->chemicalStructure();
  if (!structure)
    return;

  std::vector<MolecularWavefunction *> wavefunctions;
  for (const auto &wfn : modelParameters.wavefunctions) {
    auto candidates = structure->wavefunctionsAndTransformsForAtoms(wfn.atoms);
    bool found = false;
    qDebug() << "Found " << candidates.size() << "candidates";
    for (auto &candidate : candidates) {
      if (wfn.hasEquivalentMethodTo(candidate.wavefunction->parameters())) {
        found = true;
        wavefunctions.push_back(candidate.wavefunction);
        break;
      }
    }
    if (!found) {
      qDebug() << "Unable to find corresponding wavefunction...";
    }
  }

  std::vector<pair_energy::Parameters> energies;

  auto *pairInteractions = structure->pairInteractions();
  for (const auto &pair : modelParameters.pairs) {
    pair_energy::Parameters p;
    p.fragmentDimer = pair;
    p.structure = structure;
    p.atomsA = pair.a.atomIndices;
    p.atomsB = pair.b.atomIndices;
    p.model = modelParameters.model;

    bool foundA = false;
    bool foundB = false;
    for (auto *wfn : wavefunctions) {
      if (foundA && foundB)
        break;
      if (!foundA) {
        foundA = structure->getTransformation(wfn->atomIndices(), p.atomsA,
                                              p.transformA);
        if (foundA) {
          qDebug() << "Found wavefunction for A";
          p.wfnA = wfn;
        }
      }
      if (!foundB) {
        foundB = structure->getTransformation(wfn->atomIndices(), p.atomsB,
                                              p.transformB);
        if (foundB) {
          qDebug() << "Found wavefunction for B";
          p.wfnB = wfn;
        }
      }
    }
    if (!foundA && foundB) {
      qDebug() << "Unable to find wavefunctions for A and B";
      return;
    }

    QString model = modelParameters.model.toUpper();
    auto *existingInteraction = pairInteractions->getInteraction(model, pair);
    if (!existingInteraction) {
      energies.push_back(p);
    } else {
      qDebug() << "Found matching interaction:" << existingInteraction;
    }
  }

  PairEnergyCalculator *calc = new PairEnergyCalculator(this);
  calc->setTaskManager(m_taskManager);

  connect(calc, &PairEnergyCalculator::calculationComplete, this,
          [this, calc]() {
            qDebug() << "Calculation of pair energies complete";
            showInfo(InfoType::InteractionEnergy);
            calc->deleteLater();
          });

  calc->start_batch(energies);
}

void Crystalx::calculatePairEnergies(
    pair_energy::EnergyModelParameters modelParameters) {

  Scene *scene = project->currentScene();
  if (!scene)
    return;
  auto *structure = scene->chemicalStructure();
  if (!structure)
    return;
  qDebug() << "In calculatePairEnergies";

  qDebug() << "Wavefunctions needed:" << modelParameters.wavefunctions.size();
  qDebug() << "Pairs needed:" << modelParameters.pairs.size();
  qDebug() << "Model" << modelParameters.model;

  std::vector<wfn::Parameters> wavefunctionsToCalculate;
  for (auto &wfn : modelParameters.wavefunctions) {
    if (!wfn.accepted) {
      wfn = Crystalx::getWavefunctionParametersFromUser(wfn.atoms, wfn.charge,
                                                        wfn.multiplicity);
    }
    if (!wfn.accepted)
      return;
    break;
  }

  for (auto &wfn : modelParameters.wavefunctions) {
    wfn.structure = structure;
    auto candidates = structure->wavefunctionsAndTransformsForAtoms(wfn.atoms);
    bool found = false;
    for (auto &candidate : candidates) {
      if (wfn.hasEquivalentMethodTo(candidate.wavefunction->parameters())) {
        found = true;
        break;
      }
    }
    if (!found) {
      wavefunctionsToCalculate.push_back(wfn);
    }
  }

  if (wavefunctionsToCalculate.size() > 0) {
    qDebug() << "Make calculator";
    WavefunctionCalculator *wavefunctionCalc = new WavefunctionCalculator();
    wavefunctionCalc->setTaskManager(m_taskManager);

    connect(wavefunctionCalc, &WavefunctionCalculator::calculationComplete,
            this, [modelParameters, this, wavefunctionCalc]() {
              calculatePairEnergiesWithExistingWavefunctions(modelParameters);
              wavefunctionCalc->deleteLater();
            });

    wavefunctionCalc->start_batch(modelParameters.wavefunctions);
  } else {
    calculatePairEnergiesWithExistingWavefunctions(modelParameters);
  }
}

void Crystalx::handleSceneSelectionChange() { handleStructureChange(); }

void Crystalx::handleStructureChange() {
  auto *scene = project->currentScene();
  if (!scene) {
    childPropertyController->reset();
    clearAll();
    return;
  }
  auto *structure = scene->chemicalStructure();
  if (!structure) {
    childPropertyController->reset();
    clearAll();
    return;
  }
  qDebug() << "Structure changed";
  childPropertyController->setCurrentObject(structure);
  glWindow->redraw();
}

////////////////////////////////////////////////////////////////////////////////////
//
// Info Documents
//
////////////////////////////////////////////////////////////////////////////////////
void Crystalx::showInfoViewer() {
  infoViewer->show();
  updateInfo(infoViewer->currentTab());
}

void Crystalx::showInfo(InfoType infoType) {
  infoViewer->setTab(infoType);
  showInfoViewer();
}

void Crystalx::updateInfo(InfoType infoType) {
  Scene *scene = project->currentScene();
  if (!scene)
    return;

  if (infoViewer->isVisible()) {
    setInfoTabSpecificViewOptions(infoType);
    infoViewer->setScene(scene);
  }
}

void Crystalx::handleEnergyColorSchemeChanged() {
  togglePairInteractionHighlighting(true);
}

void Crystalx::togglePairInteractionHighlighting(bool state) {
  qDebug() << "Toggle pair interaction highlighting";
  Scene *scene = project->currentScene();
  if (!scene)
    return;
  scene->togglePairHighlighting(state);
  glWindow->redraw();
}

void Crystalx::setInfoTabSpecificViewOptions(InfoType infoType) {
  Scene *scene = project->currentScene();
  Q_ASSERT(scene);

  if (infoType == InfoType::InteractionEnergy) {
    scene->togglePairHighlighting(true);
  } else {
    scene->togglePairHighlighting(false);
  }
  glWindow->redraw();
}

void Crystalx::tidyUpAfterInfoViewerClosed() {
  Scene *crystal = project->currentScene();
  Q_ASSERT(crystal);

  if (crystal) {
    crystal->togglePairHighlighting(false);
  }
}

////////////////////////////////////////////////////////////////////////////////////
//
// Energy Structures
//
////////////////////////////////////////////////////////////////////////////////////

void Crystalx::showEnergyFrameworkDialog() {
  Scene *scene = project->currentScene();
  if (!scene)
    return;
  auto *structure = scene->chemicalStructure();
  if (!structure)
    return;
  auto *interactions = structure->pairInteractions();
  if (!interactions)
    return;
  if (!interactions->haveInteractions())
    return;

  if (childPropertyController) {
    childPropertyController->setCurrentPairInteractions(interactions);
    childPropertyController->toggleShowEnergyFramework();
  }
}

void Crystalx::cycleEnergyFrameworkBackwards() { cycleEnergyFramework(true); }

void Crystalx::cycleEnergyFramework(bool cycleBackwards) {
  qDebug() << "Todo cycle energy framework";
  /*
  Scene *crystal = project->currentScene();

  if (crystal && crystal->crystal()->hasInteractionEnergies()) {
    project->cycleEnergyFramework(cycleBackwards);
    frameworkDialog->setCurrentFramework(crystal->currentFramework());
  }
  */
}

void Crystalx::createSurfaceCut(int h, int k, int l, double offset,
                                double depth) {
  Scene *scene = project->currentScene();
  if (!scene) {
    qDebug() << "No current scene for surface cut";
    return;
  }

  auto *crystal = qobject_cast<CrystalStructure *>(scene->chemicalStructure());
  if (!crystal) {
    qDebug()
        << "Current structure is not a crystal - cannot create surface cut";
    return;
  }

  // Generate the surface cut using SlabStructure with specified depth
  SlabStructure *slab =
      cx::crystal::generateSurfaceCut(crystal, h, k, l, offset, depth);
  if (!slab) {
    qDebug() << "Failed to generate surface cut";
    return;
  }

  // Set a descriptive title
  QString title = QString("Surface cut (%1,%2,%3) offset=%4 depth=%5Å")
                      .arg(h)
                      .arg(k)
                      .arg(l)
                      .arg(offset, 0, 'f', 3)
                      .arg(depth, 0, 'f', 1);

  // Add the slab structure to the project
  project->addSlabStructure(slab, title);

  qDebug() << "Created surface cut:" << title;
}

////////////////////////////////////////////////////////////////////////////////////
//
// Charges
//
////////////////////////////////////////////////////////////////////////////////////

// This slot is connected to the "Set Fragment Charges" menu option
void Crystalx::setFragmentStates() {
  Scene *scene = project->currentScene();
  if (!scene)
    return;
  auto *structure = scene->chemicalStructure();

  if (structure) {
    getFragmentStatesFromUser(structure);
  }
}

bool Crystalx::getFragmentStatesIfMultipleFragments(
    ChemicalStructure *structure) {
  bool success = true;
  if (structure->symmetryUniqueFragments().size() > 1) {
    success = getFragmentStatesFromUser(structure);
  }
  return success;
}

bool Crystalx::getFragmentStatesFromUser(ChemicalStructure *structure) {
  if (!structure)
    return false;

  if (!m_fragmentStateDialog) {
    m_fragmentStateDialog = new FragmentStateDialog(this);
  }

  m_fragmentStateDialog->populate(structure);

  bool success = false;

  if (m_fragmentStateDialog->exec() == QDialog::Accepted) {
    if (m_fragmentStateDialog->hasFragmentStates()) {
      const auto &states = m_fragmentStateDialog->getFragmentStates();
      const auto &asymFrags = structure->symmetryUniqueFragments();
      int i = 0;
      for (const auto &[fragIndex, frag] : asymFrags) {
        structure->setSymmetryUniqueFragmentState(fragIndex, states[i]);
        i++;
      }
    }
    success = true;
  }

  return success;
}

void Crystalx::taskManagerTaskComplete(TaskID id) {
  showStatusMessage(QString("Task %1 complete").arg(id.toString()));
  int finished = m_taskManager->numFinished();
  int numTasks = m_taskManager->numTasks();
  updateProgressBar(finished, numTasks);
}

void Crystalx::taskManagerTaskError(TaskID id, QString errorMessage) {
  showStatusMessage(
      QString("Task %1 had error: %2").arg(id.toString()).arg(errorMessage));
  ;
  int finished = m_taskManager->numFinished();
  int numTasks = m_taskManager->numTasks();
  updateProgressBar(finished, numTasks);
}

void Crystalx::taskManagerTaskAdded(TaskID id) {
  showStatusMessage(QString("Task %1 added").arg(id.toString()));
  updateProgressBar(m_taskManager->numFinished(), m_taskManager->numTasks());
  setBusy(true);
}

void Crystalx::taskManagerTaskRemoved(TaskID id) {
  showStatusMessage(QString("Task %1 removed").arg(id.toString()));
  int finished = m_taskManager->numFinished();
  int numTasks = m_taskManager->numTasks();
  updateProgressBar(m_taskManager->numFinished(), m_taskManager->numTasks());
}

void Crystalx::showTaskManagerWidget() { m_taskManagerWidget->show(); }

void Crystalx::setupDragAndDrop() {
  setAcceptDrops(true);

  m_acceptedFileTypes << CIF_EXTENSION << CIF2_EXTENSION << PROJECT_EXTENSION
                      << XYZ_FILE_EXTENSION << "pdb" << "json" << "gin";
}

bool Crystalx::isFileAccepted(const QString &filePath) const {
  if (m_acceptedFileTypes.isEmpty()) {
    return true;
  }

  QFileInfo fileInfo(filePath);
  return m_acceptedFileTypes.contains(fileInfo.suffix().toLower());
}

void Crystalx::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) {
    bool hasValidFile = false;

    for (const QUrl &url : event->mimeData()->urls()) {
      if (url.isLocalFile() && isFileAccepted(url.toLocalFile())) {
        hasValidFile = true;
        break;
      }
    }

    if (hasValidFile) {
      event->acceptProposedAction();
    }
  }
}

void Crystalx::dragMoveEvent(QDragMoveEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

void Crystalx::dropEvent(QDropEvent *event) {
  if (event->mimeData()->hasUrls()) {
    QStringList filePaths;

    for (const QUrl &url : event->mimeData()->urls()) {
      if (url.isLocalFile()) {
        QString filePath = url.toLocalFile();

        if (isFileAccepted(filePath)) {
          filePaths << filePath;
        }
      }
    }

    for (const QString &filePath : filePaths) {
      openFilename(filePath);
    }

    event->acceptProposedAction();
  }
}

void Crystalx::calculateElasticTensor(const QString &modelName, double cutoffRadius) {
  Scene *scene = project->currentScene();
  if (!scene) {
    return;
  }

  auto *structure = scene->chemicalStructure();
  if (!structure) {
    return;
  }

  auto *interactions = structure->pairInteractions();
  if (!interactions || interactions->getCount() == 0) {
    QMessageBox::warning(this, "No Data",
                        "No pair interactions available. Please calculate pair energies first.");
    return;
  }

  // Create a temporary JSON file with the pair energies
  QTemporaryFile *tempFile = new QTemporaryFile(this);
  tempFile->setAutoRemove(false); // Keep for debugging if needed

  if (!tempFile->open()) {
    QMessageBox::critical(this, "Error",
                         "Failed to create temporary file for elastic tensor calculation.");
    delete tempFile;
    return;
  }

  QString tempJsonPath = tempFile->fileName();
  tempFile->close();

  // Export the current model's energies to the temp file (elastic_fit_pairs format)
  bool exported = save_elastic_fit_pairs_json(
      interactions, structure, modelName, tempJsonPath);

  if (!exported) {
    QMessageBox::critical(this, "Export Failed",
                         "Failed to export pair energies for elastic tensor calculation.");
    delete tempFile;
    return;
  }

  // Create tensor name with model and radius
  QString tensorName = QString("%1 (r=%2 \u00C5)").arg(modelName).arg(cutoffRadius, 0, 'f', 1);

  // Create and run the elastic tensor task
  auto *elasticTask = new OccElasticTensorTask(this);
  elasticTask->setProperty("name", QString("elastic_fit_%1").arg(modelName));
  elasticTask->setInputJsonFile(tempJsonPath);

  QString outputFile = elasticTask->outputJsonFilename();

  connect(elasticTask, &OccElasticTensorTask::completed, this,
          [this, tempFile, tensorName, outputFile, scene]() {
    // Load the elastic tensor from file
    QFile file(outputFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream in(&file);
      occ::Mat6 matrix;

      // Read 6x6 matrix from file
      for (int i = 0; i < 6; i++) {
        QString line = in.readLine();
        QStringList values = line.simplified().split(' ', Qt::SkipEmptyParts);
        if (values.size() >= 6) {
          for (int j = 0; j < 6; j++) {
            matrix(i, j) = values[j].toDouble();
          }
        }
      }
      file.close();

      // Create and add elastic tensor result to structure
      auto *structure = scene->chemicalStructure();
      if (structure) {
        auto *tensorResult = new ElasticTensorResults(matrix, tensorName, structure);

        // Show info viewer with the new tensor selected
        infoViewer->elasticTensorInfoDocument->updateElasticTensor(tensorResult);
        infoViewer->setTab(InfoType::ElasticTensor);
        infoViewer->show();

        statusBar()->showMessage(QString("Elastic tensor predicted: %1").arg(tensorName), 3000);
      }
    } else {
      QMessageBox::warning(this, "Load Failed",
                          QString("Elastic tensor calculated but failed to load from:\n%1")
                          .arg(outputFile));
    }
    tempFile->deleteLater();
  });

  connect(elasticTask, &OccElasticTensorTask::errorOccurred, this,
          [this, tempFile](const QString &error) {
    QMessageBox::critical(this, "Calculation Failed",
                         QString("Failed to calculate elastic tensor.\n\nError: %1")
                         .arg(error));
    tempFile->deleteLater();
  });

  m_taskManager->add(elasticTask);
}

void Crystalx::showLatticeEnergyDialog() {
  Scene *scene = project->currentScene();
  if (!scene) {
    QMessageBox::warning(this, "No Crystal Structure",
                        "Please load a crystal structure first.");
    return;
  }

  auto *structure = scene->chemicalStructure();
  if (!structure) {
    return;
  }

  // Check that we have a CIF file
  QString cifFile = structure->filename();
  if (cifFile.isEmpty()) {
    QMessageBox::warning(this, "No CIF File",
                        "No CIF file is associated with this structure.");
    return;
  }

  LatticeEnergyDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted) {
    QString model = dialog.selectedModel();
    double radius = dialog.radius();
    int threads = dialog.threads();
    calculateLatticeEnergy(model, radius, threads, cifFile);
  }
}

void Crystalx::calculateLatticeEnergy(const QString &modelName, double radius, int threads, const QString &cifFile) {
  // Create and run the lattice energy task
  auto *elatTask = new OccElatTask(this);
  elatTask->setProperty("name", QString("lattice_energy_%1").arg(modelName));
  elatTask->setCrystalStructureFile(cifFile);
  elatTask->setEnergyModel(modelName);
  elatTask->setRadius(radius);
  elatTask->setThreads(threads);

  QString outputFilename = elatTask->outputJsonFilename();
  QFileInfo cifInfo(cifFile);
  QString fullOutputPath = cifInfo.absolutePath() + "/" + outputFilename;

  Scene *scenePtr = project->currentScene();

  connect(elatTask, &OccElatTask::completed, this,
          [this, modelName, fullOutputPath]() {
    loadLatticeEnergyResults(fullOutputPath, modelName);
  });

  connect(elatTask, &OccElatTask::errorOccurred, this,
          [this](const QString &error) {
    QMessageBox::critical(this, "Calculation Failed",
                         QString("Failed to calculate lattice energy.\n\nError: %1")
                         .arg(error));
  });

  m_taskManager->add(elatTask);
}

void Crystalx::loadLatticeEnergyResults(const QString &filename, const QString &modelName) {
  Scene *scene = project->currentScene();
  if (!scene) {
    QMessageBox::warning(this, "No Scene",
                        "No active scene to load results into.");
    return;
  }

  auto *currentStructure = qobject_cast<CrystalStructure*>(scene->chemicalStructure());
  if (!currentStructure) {
    QMessageBox::warning(this, "Not a Crystal",
                        "Current structure is not a crystal structure.");
    return;
  }

  // Load the elat results structure
  auto *loadedStructure = io::loadCrystalClearJson(filename);
  if (!loadedStructure) {
    QMessageBox::critical(this, "Load Failed",
                         QString("Failed to load lattice energy results from:\n%1")
                         .arg(filename));
    return;
  }

  // Get the interactions from the loaded structure
  auto *loadedInteractions = loadedStructure->pairInteractions();
  if (!loadedInteractions || !loadedInteractions->haveInteractions()) {
    QMessageBox::warning(this, "No Data",
                        "No interaction data found in file.");
    delete loadedStructure;
    return;
  }

  // Get the interactions for the model
  auto modelInteractions = loadedInteractions->filterByModel(modelName);
  if (modelInteractions.empty()) {
    QMessageBox::warning(this, "No Data",
                        QString("No interactions found for model '%1' in file.")
                        .arg(modelName));
    delete loadedStructure;
    return;
  }

  // Now we need to get the raw data to call setPairInteractionsFromDimerAtoms
  // Since we can't extract it back out easily, just load the same file again
  // and extract the raw data ourselves
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::critical(this, "Load Failed",
                         QString("Failed to open file:\n%1").arg(filename));
    delete loadedStructure;
    return;
  }

  QByteArray data = file.readAll();
  file.close();

  try {
    auto json = nlohmann::json::parse(data.constData());

    bool hasPermutationSymmetry = true;
    if (json.contains("has_permutation_symmetry")) {
      hasPermutationSymmetry = json["has_permutation_symmetry"];
    }

    QList<QList<PairInteraction*>> interactions;
    QList<QList<DimerAtoms>> atomIndices;

    auto pairsArray = json["pairs"];
    interactions.resize(pairsArray.size());
    atomIndices.resize(pairsArray.size());

    for (size_t i = 0; i < pairsArray.size(); ++i) {
      auto siteEnergies = pairsArray[i];
      auto &neighbors = interactions[i];
      auto &offsets = atomIndices[i];
      neighbors.reserve(siteEnergies.size());
      offsets.reserve(siteEnergies.size());

      for (size_t j = 0; j < siteEnergies.size(); ++j) {
        auto *pair = new PairInteraction(modelName);
        pair_energy::Parameters params;
        params.hasPermutationSymmetry = hasPermutationSymmetry;
        pair->setParameters(params);

        const auto &dimerObj = siteEnergies[j];
        pair->setLabel(QString::number(j + 1));

        // Load energies
        const auto &energiesObj = dimerObj["energies"];
        for (auto it = energiesObj.begin(); it != energiesObj.end(); ++it) {
          QString key = QString::fromStdString(it.key());
          if (it->is_number()) {
            pair->addComponent(key, it->get<double>());
          }
        }

        // Load atom offsets
        const auto &offsetsObj = dimerObj["uc_atom_offsets"];
        DimerAtoms d;
        const auto &a = offsetsObj[0];
        d.a.reserve(a.size());
        for (size_t k = 0; k < a.size(); k++) {
          auto idx = a[k];
          d.a.push_back(GenericAtomIndex{idx[0], idx[1], idx[2], idx[3]});
        }
        const auto &b = offsetsObj[1];
        d.b.reserve(b.size());
        for (size_t k = 0; k < b.size(); k++) {
          auto idx = b[k];
          d.b.push_back(GenericAtomIndex{idx[0], idx[1], idx[2], idx[3]});
        }

        neighbors.push_back(pair);
        offsets.push_back(d);
      }
    }

    // Now set the interactions on the current structure
    currentStructure->setPairInteractionsFromDimerAtoms(interactions, atomIndices, hasPermutationSymmetry);

    // Update the info viewer to show the energies tab
    infoViewer->setTab(InfoType::InteractionEnergy);

    statusBar()->showMessage(
        QString("Lattice energy loaded for model '%1'").arg(modelName), 3000);

  } catch (const std::exception &e) {
    QMessageBox::critical(this, "Parse Error",
                         QString("Failed to parse lattice energy file:\n%1\n\nError: %2")
                         .arg(filename, e.what()));
  }

  delete loadedStructure;
}
