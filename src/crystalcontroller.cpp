#include "crystalcontroller.h"
#include "confirmationbox.h"
#include "dialoghtml.h"
#include "object_tree_model.h"

CrystalController::CrystalController(QWidget *parent) : QWidget(parent) {
  setupUi(this);
  initConnections();
}

void CrystalController::initConnections() {
  structureListView->installEventFilter(this);
  structureTreeView->installEventFilter(this);

  connect(structureTreeView, &QTreeView::clicked, this,
          &CrystalController::structureViewClicked);
}

void CrystalController::update(Project *project) {
  // TODO probably disconnect previous project here
  structureListView->setModel(project);
  connect(structureListView->selectionModel(),
          &QItemSelectionModel::selectionChanged, project,
          &Project::onSelectionChanged);
}

void CrystalController::handleSceneSelectionChange(int selection) {
  if (selection < 0)
    return;
  auto *project = qobject_cast<Project *>(structureListView->model());
  if (!project)
    return;

  QModelIndex currentIndex = structureListView->currentIndex();
  QModelIndex targetIndex = project->index(selection, 0); // Assuming column 0

  if (currentIndex != targetIndex) {
    structureListView->setCurrentIndex(targetIndex);
  }

  structureListView->setFocus();

  // Assuming currentScene() returns a pointer/reference to a scene or similar
  // object And that updateSurfaceInfo expects such an object
  if (Scene *currentScene = project->currentScene(); currentScene != nullptr) {
    updateSurfaceInfo(currentScene);
  }
}

void CrystalController::handleChildSelectionChange(QModelIndex targetIndex) {

  ChemicalStructure *structure =
      qobject_cast<ChemicalStructure *>(structureTreeView->model());
  if (!structure)
    return;

  QModelIndex currentIndex = structureTreeView->currentIndex();
  qDebug() << "target:" << targetIndex << "Current:" << currentIndex;

  if (currentIndex != targetIndex) {
    structureTreeView->setCurrentIndex(targetIndex);
    structureTreeView->setFocus();
  }
}

void CrystalController::setSurfaceInfo(Project *project) {
  updateSurfaceInfo(project->currentScene());
}

void CrystalController::updateSurfaceInfo(Scene *scene) {
  structureTreeView->setModel(scene->chemicalStructure()->treeModel());
  connect(structureTreeView->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &CrystalController::onStructureViewSelectionChanged,
          Qt::UniqueConnection);
}

void CrystalController::structureViewClicked(const QModelIndex &index) {
  if (index.column() != 0)
    return;
  // Ensure the view corresponds to an ObjectTreeModel
  ObjectTreeModel *model =
      qobject_cast<ObjectTreeModel *>(structureTreeView->model());
  if (!model)
    return;

  QObject *item = static_cast<QObject *>(index.internalPointer());
  qDebug() << "Item on click: " << item;
  if (item) {
    // Toggle the visibility);
    auto prop = item->property("visible");
    if (prop.isNull())
      return;
    bool currentVisibility = prop.toBool();
    item->setProperty("visible", !currentVisibility);
    qDebug() << "Setting object visibility";

    // emit structure->dataChanged(index, index, {Qt::DecorationRole});

    // Update all visibility decorations as they may have changed
    // TODO make this more efficient
    QModelIndex topLeft = model->index(0, 0);
    QModelIndex bottomRight =
        model->index(model->rowCount() - 1, model->columnCount() - 1);

    emit model->dataChanged(topLeft, bottomRight, {Qt::DecorationRole});
    structureTreeView->viewport()->update();
  }
}

void CrystalController::onStructureViewSelectionChanged(
    const QItemSelection &selected, const QItemSelection &deselected) {
  Q_UNUSED(deselected);

  QModelIndex currentIndex =
      structureTreeView->selectionModel()->currentIndex();
  if (currentIndex.isValid()) {
    // You now have the selected index. You can emit a signal or perform actions
    // based on this.
    emit childSelectionChanged(currentIndex);
    qDebug() << "Emit child selection changed";
  }
}

template <typename T>
T *maybeCastChild(const QModelIndex &index, QAbstractItemModel *model) {
  if (!index.isValid())
    return nullptr;

  ObjectTreeModel *tree = qobject_cast<ObjectTreeModel *>(model);
  if (!tree)
    return nullptr;

  QObject *item = static_cast<QObject *>(index.internalPointer());
  return qobject_cast<T *>(item);
}

bool CrystalController::eventFilter(QObject *obj, QEvent *event) {
  if (obj == structureListView || obj == structureTreeView) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
      if (keyEvent->key() == Qt::Key_Delete ||
          keyEvent->key() == Qt::Key_Backspace) {
        if (obj == structureListView) {
          verifyDeleteCurrentCrystal();
        } else {
          verifyDeleteCurrentSurface();
        }
        return true; // Event handled, stop its propagation
      }
    }
    return false;
  }
  return false;
}

void CrystalController::resetViewModel() {
  if (structureTreeView->model()) {
    disconnect(structureTreeView->model(), nullptr, this, nullptr);
    structureTreeView->setModel(nullptr);
  }
  if (structureListView->model()) {
    disconnect(structureListView->model(), nullptr, this, nullptr);
    structureListView->setModel(nullptr);
  }
}

void CrystalController::reset() {
  resetViewModel();
  structureListView->setModel(nullptr);
  structureTreeView->setModel(nullptr);
}

void CrystalController::verifyDeleteCurrentCrystal() {
  auto *project = qobject_cast<Project *>(structureListView->model());
  if (!project)
    return;

  QModelIndex currentIndex = structureListView->currentIndex();
  if (!currentIndex.isValid())
    return;

  QString crystalName = project->data(currentIndex, Qt::DisplayRole).toString();
  if (ConfirmationBox::confirmCrystalDeletion(false, crystalName)) {
    emit currentCrystalDeleted();
    reset();
  }
}

void CrystalController::verifyDeleteCurrentSurface() {
  auto *model = qobject_cast<ObjectTreeModel *>(structureTreeView->model());
  if (!model)
    return;

  QModelIndex currentIndex = structureTreeView->currentIndex();
  if (!currentIndex.isValid())
    return;

  QObject *item = static_cast<QObject *>(currentIndex.internalPointer());
  if (!item)
    return;

  // Check if it's a parent surface (Mesh) or child surface (MeshInstance)
  bool isParent = qobject_cast<Mesh *>(item) != nullptr;
  QString surfaceDescription =
      model->data(currentIndex, Qt::DisplayRole).toString();

  if (ConfirmationBox::confirmSurfaceDeletion(isParent, surfaceDescription)) {
    emit currentSurfaceDeleted();

    // Reset the view if we're deleting a parent surface
    if (isParent) {
      reset();
    }
  }
}

// Replace clearAllCrystals with deleteAllCrystals
void CrystalController::deleteAllCrystals() {
  auto *project = qobject_cast<Project *>(structureListView->model());
  if (!project)
    return;

  if (ConfirmationBox::confirmCrystalDeletion(true, "")) {
    emit allCrystalsDeleted();
    reset();
  }
}
