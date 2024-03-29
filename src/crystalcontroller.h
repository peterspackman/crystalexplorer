#pragma once
#include <QWidget>

#include "project.h"

#include "ui_crystalcontroller.h"

/*!
 \brief A simpler replacement for the masterlist
 */
class CrystalController : public QWidget, public Ui::CrystalController {
  Q_OBJECT

public:
  CrystalController(QWidget *parent = 0);

public slots:
  void update(Project *);
  void setCurrentCrystal(Project *);
  void setSurfaceInfo(Project *);
  void updateVisibilityIconForCurrentSurface(Project *);
  void selectSurface(int);
  void clearCurrentCrystal() { verifyDeleteCurrentCrystal(); }
  void clearAllCrystals();
  void updateVisibilityIconsForSurfaces(Project *);

signals:
  void structureSelectionChanged(int);
  void surfaceSelectionChanged(int);
  void toggleVisibilityOfSurface(int);
  void deleteCurrentCrystal();
  void deleteCurrentSurface();
  void deleteAllCrystals();

protected:
  bool eventFilter(QObject *, QEvent *);

private slots:
  void currentSurfaceChanged(QTreeWidgetItem *, QTreeWidgetItem *);
  void columnInSurfaceListClicked(QTreeWidgetItem *, int);

private:
  void init();
  void initConnections();
  void updateSurfaceInfo(Scene *);
  void verifyDeleteCurrentCrystal();
  void verifyDeleteCurrentSurface();
  int indexOfSelectedCrystal();
  bool itemOfParentSurface(QTreeWidgetItem *);
  void setChildrenEnabled(QTreeWidgetItem *, bool);

  QIcon tickIcon;
  QIcon crossIcon;
  QMap<ScenePeriodicity, QIcon> m_sceneListIcons;
};
