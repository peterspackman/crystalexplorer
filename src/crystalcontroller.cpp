#include "crystalcontroller.h"
#include "confirmationbox.h"
#include "dialoghtml.h"

CrystalController::CrystalController(QWidget *parent) : QWidget(parent) {
  setupUi(this);
  init();
  initConnections();
}

void CrystalController::init() {
  tickIcon.addFile(":/images/tick.png");
  crossIcon.addFile(":/images/cross.png");
  m_sceneListIcons[ScenePeriodicity::ZeroDimensions] =
      QIcon(":/images/molecule_icon.png");
  m_sceneListIcons[ScenePeriodicity::ThreeDimensions] =
      QIcon(":/images/crystal_icon.png");

  structureListWidget->installEventFilter(this);
  surfaceTreeWidget->installEventFilter(this);
}

void CrystalController::initConnections() {
  connect(structureListWidget, &QListWidget::currentRowChanged, this,
          &CrystalController::structureSelectionChanged);
  connect(surfaceTreeWidget, &QTreeWidget::itemClicked, this,
          &CrystalController::columnInSurfaceListClicked);
  connect(surfaceTreeWidget, &QTreeWidget::currentItemChanged, this,
          &CrystalController::currentSurfaceChanged);
}

void CrystalController::update(Project *project) {
  structureListWidget->clear();
  auto kinds = project->scenePeriodicities();
  auto titles = project->sceneTitles();
  for (int i = 0; i < kinds.size(); i++) {
    QListWidgetItem *item = new QListWidgetItem(titles[i]);
    const auto &kind = kinds[i];
    if (m_sceneListIcons.contains(kind)) {
      item->setIcon(m_sceneListIcons[kind]);
    }
    structureListWidget->addItem(item);
  }
}

void CrystalController::setCurrentCrystal(Project *project) {
  int crystalIndex = project->currentCrystalIndex();
  if (crystalIndex == -1) {
    return;
  }
  Q_ASSERT(crystalIndex < structureListWidget->count());

  if (crystalIndex != structureListWidget->currentRow()) {
    structureListWidget->setCurrentRow(crystalIndex);
  }

  structureListWidget->setFocus();

  updateSurfaceInfo(project->currentScene());
}

void CrystalController::setChildrenEnabled(QTreeWidgetItem *item,
                                           bool enabled) {
  item = surfaceTreeWidget->itemBelow(item);
  while (item) {
    if (!itemOfParentSurface(item)) {
      item->setDisabled(!enabled);
      item->setIcon(0, enabled ? tickIcon : crossIcon);
    } else {
      break;
    }
    item = surfaceTreeWidget->itemBelow(item);
  }
}

void CrystalController::updateVisibilityIconForCurrentSurface(
    Project *project) {
  auto surfaceVisibilities =
      project->currentScene()->listOfSurfaceVisibilities();

  QTreeWidgetItem *currentItem = surfaceTreeWidget->currentItem();

  bool visible = surfaceVisibilities[indexOfSelectedCrystal()];

  currentItem->setIcon(0, visible ? tickIcon : crossIcon);

  // Enable/disable children if currentItem is a parent surface
  if (itemOfParentSurface(currentItem)) {
    setChildrenEnabled(currentItem, visible);
  }
}

int CrystalController::indexOfSelectedCrystal() {
  QTreeWidgetItem *currentItem = surfaceTreeWidget->currentItem();

  for (int n = 0; n < surfaceTreeWidget->topLevelItemCount(); ++n) {
    if (surfaceTreeWidget->topLevelItem(n) == currentItem) {
      return n;
    }
  }
  Q_ASSERT(false); // Should always be able to find the tree index
  return -1;       // Suppress compiler warning about reaching end of non-void
                   // function
}

void CrystalController::setSurfaceInfo(Project *project) {
  updateSurfaceInfo(project->currentScene());
}

/*!
 Whenever the surface info is updated the last item in the surfaceTreeWidget is
 selected.
 */
void CrystalController::updateSurfaceInfo(Scene *scene) {
  surfaceTreeWidget->clear();

  QTreeWidgetItem *root = surfaceTreeWidget->invisibleRootItem();

  QStringList surfaceTitles;
  QVector<bool> surfaceVisibilities;
  if (scene != nullptr) {
    surfaceTitles = scene->listOfSurfaceTitles();
    surfaceVisibilities = scene->listOfSurfaceVisibilities();
  }

  //	QStringList surfaceTitles = crystal->listOfSurfaceTitles();
  //	QList<bool> surfaceVisibilities = crystal->listOfSurfaceVisibilities();

  Q_ASSERT(surfaceTitles.size() == surfaceVisibilities.size());

  for (int i = 0; i < surfaceTitles.size(); ++i) {
    QTreeWidgetItem *item = new QTreeWidgetItem();
    if (surfaceVisibilities[i]) {
      item->setIcon(0, tickIcon);
    } else {
      item->setIcon(0, crossIcon);
    }
    item->setText(1, surfaceTitles[i]);
    root->addChild(item);

    if (i == surfaceTitles.size() - 1) {
      surfaceTreeWidget->setCurrentItem(item);
    }
  }
}

void CrystalController::currentSurfaceChanged(QTreeWidgetItem *currentItem,
                                              QTreeWidgetItem *previousItem) {
  Q_UNUSED(previousItem);

  int surfaceIndex = surfaceTreeWidget->indexOfTopLevelItem(currentItem);
  emit surfaceSelectionChanged(surfaceIndex);
}

bool CrystalController::itemOfParentSurface(QTreeWidgetItem *item) {
  return !(item->text(1).startsWith("+ "));
}

void CrystalController::columnInSurfaceListClicked(QTreeWidgetItem *currentItem,
                                                   int column) {
  if (column == 0) { // Show column
    if (currentItem->isDisabled())
      return;
    int surfaceIndex = surfaceTreeWidget->indexOfTopLevelItem(currentItem);
    emit toggleVisibilityOfSurface(surfaceIndex);
  }
}

void CrystalController::selectSurface(int surfaceIndex) {
  QTreeWidgetItem *root = surfaceTreeWidget->invisibleRootItem();
  surfaceTreeWidget->setCurrentItem(root->child(surfaceIndex));
}

bool CrystalController::eventFilter(QObject *obj, QEvent *event) {
  if (obj == structureListWidget || obj == surfaceTreeWidget) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
      if (keyEvent->key() == Qt::Key_Delete ||
          keyEvent->key() == Qt::Key_Backspace) {
        (obj == structureListWidget) ? verifyDeleteCurrentCrystal()
                                     : verifyDeleteCurrentSurface();
      }
      return true;
    } else {
      return false;
    }
  } else {
    return QWidget::eventFilter(obj,
                                event); // pass the event on to the parent class
  }
}

void CrystalController::clearAllCrystals() {
  if (structureListWidget->count() > 0 &&
      ConfirmationBox::confirmCrystalDeletion(true)) {
    emit deleteAllCrystals();
  }
}

void CrystalController::verifyDeleteCurrentCrystal() {
  if (structureListWidget->currentItem() &&
      ConfirmationBox::confirmCrystalDeletion(
          false, structureListWidget->currentItem()->text())) {
    emit deleteCurrentCrystal();
  }
}

void CrystalController::verifyDeleteCurrentSurface() {
  QTreeWidgetItem *currentItem = surfaceTreeWidget->currentItem();
  bool isParent = itemOfParentSurface(currentItem);
  QString surfaceDescription = currentItem->text(1);

  if (currentItem &&
      ConfirmationBox::confirmSurfaceDeletion(isParent, surfaceDescription)) {
    emit deleteCurrentSurface();
  }
}

void CrystalController::updateVisibilityIconsForSurfaces(Project *project) {
  auto surfaceVisibilities =
      project->currentScene()->listOfSurfaceVisibilities();

  QTreeWidgetItem *root = surfaceTreeWidget->invisibleRootItem();

  for (int i = 0; i < root->childCount(); ++i) {
    if (surfaceVisibilities[i]) {
      root->child(i)->setIcon(0, tickIcon);
    } else {
      root->child(i)->setIcon(0, crossIcon);
    }
  }
}
