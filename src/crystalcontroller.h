#pragma once
#include <QWidget>

#include "meshinstance.h"
#include "molecular_wavefunction.h"
#include "pair_energy_results.h"
#include "project.h"

#include "ui_crystalcontroller.h"

class CrystalController : public QWidget, public Ui::CrystalController {
  Q_OBJECT

public:
  CrystalController(QWidget *parent = 0);

  template <typename T> T *getChild(const QModelIndex &index) {
    if (!index.isValid())
      return nullptr;

    ObjectTreeModel *tree =
        qobject_cast<ObjectTreeModel *>(structureTreeView->model());
    if (!tree)
      return nullptr;

    QObject *item = static_cast<QObject *>(index.internalPointer());
    return qobject_cast<T *>(item);
  }

public slots:
  void update(Project *);
  void handleSceneSelectionChange(int);
  void handleChildSelectionChange(QModelIndex);

  void setSurfaceInfo(Project *);
  void deleteCurrentCrystal() { verifyDeleteCurrentCrystal(); }
  void deleteAllCrystals();
  void reset();

signals:
  void structureSelectionChanged(int);
  void childSelectionChanged(QModelIndex);
  void currentCrystalDeleted();
  void currentSurfaceDeleted();
  void allCrystalsDeleted();

protected:
  bool eventFilter(QObject *, QEvent *);

private slots:
  void structureViewClicked(const QModelIndex &);
  void onStructureViewSelectionChanged(const QItemSelection &selected,
                                       const QItemSelection &deselected);

private:
  void resetViewModel();
  void initConnections();
  void updateSurfaceInfo(Scene *);
  void verifyDeleteCurrentCrystal();
  void verifyDeleteCurrentSurface();
};
