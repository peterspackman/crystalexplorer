#pragma once
#include <QAction>
#include <QDebug>
#include <QObject>
#include <QVector>

#include "jobparameters.h"
#include "packingdialog.h" // access to enum UnitCellPackingCriteria
#include "scene.h"

/*!
 \class Project
 \brief Contains all the data: crystals, surfaces etc.

 This eventually will allow us to save projects and return to them later
 */
class Project : public QObject {
  Q_OBJECT

  friend QDataStream &operator<<(QDataStream &, const Project &);
  friend QDataStream &operator>>(QDataStream &, Project &);

public:
  Project(QObject *parent = 0);
  ~Project();
  void reset();
  bool loadCrystalData(const JobParameters &);
  bool loadSurfaceData(const JobParameters &);
  Scene *currentScene();
  const Scene *currentScene() const;
  Scene *previousCrystal();
  QStringList sceneTitles();
  QList<ScenePeriodicity> scenePeriodicities() const;
  int currentCrystalIndex() { return m_currentSceneIndex; }
  bool saveToFile(QString);
  bool hasUnsavedChanges() { return m_haveUnsavedChanges; }
  bool loadFromFile(QString);

  bool loadChemicalStructureFromXyzFile(const QString &);
  bool loadCrystalStructuresFromCifFile(const QString &);

  bool previouslySaved();
  QString saveFilename();
  void removeContactAtoms();
  int numberOfCrystals() { return m_scenes.size(); }
  bool currentHasSelectedAtoms() const;
  bool currentHasSurface() const;
  void addMonomerEnergyToCurrent(const MonomerEnergy &m);

public slots:
  void setCurrentCrystal(int);
  void setCurrentCrystal(int, bool);
  void updateCurrentCrystalContents();
  void toggleVisibilityOfSurface(int);
  void setCurrentSurface(int);
  void setCurrentPropertyForCurrentSurface(int);
  void updatePropertyRangeForCurrentSurface(float, float);
  void setSurfaceTransparency(bool);
  void generateCells(QPair<QVector3D, QVector3D>);
  void removeAllCrystals();
  void removeCurrentCrystal();
  void deleteCurrentSurface();
  void confirmAndDeleteSurface(int);
  void completeFragmentsForCurrentCrystal();
  void toggleUnitCellAxes(bool);
  void toggleMultipleUnitCellBoxes(bool);
  void toggleAtomicLabels(bool);
  void toggleHydrogenAtoms(bool);
  void toggleSuppressedAtoms(bool);
  void toggleCloseContacts(bool);
  void toggleHydrogenBonds(bool);
  void toggleSurfaceCaps(bool);
  void updateNonePropertiesForAllCrystals();
  void updateHydrogenBondsForCurrent(QString, QString, double, bool);
  void cycleDisorderHighlighting();
  void cycleEnergyFramework(bool cycleBackwards = false);
  void updateEnergyFramework();
  void turnOffEnergyFramework();
  void updateEnergyTheoryForEnergyFramework(EnergyTheory);
  void toggleShowCurrentSurfaceInterior(bool);
  void toggleShowSurfaceInteriors(bool);

  void updateAllCrystalsForChangeInElementData();

  void toggleCC1(bool);
  void toggleCC2(bool);
  void toggleCC3(bool);
  void toggleCloseContact(int, bool);
  void updateCloseContactsForCurrent(int, QString, QString, double);
  void reportSurfaceVisibilitiesChanged();
  void removeIncompleteFragmentsForCurrentCrystal();
  void removeSelectedAtomsForCurrentCrystal();
  void resetCurrentCrystal();

  void showAtomsWithinRadius(float, bool);

  void hideSurface(int);
  void toggleAtomsForFingerprintSelectionFilter(bool);

  void suppressSelectedAtoms();
  void unsuppressSelectedAtoms();
  void selectAllAtoms();
  void selectSuppressedAtoms();
  void selectAtomsOutsideRadiusOfSelectedAtoms(float);
  void selectAtomsInsideCurrentSurface();
  void selectAtomsOutsideCurrentSurface();
  void invertSelection();

  void removeAllMeasurements();

signals:
  void projectChanged(Project *);
  void projectSaved();
  void currentCrystalChanged(Project *);
  void currentSceneChanged();
  void currentCrystalSurfacesChanged(Project *);
  void currentSurfaceVisibilityChanged(Project *);
  void currentSurfaceChanged(Surface *);
  void currentPropertyChanged(const SurfaceProperty *);
  void currentSurfacePropertyChanged();
  void currentSurfaceTransparencyChanged();
  void surfaceSelected(int);
  void currentSurfaceFaceSelected(float);
  void atomSelectionChanged();
  void contactAtomsTurnedOff();
  void currentCrystalHasNoSurfaces();
  void surfaceVisibilitiesChanged(Project *);
  void currentCrystalReset();
  void currentCrystalViewChanged();
  void newPropertyAddedToCurrentSurface();

private:
  void init();
  void initConnections();
  void tidyUpOutgoingScene();
  void connectUpCurrentScene();
  void deleteAllCrystals();
  void deleteCurrentCrystal();
  void setCurrentCrystalUnconditionally(int);
  QString projectFileVersion() const;
  QString projectFileCompatibilityVersion() const;
  void setUnsavedChangesExists();

  QVector<Scene *> m_scenes;
  int m_currentSceneIndex{-1};
  int m_previousSceneIndex{-1};
  QString _saveFilename;

  bool m_haveUnsavedChanges{false};
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &, const Project &);
QDataStream &operator>>(QDataStream &, Project &);
