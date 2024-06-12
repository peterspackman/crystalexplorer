#pragma once
#include <QCloseEvent>
#include <QDockWidget>
#include <QFutureWatcher>
#include <QMainWindow>
#include <QTextEdit>
#include <QtConcurrent>
#include <QMessageBox>

#include "crystalcontroller.h"
#include "fingerprintwindow.h"
#include "glwindow.h"
#include "project.h"
#include "childpropertycontroller.h"
#include "viewtoolbar.h"

#include "animationsettingsdialog.h"
#include "celllimitsdialog.h"
#include "fragmentstatedialog.h"
#include "closecontactsdialog.h"
#include "depthfadingandclippingdialog.h"
#include "energycalculationdialog.h"
#include "infoviewer.h"
#include "planegenerationdialog.h"
#include "preferencesdialog.h"
#include "surfacegenerationdialog.h"
#include "wavefunctioncalculationdialog.h"
#include "pair_energy_parameters.h"

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
const QString PROJECT_EXTENSION = "cxp";
const QString XYZ_FILE_EXTENSION = "xyz";

const int STATUSBAR_MSG_DELAY =
    2000; // length of time /msec that messages appear for in status bar

class Crystalx : public QMainWindow, public Ui::Crystalx {
  Q_OBJECT

public:
  Crystalx();

  void loadExternalFileData(QString);

public slots:
  void generateSurface(isosurface::Parameters);
  void generateSurfaceRequiringWavefunction(isosurface::Parameters, wfn::Parameters);
  wfn::Parameters getWavefunctionParametersFromUser(const std::vector<GenericAtomIndex>&, int, int);
  void generateWavefunction(wfn::Parameters);
  void jobRunning();
  void jobCancelled(QString);
  void uncheckContactAtomsAction();
  void displayFingerprint();
  void passCurrentCrystalToFingerprintWindow();

  void calculateEnergies(pair_energy::EnergyModelParameters);
  void calculateEnergiesWithExistingWavefunctions(pair_energy::EnergyModelParameters);

  void showInfo(InfoType);
  void showTaskManagerWidget();
  void updateInfo(InfoType);
  void setInfoTabSpecificViewOptions(InfoType);
  void tidyUpAfterInfoViewerClosed();
  void updateStatusMessage(QString);
  void updateProgressBar(int, int);
  void clearStatusMessage();

protected slots:
  void closeEvent(QCloseEvent *);

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
  void updateCloseContactOptions();
  void setMoleculeStyleForCurrent();
  void setEllipsoidStyleWithProbabilityForCurrent();
  void toggleDrawHydrogenEllipsoids(bool);
  void updateMenuOptionsForScene();
  void newProject();
  void saveProject();
  void saveProjectAs();
  void exportAs();
  QString suggestedProjectFilename();
  void showPreferencesDialog();
  void helpAboutActionDialog();
  void generateCells();
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

  void allowActionsThatRequireSelectedAtoms();
  void allowCloneSurfaceAction();
  void allowCalculateEnergiesAction();
  void showEnergyCalculationDialog();

  void showEnergyFrameworkDialog();
  void cycleEnergyFrameworkBackwards();
  void cycleEnergyFramework(bool cycleBackwards = false);

  void showCrystalPlaneDialog();

  void setFragmentStates();

  void backgroundTaskFinished();
  void handleTransformationMatrixUpdate();

  void taskManagerTaskComplete(TaskID);
  void taskManagerTaskError(TaskID, QString);
  void taskManagerTaskAdded(TaskID);
  void taskManagerTaskRemoved(TaskID);

  void handleStructureChange();
  void handleGenerateWavefunctionAction();

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
  void createCrystalControllerDockWidget();
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

  Project *project{nullptr};
  GLWindow *glWindow{nullptr};
  ViewToolbar *viewToolbar{nullptr};
  ChildPropertyController *childPropertyController{nullptr};
  CrystalController *crystalController{nullptr};
  SurfaceGenerationDialog *m_surfaceGenerationDialog{nullptr};
  WavefunctionCalculationDialog *wavefunctionCalculationDialog{nullptr};
  EnergyCalculationDialog *m_energyCalculationDialog{nullptr};
  FingerprintWindow *fingerprintWindow{nullptr};
  PreferencesDialog *preferencesDialog{nullptr};
  CloseContactDialog *m_closeContactDialog{nullptr};
  DepthFadingAndClippingDialog *depthFadingAndClippingDialog{nullptr};
  FragmentStateDialog *m_fragmentStateDialog{nullptr};
  InfoViewer *infoViewer{nullptr};
  PlaneGenerationDialog *m_planeGenerationDialog{nullptr};

  QDockWidget *childPropertyControllerDockWidget{nullptr};
  QDockWidget *crystalControllerDockWidget{nullptr};
  QAction *quitAction{nullptr};
  QAction *recentFileActions[MAXHISTORYSIZE];
  QVector<QAction *> moleculeStyleActions;
  QMessageBox *loadingMessageBox{nullptr};
  AnimationSettingsDialog *_animationSettingsDialog{nullptr};
  QAction *_clearRecentFileAction{nullptr};
  QMenu *_thermalEllipsoidMenu{nullptr};
  QAction *_drawHEllipsoidsAction{nullptr};

  QProgressBar *_jobProgress{nullptr};
  QToolButton *_jobCancel{nullptr};

  QWidget *fileWindow{nullptr};
  QTextEdit *fileViewer{nullptr};
  QVBoxLayout *fileViewerLayout{nullptr};

  QPair<QVector3D, QVector3D> _savedCellLimits;
  QMap<QString, DrawingStyle> m_drawingStyleLabelToDrawingStyle;
  QFutureWatcher<bool> m_futureWatcher;

  TaskManager * m_taskManager{nullptr};
  TaskManagerWidget * m_taskManagerWidget{nullptr};

};
