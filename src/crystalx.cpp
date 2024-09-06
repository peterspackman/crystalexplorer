#include <QAction>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QInputDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>
#include <QRegularExpression>
#include <QTextBrowser>
#include <QUrl>
#include <QtDebug>

#include "confirmationbox.h"
#include "crystalx.h"
#include "dialoghtml.h"
#include "elementdata.h"
#include "exportdialog.h"
#include "isosurface_calculator.h"
#include "mathconstants.h"
#include "pair_energy_calculator.h"
#include "settings.h"
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
  enableExperimentalFeatures(ENABLE_EXPERIMENTAL_FEATURES);

  updateWorkingDirectories(".");
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
}

void Crystalx::createDockWidgets() {
  createCrystalControllerDockWidget();
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

  connect(
      crystalController, &CrystalController::childSelectionChanged,
      [&](QModelIndex index) {
        auto *mesh = crystalController->getChild<Mesh>(index);
        auto *meshinstance = crystalController->getChild<MeshInstance>(index);
        auto *wfn = crystalController->getChild<MolecularWavefunction>(index);
        auto *pairInteractions =
            crystalController->getChild<PairInteractions>(index);
        auto *chemicalStructure =
            crystalController->getChild<ChemicalStructure>(index);

        // check for mesh instance first
        if (mesh) {
          qDebug() << "Setting current mesh to " << mesh;
          childPropertyController->setCurrentMesh(mesh);
        } else if (meshinstance) {
          qDebug() << "Setting current mesh instance to " << meshinstance;
          childPropertyController->setCurrentMeshInstance(meshinstance);

        } else if (wfn) {
          qDebug() << "Setting current wfn to " << wfn;
          childPropertyController->setCurrentWavefunction(wfn);
        } else if (pairInteractions) {
          qDebug() << "Setting pair interactions to" << pairInteractions;
          childPropertyController->setCurrentPairInteractions(pairInteractions);
        } else if (chemicalStructure) {
          int frame = 0;
          auto prop = chemicalStructure->property("frame");
          if (prop.isValid())
            frame = prop.toInt();
          qDebug() << "Setting frame to " << frame;
          project->setCurrentFrame(frame);
        }
      });
}

void Crystalx::createCrystalControllerDockWidget() {
  crystalController = new CrystalController();
  crystalControllerDockWidget = new QDockWidget(tr("Structures"));
  crystalControllerDockWidget->setObjectName("crystalControllerDockWidget");
  crystalControllerDockWidget->setWidget(crystalController);
  crystalControllerDockWidget->setAllowedAreas(Qt::RightDockWidgetArea);
  crystalControllerDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
  crystalControllerDockWidget->adjustSize();
  // crystalControllerDockWidget->setFocus();

  connect(project, &Project::clickedSurface, crystalController,
          &CrystalController::handleChildSelectionChange);

  addDockWidget(Qt::RightDockWidgetArea, crystalControllerDockWidget);
}

void Crystalx::initConnections() {
  initMenuConnections();

  // Project connections - project changed in some way
  connect(project, &Project::projectChanged, crystalController,
          &CrystalController::update);

  connect(project, &Project::structureChanged, this,
          &Crystalx::handleStructureChange, Qt::QueuedConnection);

  connect(m_taskManager, &TaskManager::busyStateChanged, this,
          &Crystalx::handleBusyStateChange);

  // Project connections - current crystal changed in some way
  connect(project, &Project::selectedSceneChanged, crystalController,
          &CrystalController::handleSceneSelectionChange);
  connect(project, &Project::selectedSceneChanged, this,
          &Crystalx::handleSceneSelectionChange);
  connect(project, &Project::selectedSceneChanged,
          [&](int) { glWindow->setCurrentCrystal(project); });
  connect(project, &Project::projectSaved, this, &Crystalx::updateWindowTitle);
  connect(project, &Project::projectChanged, this,
          &Crystalx::updateWindowTitle);
  connect(project, &Project::selectedSceneChanged, this,
          &Crystalx::updateWindowTitle);
  connect(project, &Project::selectedSceneChanged, this,
          &Crystalx::handleAtomSelectionChanged);
  connect(project, &Project::selectedSceneChanged, this,
          &Crystalx::updateMenuOptionsForScene);
  connect(project, &Project::selectedSceneChanged, this,
          &Crystalx::updateCloseContactOptions);
  connect(project, &Project::selectedSceneChanged, this,
          &Crystalx::allowCloneSurfaceAction);
  connect(project, &Project::selectedSceneChanged, this,
          &Crystalx::updateCrystalActions);
  connect(project, &Project::selectedSceneChanged, infoViewer,
          &InfoViewer::updateInfoViewerForCrystalChange);
  connect(project, &Project::currentSceneChanged, glWindow, &GLWindow::redraw);
  connect(project, &Project::currentSceneChanged, this,
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
  connect(crystalController, &CrystalController::structureSelectionChanged,
          project, QOverload<int>::of(&Project::setCurrentCrystal));
  connect(crystalController, &CrystalController::deleteCurrentCrystal, project,
          &Project::removeCurrentCrystal);
  connect(crystalController, &CrystalController::deleteAllCrystals, project,
          &Project::removeAllCrystals);

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

  connect(actionExport_As, &QAction::triggered, this, &Crystalx::exportAs);

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
  connect(showAtomicLabelsAction, &QAction::toggled, project,
          &Project::toggleAtomicLabels);
  connect(showHydrogenAtomsAction, &QAction::toggled, project,
          &Project::toggleHydrogenAtoms);
  connect(showSuppressedAtomsAction, &QAction::toggled, project,
          &Project::toggleSuppressedAtoms);
  connect(cycleDisorderHighlightingAction, &QAction::triggered, project,
          &Project::cycleDisorderHighlighting);
  connect(energyFrameworksAction, &QAction::triggered, this,
          &Crystalx::showEnergyFrameworkDialog);

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
  connect(removeSelectedAtomsAction, &QAction::triggered, project,
          &Project::removeSelectedAtomsForCurrentCrystal);
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
  connect(generateCellsAction, &QAction::triggered, this,
          &Crystalx::generateSlab);
  connect(cloneSurfaceAction, &QAction::triggered, this,
          &Crystalx::cloneSurface);
  connect(calculateEnergiesAction, &QAction::triggered, this,
          &Crystalx::showEnergyCalculationDialog);
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
  connect(showCrystalPlanesAction, &QAction::triggered, this,
          &Crystalx::showCrystalPlaneDialog);
  connect(actionShowTaskManager, &QAction::triggered, this,
          &Crystalx::showTaskManagerWidget);

  connect(generateWavefunctionAction, &QAction::triggered, this,
          &Crystalx::handleGenerateWavefunctionAction);

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

  completeFragmentsAction->setEnabled(enable);
  generateCellsAction->setEnabled(enable);
  toggleContactAtomsAction->setEnabled(enable);
  showAtomsWithinRadiusAction->setEnabled(enable);
  generateWavefunctionAction->setEnabled(enable);

  distanceAction->setEnabled(enable);
  angleAction->setEnabled(enable);
  dihedralAction->setEnabled(enable);
  OutOfPlaneBendAction->setEnabled(enable);
  InPlaneBendAction->setEnabled(enable);
  calculateEnergiesAction->setEnabled(enable);
  infoAction->setEnabled(enable);
}

void Crystalx::initSurfaceActions() {
  generateSurfaceAction->setEnabled(false);
  cloneSurfaceAction->setEnabled(false);
}

void Crystalx::enableExperimentalFeatures(bool enable) {
  experimentalAction->setEnabled(enable);
  experimentalAction->setVisible(enable);
}

void Crystalx::gotoCrystalExplorerWebsite() {
  QDesktopServices::openUrl(QUrl(CE_URL));
}
void Crystalx::gotoHowToCiteCrystalExplorer() {
  QDesktopServices::openUrl(QUrl(CE_CITATION_URL));
}

void Crystalx::setShowAtomsWithinRadius() {
  if (project->currentScene()) {
    bool generateClusterForSelection = project->currentHasSelectedAtoms();

    QString title = "Show atoms within a radius...";

    QString label;
    QString msgStart =
        QString("Show atoms within a ") +
        DialogHtml::bold(QString("radius (") + ANGSTROM_SYMBOL + ")");
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
                    ANGSTROM_SYMBOL + ")</b> of the currently selected atoms:";
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

void Crystalx::clearCurrent() { crystalController->clearCurrentCrystal(); }

void Crystalx::clearAll() { crystalController->clearAllCrystals(); }

void Crystalx::generateSlab() {
  Q_ASSERT(project->currentScene());

  bool ok{false};
  auto slabOptions = CellLimitsDialog::getSlabGenerationOptions(0, "Generate slab", QString(), ok);

  if (ok) {
    m_savedSlabGenerationOptions = slabOptions; // save cell limits for use by cloneVoidSurface
    project->generateSlab(slabOptions);
  }
}

void Crystalx::helpAboutActionDialog() {
  QString message = "";
  message += DialogHtml::paragraph(CE_AUTHORS);
  message += DialogHtml::paragraph(
      "Website: " + DialogHtml::website(CE_URL, CE_URL) +
      DialogHtml::linebreak() + "Email: " +
      DialogHtml::email(CE_EMAIL, CE_EMAIL, "CrystalExplorer",
                        QString("Re: Build= %1").arg(CX_BUILD_DATE)) +
      DialogHtml::linebreak() + "Citation: " +
      DialogHtml::website(CE_CITATION_URL, "How to Cite CrystalExplorer"));
  message += DialogHtml::paragraph(
      QString(COPYRIGHT_NOTICE).arg(QDate::currentDate().year()));
  message += DialogHtml::paragraph(QString(CE_NAME) + " uses " +
                                   DialogHtml::website(OCC_URL, "OCC") +
                                   "<br/>by P.R. Spackman");
  message += DialogHtml::line();
  message += DialogHtml::paragraph(QString("Version:  %1").arg(CX_VERSION) +
                                   ",  Revision: " + CX_GIT_REVISION +
                                   DialogHtml::linebreak() +
                                   "Build:    " + CX_BUILD_DATE);
  QMessageBox::information(this, settings::APPLICATION_NAME, message);
}

void Crystalx::initMoleculeStyles() {
  // QActionGroup* moleculeStyleGroup = new QActionGroup(toolBar);
  const auto availableDrawingStyles = {
      DrawingStyle::Tube, DrawingStyle::BallAndStick, DrawingStyle::SpaceFill,
      DrawingStyle::WireFrame, DrawingStyle::Ortep};
  for (const auto &drawingStyle : availableDrawingStyles) {
    QString moleculeStyleString = drawingStyleLabel(drawingStyle);
    m_drawingStyleLabelToDrawingStyle[moleculeStyleString] = drawingStyle;
    if (drawingStyle == DrawingStyle::Ortep) {
      const QStringList probs{"0.5", "0.9", "0.99"};
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
                         " *.pdb" + " *.json" + ")";
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

  QFileInfo fileInfo(filename);
  QString extension = fileInfo.suffix().toLower();

  if (filename.endsWith("cg_results.json")) {
    showStatusMessage(
        QString("Loading crystal clear output from %1").arg(filename));
    project->loadCrystalClearJson(filename);
  } else if (filename.endsWith("surface.json")) {
    showStatusMessage(QString("Loading crystal surface from %1").arg(filename));
    project->loadCrystalClearSurfaceJson(filename);
  } else if (extension == CIF_EXTENSION || extension == CIF2_EXTENSION) {
    processCif(filename);
  } else if (extension == "pdb") {
    processPdb(filename);
  } else if (extension == PROJECT_EXTENSION) {
    loadProject(filename);
  } else if (extension == XYZ_FILE_EXTENSION) {
    loadXyzFile(filename);
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
  crystalController->setEnabled(!busy);
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
    setWindowIcon(QPixmap(":images/crystalexplorer.png"));
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
  auto candidates = structure->wavefunctionsAndTransformsForAtoms(atomIndices);
  m_surfaceGenerationDialog->setSuitableWavefunctions(candidates);
  m_surfaceGenerationDialog->show();
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
  QString title;
  if (project->previouslySaved()) {
    QFileInfo fi(project->saveFilename());
    title = QString(GLOBAL_MAINWINDOW_TITLE) + " - " + fi.fileName();
    if (project->hasUnsavedChanges()) {
      title += "*";
    }
  } else {
    if (project->currentScene()) {
      title = QString(GLOBAL_MAINWINDOW_TITLE) + " - " +
              project->currentScene()->title();
    } else {
      title = QString(GLOBAL_MAINWINDOW_TITLE) + " - Untitled";
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
    scene->setThermalEllipsoidProbabilityString(action->text());
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
      moleculeStyleString = scene->thermalEllipsoidProbabilityString();
    }
    foreach (QAction *action, moleculeStyleActions) {
      action->setChecked(action->text() == moleculeStyleString);
    }
    m_drawHEllipsoidsAction->setChecked(scene->drawHydrogenEllipsoids());
    showUnitCellAxesAction->setChecked(scene->showCells());
    showAtomicLabelsAction->setChecked(scene->showAtomLabels());
    showHydrogenAtomsAction->setChecked(scene->showHydrogenAtoms());
  }
}

void Crystalx::newProject() {
  if (closeProjectConfirmed()) {
    project->reset();
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

  ExportDialog dialog(this);
  QImage previewImage = glWindow->renderToImage(1);
  dialog.updateImage(previewImage);
  dialog.updateFilePath(suggestedFilename);
  dialog.updateBackgroundColor(glWindow->backgroundColor());

  QString filename{""};
  bool success = false;
  if (dialog.exec() == QDialog::Accepted) {
    // The user clicked OK
    filename = dialog.currentFilePath();
    int scaleFactor = dialog.currentResolutionScale();
    QColor backgroundColor = dialog.currentBackgroundColor();

    if (filename.toLower().endsWith(".png")) {
      QImage img = glWindow->exportToImage(scaleFactor, backgroundColor);
      qDebug() << "Exporting image with scale factor" << scaleFactor
               << "resolution" << img.size();
      success = img.save(filename);
    } else {
      QFile outputFile(filename);
      outputFile.open(QIODevice::WriteOnly);
      if (outputFile.isOpen()) {
        QTextStream outStream(&outputFile);
        success = glWindow->renderToPovRay(outStream);
      }
    }
  }

  if (!filename.isEmpty()) {
    showStatusMessage("Saved current graphics state to " + filename);
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
    connect(preferencesDialog, &PreferencesDialog::glDepthTestEnabledChanged,
            glWindow, &GLWindow::updateDepthTest);
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

void Crystalx::cloneSurface() {
  Scene *scene = project->currentScene();
  if (!scene) {
    qDebug() << "Clone surface called with no current scene";
    return;
  }

  Mesh *mesh = childPropertyController->getCurrentMesh();
  if (!mesh) {
    qDebug() << "Clone surface called with no current mesh";
    return;
  }

  auto *structure = scene->chemicalStructure();
  if (!structure) {
    qDebug() << "Clone surface called with no current structure";
    return;
  }

  auto selectedAtoms = structure->atomsWithFlags(AtomFlag::Selected);
  if (selectedAtoms.size() > 0) {
    auto *instance =
        MeshInstance::newInstanceFromSelectedAtoms(mesh, selectedAtoms);
    qDebug() << "Cloned surface: " << instance;

  } else {
    for (const auto &[fragIndex, fragment] : structure->getFragments()) {
      auto idxs = structure->atomIndicesForFragment(fragIndex);
      if (idxs.size() < 1)
        continue;
      auto *instance = MeshInstance::newInstanceFromSelectedAtoms(mesh, idxs);
      qDebug() << "Cloned surface: " << instance;
    }
  }
  crystalController->setSurfaceInfo(project);
}

void Crystalx::showEnergyCalculationDialog() {
  qDebug() << "Show Energy calculation dialog";
  Scene *scene = project->currentScene();
  if (!scene)
    return;
  auto *structure = scene->chemicalStructure();
  if (!structure)
    return;

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
                           .arg(ANGSTROM_SYMBOL);
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
    p.centroidDistance = pair.centroidDistance;
    p.nearestAtomDistance = pair.nearestAtomDistance;
    p.symmetry = pair.symmetry;

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

void Crystalx::handleSceneSelectionChange() {
  handleStructureChange();
}

void Crystalx::handleStructureChange() {
  ChemicalStructure *structure = project->currentScene()->chemicalStructure();
  if (structure) {
    qDebug() << "Structure changed";
    childPropertyController->setCurrentPairInteractions(structure->pairInteractions());
    for (auto *child : structure->children()) {
      auto *mesh = qobject_cast<Mesh *>(child);
      if (mesh) {
        childPropertyController->setCurrentMesh(mesh);
        break;
      }
    }
    glWindow->redraw();
    // update surface controller
    // update list of surfaces
  }
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

void Crystalx::showCrystalPlaneDialog() {
  if (m_planeGenerationDialog == nullptr) {
    m_planeGenerationDialog = new PlaneGenerationDialog(this);
  }

  Scene *scene = project->currentScene();
  if (scene == nullptr)
    return;
  auto *crystal = qobject_cast<CrystalStructure *>(scene->chemicalStructure());
  if (!crystal)
    return;

  m_planeGenerationDialog->setSpaceGroup(crystal->spaceGroup());
  m_planeGenerationDialog->loadPlanes(scene->crystalPlanes());

  if (m_planeGenerationDialog->exec() == QDialog::Accepted) {
    scene->setCrystalPlanes(m_planeGenerationDialog->planes());
  }
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
