#pragma once
#include <QCloseEvent>
#include <QDockWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QTextEdit>
#ifdef CX_HAS_CONCURRENT
#include <QFutureWatcher>
#include <QtConcurrent>
#endif

#include "childpropertycontroller.h"
#include "fingerprintwindow.h"
#include "glwindow.h"
#include "project.h"
#include "projectcontroller.h"
#include "viewtoolbar.h"

#include "animationsettingsdialog.h"
#include "celllimitsdialog.h"
#include "closecontactsdialog.h"
#include "crystalcutdialog.h"
#include "depthfadingandclippingdialog.h"
#include "elastictensordialog.h"
#include "energycalculationdialog.h"
#include "exportdialog.h"
#include "fragmentstatedialog.h"
#include "infoviewer.h"
#include "latticeenergydialog.h"
#include "pair_energy_parameters.h"
#include "planedialog.h"
#include "planegenerationdialog.h"
#include "preferencesdialog.h"
#include "surfacegenerationdialog.h"
#include "wavefunctioncalculationdialog.h"

// background tasks
#include "taskmanager.h"
#include "taskmanagerwidget.h"

#include "ui_crystalx.h"

const bool ENABLE_EXPERIMENTAL_FEATURES = false;
const bool EXPERIMENT_BUTTON_IS_TOGGLE = false;

const int MAXHISTORYSIZE = 10;

const QString CIF_EXTENSION = "cif";
const QString CIF2_EXTENSION = "cif2";
const QString CIFDATA_EXTENSION = "cxc";
const QString ATOMDATA_EXTENSION = "cxd";
const QString SURFACEDATA_EXTENSION = "cxs";
const QString ENERGYDATA_EXTENSION = "cxe";
const QString PROJECT_EXTENSION = "cxp.cbor";
const QString XYZ_FILE_EXTENSION = "xyz";

const int STATUSBAR_MSG_DELAY =
    2000; // length of time /msec that messages appear for in status bar

class Crystalx : public QMainWindow, public Ui::Crystalx {
  Q_OBJECT

public:
  Crystalx();

  void loadExternalFileData(QString);

public slots:
  void togglePairInteractionHighlighting(bool);
  void handleBusyStateChange(bool);
  void generateSurface(isosurface::Parameters);
  void generateSurfaceRequiringWavefunction(isosurface::Parameters,
                                            wfn::Parameters);
  wfn::Parameters
  getWavefunctionParametersFromUser(const std::vector<GenericAtomIndex> &, int,
                                    int);
  void generateWavefunction(wfn::Parameters);
  void jobRunning();
  void jobCancelled(QString);
  void uncheckContactAtomsAction();
  void displayFingerprint();
  void passCurrentCrystalToFingerprintWindow();

  void calculatePairEnergies(pair_energy::EnergyModelParameters);
  void calculatePairEnergiesWithExistingWavefunctions(
      pair_energy::EnergyModelParameters);

  void showInfo(InfoType);
  void showTaskManagerWidget();
  void updateInfo(InfoType);
  void setInfoTabSpecificViewOptions(InfoType);
  void tidyUpAfterInfoViewerClosed();
  void updateStatusMessage(QString);
  void updateProgressBar(int, int);
  void clearStatusMessage();
  void handleExportCurrentGeometry();
  void handleExportToGLTF();
  void handleAtomLabelActions();
  void handleMeshSelectionChanged();
  void handleEnergyColorSchemeChanged();
  void handleElasticTensorSelectionChanged();

protected slots:
  void closeEvent(QCloseEvent *) override;

private slots:
  bool resetElementData();
  void openFile();
  void processRotTransScaleAction(QAction *);
  void processMeasurementAction(QAction *);
  void resetSelectionMode();
  void fileFromHistoryChosen();
  void clearFileHistory();
  void openFilename(QString);
  void removeFileFromHistory(const QString &);
  void updateWindowTitle();
  void quit();
  void getSurfaceParametersFromUser();
  void showPlaneDialog();
  void showCrystalCutDialog();
  void handleCrystalCutDialogAccepted();
  void updateCloseContactOptions();
  void setMoleculeStyleForCurrent();
  void setEllipsoidStyleWithProbabilityForCurrent();
  void toggleDrawHydrogenEllipsoids(bool);
  void updateMenuOptionsForScene();
  void newProject();
  void saveProject();
  void saveProjectAs();
  void exportAs();
  void quickExportCurrentGraphics();
  QString suggestedProjectFilename();
  void showPreferencesDialog();
  void helpAboutActionDialog();
  void generateSlab();
  void generateSlabFromPlane(int h, int k, int l, double offset);
  void clearCurrent();
  void clearAll();
  void setAnimateScene(bool);
  void toggleAnimation(bool);
  void selectAtomsOutsideRadius();
  void setShowAtomsWithinRadius();
  void updateCrystalActions();

  void showInfoViewer();
  void showCifFile();
  void gotoCrystalExplorerWebsite();
  void gotoHowToCiteCrystalExplorer();

  void showDepthFadingOptions();
  void showClippingOptions();

  void cloneSurface();

  void handleAtomSelectionChanged();
  void allowCloneSurfaceAction();
  void allowCalculateEnergiesAction();
  void showEnergyCalculationDialog();
  void exportCurrentSurface();
  void showElasticTensorImportDialog();
  void showLatticeEnergyDialog();
  void calculateElasticTensor(const QString &modelName);
  void calculateLatticeEnergy(const QString &modelName, double radius, int threads, const QString &cifFile);
  void loadLatticeEnergyResults(const QString &filename, const QString &modelName);

  void showEnergyFrameworkDialog();
  void cycleEnergyFrameworkBackwards();
  void cycleEnergyFramework(bool cycleBackwards = false);

  void createSurfaceCut(int h, int k, int l, double offset, double depth);

  void setFragmentStates();

  void handleTransformationMatrixUpdate();

  void taskManagerTaskComplete(TaskID);
  void taskManagerTaskError(TaskID, QString);
  void taskManagerTaskAdded(TaskID);
  void taskManagerTaskRemoved(TaskID);

  void handleStructureChange();
  void handleSceneSelectionChange();
  void handleGenerateWavefunctionAction();
  void handleLoadWavefunctionAction();

private:
  void init();
  bool readElementData();
  void initStatusBar();
  void initMainWindow();
  void initMenus();
  void initMoleculeStyles();
  void initGlWindow();
  void initFingerprintWindow();
  void initInfoViewer();
  void createRecentFileActionsAndAddToFileMenu();
  void addExitOptionToFileMenu();
  void createToolbars();
  void createDockWidgets();
  void createChildPropertyControllerDockWidget();
  void createProjectControllerDockWidget();
  void initConnections();
  void initMenuConnections();
  void initActionGroups();
  void addFileToHistory(const QString &);
  void loadProject(QString);
  void updateRecentFileActions(QStringList);
  void updateWorkingDirectories(const QString &);
  void processCif(QString &);
  void processPdb(QString &);
  void loadXyzFile(const QString &);

  void showLoadingMessageBox(QString);
  void hideLoadingMessageBox();
  void processSuccessfulJob();
  void processSuccessfulEnergyCalculation();
  void initSurfaceActions();

  void setBusy(bool);
  void setBusyIcon(bool);
  void disableActionsWhenBusy(bool);
  void enableGenerateSurfaceAction(bool);
  void enableCloneSurfaceAction(bool);
  void enableCalculateEnergiesAction(bool);

  bool overrideBondLengths();
  void showStatusMessage(QString);
  void viewFile(QString, int width = 620, int height = 600,
                bool syntaxHighlight = false);
  bool closeProjectConfirmed();
  void initPreferencesDialog();
  void initCloseContactsDialog();
  void enableExperimentalFeatures(bool);

  void colorHighlightHtml(QString &lineOfText, QString regExp,
                          QString htmlColor);
  void initDepthFadingAndClippingDialog();

  bool getFragmentStatesIfMultipleFragments(ChemicalStructure *);
  bool getFragmentStatesFromUser(ChemicalStructure *);
  void exportCurrentGraphics(const QString &);

  void processDroppedFiles(const QStringList &filePaths);
  void setupDragAndDrop();

  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;

  bool isFileAccepted(const QString &filePath) const;

  Project *project{nullptr};
  GLWindow *glWindow{nullptr};
  ViewToolbar *viewToolbar{nullptr};
  ChildPropertyController *childPropertyController{nullptr};
  ProjectController *projectController{nullptr};
  SurfaceGenerationDialog *m_surfaceGenerationDialog{nullptr};
  WavefunctionCalculationDialog *wavefunctionCalculationDialog{nullptr};
  EnergyCalculationDialog *m_energyCalculationDialog{nullptr};
  FingerprintWindow *fingerprintWindow{nullptr};
  PreferencesDialog *preferencesDialog{nullptr};
  CloseContactDialog *m_closeContactDialog{nullptr};
  DepthFadingAndClippingDialog *depthFadingAndClippingDialog{nullptr};
  FragmentStateDialog *m_fragmentStateDialog{nullptr};
  InfoViewer *infoViewer{nullptr};
  PlaneDialog *m_planeDialog{nullptr};
  CrystalCutDialog *m_crystalCutDialog{nullptr};
  ElasticTensorDialog *m_elasticTensorDialog{nullptr};

  QDockWidget *childPropertyControllerDockWidget{nullptr};
  QDockWidget *projectControllerDockWidget{nullptr};
  QAction *quitAction{nullptr};
  QAction *recentFileActions[MAXHISTORYSIZE];
  QVector<QAction *> moleculeStyleActions;
  QMessageBox *loadingMessageBox{nullptr};
  AnimationSettingsDialog *_animationSettingsDialog{nullptr};
  QAction *_clearRecentFileAction{nullptr};
  QMenu *m_thermalEllipsoidMenu{nullptr};
  QAction *m_drawHEllipsoidsAction{nullptr};

  QProgressBar *_jobProgress{nullptr};
  QToolButton *_jobCancel{nullptr};

  QWidget *fileWindow{nullptr};
  QTextEdit *fileViewer{nullptr};
  QVBoxLayout *fileViewerLayout{nullptr};

  SlabGenerationOptions m_savedSlabGenerationOptions;
  QMap<QString, DrawingStyle> m_drawingStyleLabelToDrawingStyle;
  ExportDialog *m_exportDialog{nullptr};
  int m_exportCounter{0};

  TaskManager *m_taskManager{nullptr};
  TaskManagerWidget *m_taskManagerWidget{nullptr};

  bool m_showDropIndicator = false;
  QStringList m_acceptedFileTypes;
};
