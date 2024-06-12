#pragma once
#include <QAction>
#include <QDebug>
#include <QObject>
#include <QVector>
#include <QAbstractItemModel>

#include "packingdialog.h" // access to enum UnitCellPackingCriteria
#include "scene.h"

/*!
 \class Project
 \brief Contains all the data: crystals, surfaces etc.

 This eventually will allow us to save projects and return to them later
 */
class Project : public QAbstractItemModel {
  Q_OBJECT

  friend QDataStream &operator<<(QDataStream &, const Project &);
  friend QDataStream &operator>>(QDataStream &, Project &);

public:
  Project(QObject *parent = 0);
  ~Project();
  void reset();
  Scene *currentScene();
  ChemicalStructure *currentStructure();
  const Scene *currentScene() const;
  Scene *previousCrystal();
  QStringList sceneTitles();
  int currentCrystalIndex() { return m_currentSceneIndex; }
  bool saveToFile(QString);
  bool hasUnsavedChanges() { return m_haveUnsavedChanges; }
  bool loadFromFile(QString);

  bool loadChemicalStructureFromXyzFile(const QString &);
  bool loadCrystalStructuresFromCifFile(const QString &);
  bool loadCrystalStructuresFromPdbFile(const QString &);

  bool previouslySaved();
  QString saveFilename();
  int numberOfCrystals() { return m_scenes.size(); }
  bool currentHasSelectedAtoms() const;
  bool currentHasSurface() const;

  // Abstract Item Model methods
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &index) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

public slots:
  void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
  void setCurrentCrystal(int);
  void setCurrentCrystal(int, bool);
  void updateCurrentCrystalContents();
  void generateCells(QPair<QVector3D, QVector3D>);
  void removeAllCrystals();
  void removeCurrentCrystal();
  void completeFragmentsForCurrentCrystal();
  void toggleUnitCellAxes(bool);
  void toggleMultipleUnitCellBoxes(bool);
  void toggleAtomicLabels(bool);
  void toggleHydrogenAtoms(bool);
  void toggleSuppressedAtoms(bool);
  void toggleCloseContacts(bool);
  void toggleHydrogenBonds(bool);
  void updateHydrogenBondsForCurrent(QString, QString, double, bool);
  void cycleDisorderHighlighting();

  void updateAllCrystalsForChangeInElementData();

  void toggleCC1(bool);
  void toggleCC2(bool);
  void toggleCC3(bool);
  void toggleCloseContact(int, bool);
  void updateCloseContactsForCurrent(int, QString, QString, double);
  void removeIncompleteFragmentsForCurrentCrystal();
  void removeSelectedAtomsForCurrentCrystal();
  void resetCurrentCrystal();

  void showAtomsWithinRadius(float, bool);

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
  void selectedSceneChanged(int);
  void currentSceneChanged();
  void atomSelectionChanged();
  void contactAtomsTurnedOff();
  void surfaceVisibilitiesChanged(Project *);
  void currentCrystalReset();
  void currentCrystalViewChanged();
  void structureChanged();
  void clickedSurface(QModelIndex);
  void clickedSurfacePropertyValue(float);

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
  QMap<ScenePeriodicity, QIcon> m_sceneKindIcons;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &, const Project &);
QDataStream &operator>>(QDataStream &, Project &);
