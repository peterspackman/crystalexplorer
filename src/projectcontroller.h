#pragma once
#include "meshinstance.h"
#include "molecular_wavefunction.h"
#include "pair_energy_results.h"
#include "project.h"
#include "ui_projectcontroller.h"
#include <QWidget>

class ProjectController : public QWidget, public Ui::ProjectController {
  Q_OBJECT
public:
  explicit ProjectController(Project *project, QWidget *parent = nullptr);
  ~ProjectController() = default;

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
  void handleSceneSelectionChange(int);
  void handleChildSelectionChange(QModelIndex);
  void handleProjectModified();

signals:
  void structureSelectionChanged(int);
  void childSelectionChanged(QModelIndex);
  void projectStateChanged();

protected:
  bool eventFilter(QObject *, QEvent *) override;

private slots:
  void structureViewClicked(const QModelIndex &);
  void onStructureViewSelectionChanged(const QItemSelection &selected,
                                       const QItemSelection &deselected);

private:
  void initConnections();
  void updateSurfaceInfo(Scene *);
  void updateProjectView();
  Project *m_project;
};
