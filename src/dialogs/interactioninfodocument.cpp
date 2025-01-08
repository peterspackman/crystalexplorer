#include "interactioninfodocument.h"
#include <QApplication>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QShowEvent>
#include <QStackedLayout>
#include <QTabWidget>
#include <QTableView>
#include <QVBoxLayout>

InteractionInfoDocument::InteractionInfoDocument(QWidget *parent)
    : QWidget(parent), m_tabWidget(new QTabWidget(this)),
      m_noDataLabel(new QLabel("No interaction information found", this)) {

  QStackedLayout *stackedLayout = new QStackedLayout(this);
  stackedLayout->addWidget(m_tabWidget);

  // Create container for "no data" message
  QWidget *noDataContainer = new QWidget(this);
  QVBoxLayout *noDataLayout = new QVBoxLayout(noDataContainer);
  noDataLayout->addWidget(m_noDataLabel, 0, Qt::AlignCenter);
  stackedLayout->addWidget(noDataContainer);

  setLayout(stackedLayout);

  // Center the text in the label
  m_noDataLabel->setAlignment(Qt::AlignCenter);

  showNoDataMessage();

  setupCopyAction();
  connect(m_tabWidget, &QTabWidget::currentChanged, this,
          &InteractionInfoDocument::onTabChanged);
}

void InteractionInfoDocument::updateScene(Scene *scene) {
  m_scene = scene;
  updateContent();
}

void InteractionInfoDocument::onTabChanged(int index) {
  if (index >= 0 && index < m_tabWidget->count()) {
    emit currentModelChanged(m_tabWidget->tabText(index));
  }
}

void InteractionInfoDocument::showEvent(QShowEvent *event) {
  QWidget::showEvent(event);
}

void InteractionInfoDocument::updateContent() {
  m_tabWidget->clear();

  qDeleteAll(m_models);
  m_models.clear();
  m_views.clear();

  if (!m_scene || !m_scene->chemicalStructure()) {
    showNoDataMessage();
    return;
  }

  auto *interactions = m_scene->chemicalStructure()->pairInteractions();

  if (!interactions || interactions->getCount() == 0) {
    showNoDataMessage();
    return;
  }

  static_cast<QStackedLayout *>(layout())->setCurrentWidget(m_tabWidget);

  QList<QString> sortedModels = interactions->interactionModels();
  std::sort(sortedModels.begin(), sortedModels.end());

  for (const QString &model : sortedModels) {
    setupTableForModel(model);
    m_models[model]->setInteractionData(interactions->filterByModel(model));
    m_models[model]->setTitle(model);
  }

  if (m_tabWidget->count() > 0) {
    m_tabWidget->setCurrentIndex(0);
  }
}

void InteractionInfoDocument::showNoDataMessage() {
  static_cast<QStackedLayout *>(layout())->setCurrentIndex(1);
}

void InteractionInfoDocument::updateSettings(InteractionInfoSettings settings) {
  m_settings = settings;
  for (auto *model : m_models) {
    model->setEnergyPrecision(settings.energyPrecision);
    model->setDistancePrecision(settings.distancePrecision);
  }
  updateContent();
}

void InteractionInfoDocument::setupTableForModel(const QString &model) {
  auto *tableView = new QTableView(m_tabWidget);
  auto *tableModel = new PairInteractionTableModel(this);

  tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  tableView->setSelectionBehavior(QAbstractItemView::SelectItems);
  tableView->setAlternatingRowColors(true);
  tableView->setSortingEnabled(true);

  tableView->setCornerButtonEnabled(true);

  auto *hHeader = tableView->horizontalHeader();
  hHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
  hHeader->setStretchLastSection(true);

  tableView->setTextElideMode(Qt::ElideNone);
  tableView->verticalHeader()->hide();

  m_models[model] = tableModel;
  m_views[model] = tableView;

  tableModel->setEnergyPrecision(m_settings.energyPrecision);
  tableModel->setDistancePrecision(m_settings.distancePrecision);

  tableView->setModel(tableModel);
  tableView->addAction(m_copyAction); // Use the shared copy action

  tableView->resizeColumnsToContents();
  tableView->setColumnWidth(0, 30);

  auto *header = tableView->horizontalHeader();

  // right click menu on header
  header->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(header, &QHeaderView::customContextMenuRequested, this,
          &InteractionInfoDocument::showHeaderContextMenu);

  m_tabWidget->addTab(tableView, model);
}

void InteractionInfoDocument::setupCopyAction() {
  if (!m_copyAction) {
    m_copyAction = new QAction(this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    connect(m_copyAction, &QAction::triggered, this, [this]() {
      auto *currentTable =
          qobject_cast<QTableView *>(m_tabWidget->currentWidget());
      if (!currentTable)
        return;

      QModelIndexList indexes =
          currentTable->selectionModel()->selectedIndexes();
      if (indexes.isEmpty())
        return;

      auto *model =
          qobject_cast<PairInteractionTableModel *>(currentTable->model());
      if (!model)
        return;
      model->copyToClipboard(indexes);
    });
  }
}

void InteractionInfoDocument::showHeaderContextMenu(const QPoint &pos) {
  auto *currentTable = qobject_cast<QTableView *>(m_tabWidget->currentWidget());
  if (!currentTable)
    return;

  auto *model =
      qobject_cast<PairInteractionTableModel *>(currentTable->model());
  if (!model)
    return;

  auto *header = currentTable->horizontalHeader();

  if (!m_headerContextMenu) {
    m_headerContextMenu = new QMenu(this);
  }
  m_headerContextMenu->clear();

  QStringList allColumns = model->getAllColumnNames();
  for (const QString &columnName : allColumns) {
    QAction *action = m_headerContextMenu->addAction(columnName);
    action->setCheckable(true);
    action->setChecked(model->isColumnVisibleByName(columnName));

    connect(action, &QAction::triggered, this,
            [model, columnName](bool checked) {
              model->setColumnVisibleByName(columnName, checked);
            });
  }

  m_headerContextMenu->exec(header->mapToGlobal(pos));
}
