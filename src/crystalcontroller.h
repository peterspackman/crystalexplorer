#pragma once
#include <QWidget>

#include "project.h"
#include "meshinstance.h"
#include "pair_energy_results.h"
#include "molecular_wavefunction.h"

#include "ui_crystalcontroller.h"

/*!
 \brief A simpler replacement for the masterlist
 */
class CrystalController : public QWidget, public Ui::CrystalController {
  Q_OBJECT

public:
  CrystalController(QWidget *parent = 0);

  template<typename T>
  T* getChild(const QModelIndex &index) {
      if (!index.isValid()) return nullptr;

      ObjectTreeModel * tree = qobject_cast<ObjectTreeModel*>(structureTreeView->model());
      if (!tree) return nullptr;

      QObject* item = static_cast<QObject*>(index.internalPointer());
      return qobject_cast<T*>(item);
  }

public slots:
  void update(Project *);
  void handleSceneSelectionChange(int);
  void handleChildSelectionChange(QModelIndex);

  void setSurfaceInfo(Project *);
  void clearCurrentCrystal() { verifyDeleteCurrentCrystal(); }
  void clearAllCrystals();
  void updateVisibilityIconsForSurfaces(Project *);

signals:
  void structureSelectionChanged(int);
  void childSelectionChanged(QModelIndex);
  void deleteCurrentCrystal();
  void deleteCurrentSurface();
  void deleteAllCrystals();

protected:
  bool eventFilter(QObject *, QEvent *);

private slots:
  void structureViewClicked(const QModelIndex &);
  void onStructureViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private:
  void initConnections();
  void updateSurfaceInfo(Scene *);
  void verifyDeleteCurrentCrystal();
  void verifyDeleteCurrentSurface();
};
