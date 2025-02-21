#include "projectcontroller.h"
#include "object_tree_model.h"
#include <QKeyEvent>
#include <QMenu>

ProjectController::ProjectController(Project *project, QWidget *parent)
    : QWidget(parent), m_project(project) {
  setupUi(this);
  initConnections();
  updateProjectView();
}

void ProjectController::initConnections() {
  structureListView->installEventFilter(this);
  structureTreeView->installEventFilter(this);

  setupContextMenus();
  connect(structureTreeView, &QTreeView::clicked, this,
          &ProjectController::structureViewClicked);

  if (m_project) {
    connect(m_project, &Project::sceneSelectionChanged, this,
            &ProjectController::handleSceneSelectionChange);
    connect(m_project, &Project::projectModified, this,
            &ProjectController::handleProjectModified);
  }
}

void ProjectController::updateProjectView() {
  if (m_project) {
    if (structureListView->selectionModel()) {
      disconnect(structureListView->selectionModel(),
                 &QItemSelectionModel::selectionChanged, m_project,
                 &Project::onSelectionChanged);
    }

    structureListView->setModel(m_project);
    updateSurfaceInfo(m_project->currentScene());

    if (structureListView->selectionModel()) {
      connect(structureListView->selectionModel(),
              &QItemSelectionModel::selectionChanged, m_project,
              &Project::onSelectionChanged);
    }
  } else {
    structureListView->setModel(nullptr);
    structureTreeView->setModel(nullptr);
  }

  structureListView->viewport()->update();
  structureTreeView->viewport()->update();
}

void ProjectController::handleProjectModified() {
  updateProjectView();
  emit projectStateChanged();
}

void ProjectController::handleSceneSelectionChange(int selection) {
  if (selection < 0 || !m_project) {
    return;
  }

  QModelIndex currentIndex = structureListView->currentIndex();
  QModelIndex targetIndex = m_project->index(selection, 0);

  if (currentIndex != targetIndex) {
    structureListView->setCurrentIndex(targetIndex);
  }

  structureListView->setFocus();

  if (Scene *currentScene = m_project->currentScene()) {
    updateSurfaceInfo(currentScene);
  }
}

void ProjectController::handleChildSelectionChange(QModelIndex targetIndex) {
  ChemicalStructure *structure =
      qobject_cast<ChemicalStructure *>(structureTreeView->model());
  if (!structure)
    return;

  QModelIndex currentIndex = structureTreeView->currentIndex();

  if (currentIndex != targetIndex) {
    structureTreeView->setCurrentIndex(targetIndex);
    structureTreeView->setFocus();
  }
}

void ProjectController::structureViewClicked(const QModelIndex &index) {
  if (index.column() != 0)
    return;

  ObjectTreeModel *model =
      qobject_cast<ObjectTreeModel *>(structureTreeView->model());
  if (!model)
    return;

  QObject *item = static_cast<QObject *>(index.internalPointer());
  if (item) {
    auto prop = item->property("visible");
    if (prop.isNull())
      return;
    bool currentVisibility = prop.toBool();
    item->setProperty("visible", !currentVisibility);

    QModelIndex topLeft = model->index(0, 0);
    QModelIndex bottomRight =
        model->index(model->rowCount() - 1, model->columnCount() - 1);

    emit model->dataChanged(topLeft, bottomRight, {Qt::DecorationRole});
    structureTreeView->viewport()->update();
  }
}

void ProjectController::updateSurfaceInfo(Scene *scene) {

  if (structureTreeView->selectionModel()) {
    disconnect(structureTreeView->selectionModel(),
               &QItemSelectionModel::selectionChanged, this,
               &ProjectController::onStructureViewSelectionChanged);
  }

  if (!scene) {
    qDebug() << "Scene is null!";
    structureTreeView->setModel(nullptr);
    return;
  }
  structureTreeView->setModel(scene->chemicalStructure()->treeModel());

  if (structureTreeView->selectionModel()) {
    connect(structureTreeView->selectionModel(),
            &QItemSelectionModel::selectionChanged, this,
            &ProjectController::onStructureViewSelectionChanged);
  }
}

void ProjectController::onStructureViewSelectionChanged(
    const QItemSelection &selected, const QItemSelection &deselected) {
  Q_UNUSED(deselected);

  QModelIndex currentIndex =
      structureTreeView->selectionModel()->currentIndex();
  if (currentIndex.isValid()) {
    qDebug() << "Child selection: " << currentIndex;
    emit childSelectionChanged(currentIndex);
  }
}

bool ProjectController::eventFilter(QObject *obj, QEvent *event) {
  if (obj == structureListView || obj == structureTreeView) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
      if (keyEvent->key() == Qt::Key_Delete ||
          keyEvent->key() == Qt::Key_Backspace) {
        if (obj == structureListView && m_project) {
          // Get the currently selected index
          QModelIndex currentIndex = structureListView->currentIndex();
          if (currentIndex.isValid()) {
            // Use the model's removeScene method
            m_project->removeScene(currentIndex.row());
            return true;
          }
        } else if (obj == structureTreeView && m_project &&
                   m_project->currentScene()) {
          // Let the project handle surface deletion through the tree model
          m_project->currentScene()->setNeedsUpdate();
          emit m_project->sceneContentChanged();
          return true;
        }
      }
    }
  }
  return QWidget::eventFilter(obj, event);
}

void ProjectController::setupContextMenus() {
  structureListView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(structureListView, &QWidget::customContextMenuRequested, this,
          &ProjectController::showStructureListContextMenu);
}

void ProjectController::showStructureListContextMenu(const QPoint &pos) {
  QModelIndex index = structureListView->indexAt(pos);
  if (!index.isValid() || !m_project)
    return;

  QMenu contextMenu(tr("Structure Menu"), this);

  QAction *deleteAction = contextMenu.addAction(tr("Delete Structure"));
  connect(deleteAction, &QAction::triggered, this,
          [this, index]() { m_project->removeScene(index.row()); });

  // Add other context menu actions as needed

  contextMenu.exec(structureListView->mapToGlobal(pos));
}
