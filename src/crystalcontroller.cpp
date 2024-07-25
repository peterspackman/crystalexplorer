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
    bool currentVisibility = item->property("visible").toBool();
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

void CrystalController::clearAllCrystals() {
  qDebug() << "Clear all crystals";
  /*
if (structureListWidget->count() > 0 &&
    ConfirmationBox::confirmCrystalDeletion(true)) {
  emit deleteAllCrystals();
}
*/
}

void CrystalController::verifyDeleteCurrentCrystal() {
  qDebug() << "Delete current crystal";
  /*
if (structureListWidget->currentItem() &&
    ConfirmationBox::confirmCrystalDeletion(
        false, structureListWidget->currentItem()->text())) {
  emit deleteCurrentCrystal();
}
*/
}

void CrystalController::verifyDeleteCurrentSurface() {
  qDebug() << "Delete current surface";
  /*
QTreeWidgetItem *currentItem = structureTreeView->currentItem();
bool isParent = itemOfParentSurface(currentItem);
QString surfaceDescription = currentItem->text(1);

if (currentItem &&
    ConfirmationBox::confirmSurfaceDeletion(isParent, surfaceDescription)) {
  emit deleteCurrentSurface();
}
*/
}

void CrystalController::updateVisibilityIconsForSurfaces(Project *project) {
  /*
auto surfaceVisibilities =
    project->currentScene()->listOfSurfaceVisibilities();

//QTreeWidgetItem *root = structureTreeView->invisibleRootItem();

for (int i = 0; i < root->childCount(); ++i) {
  if (surfaceVisibilities[i]) {
    root->child(i)->setIcon(0, tickIcon);
  } else {
    root->child(i)->setIcon(0, crossIcon);
  }
}
*/
}
