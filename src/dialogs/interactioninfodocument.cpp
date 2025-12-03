#include "interactioninfodocument.h"
#include "predictelastictensordialog.h"
#include "publication_reference.h"
#include <QApplication>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QShowEvent>
#include <QStackedLayout>
#include <QTabWidget>
#include <QTableView>
#include <QVBoxLayout>
#include <QTextBrowser>
#include <QSplitter>
#include <QDesktopServices>
#include <QUrl>
#include <QFrame>
#include <QPalette>
#include <QFileDialog>
#include <QMessageBox>
#include <QToolBar>
#include <QToolButton>

InteractionInfoDocument::InteractionInfoDocument(QWidget *parent)
    : QWidget(parent), m_tabWidget(new QTabWidget(this)),
      m_noDataLabel(new QLabel(this)) {

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Add button bar at the top
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->setContentsMargins(4, 4, 4, 4);
  buttonLayout->addStretch();

  m_elasticTensorButton = new QPushButton("Predict Elastic Tensor...", this);
  m_elasticTensorButton->setToolTip("Predict elastic tensor from pair interaction energies");
  buttonLayout->addWidget(m_elasticTensorButton);

  // Hide experimental features by default
  m_elasticTensorButton->setVisible(false);

  mainLayout->addLayout(buttonLayout);

  // Create stacked layout for tab widget and no data message
  m_stackedLayout = new QStackedLayout();
  m_stackedLayout->addWidget(m_tabWidget);

  // Create a more aesthetically pleasing "no data" message
  QWidget *noDataContainer = new QWidget(this);
  QVBoxLayout *noDataLayout = new QVBoxLayout(noDataContainer);

  // Simple, clean no data message
  m_noDataLabel->setText("<html><body>"
                         "<p style='font-size: 14pt;'>No interaction information available</p>"
                         "<p style='font-size: 11pt; opacity: 0.7;'>Select fragments to calculate interaction energies</p>"
                         "</body></html>");
  m_noDataLabel->setAlignment(Qt::AlignCenter);
  m_noDataLabel->setWordWrap(true);

  noDataLayout->addStretch();
  noDataLayout->addWidget(m_noDataLabel, 0, Qt::AlignCenter);
  noDataLayout->addStretch();
  m_stackedLayout->addWidget(noDataContainer);

  mainLayout->addLayout(m_stackedLayout);

  showNoDataMessage();

  setupCopyAction();

  connect(m_elasticTensorButton, &QPushButton::clicked, this,
          &InteractionInfoDocument::estimateElasticTensor);
  connect(m_tabWidget, &QTabWidget::currentChanged, this,
          &InteractionInfoDocument::onTabChanged);
}

void InteractionInfoDocument::forceUpdate() { updateContent(); }

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

  m_stackedLayout->setCurrentWidget(m_tabWidget);

  QList<QString> sortedModels = interactions->interactionModels();
  std::sort(sortedModels.begin(), sortedModels.end());

  for (const QString &model : sortedModels) {
    setupTableForModel(model);
    m_models[model]->setInteractionData(interactions->filterByModel(model));
    m_models[model]->setTitle(model);
    
    // Add the combined widget (table + citations) to the tab
    QWidget *combinedWidget = createTableWithCitations(model);
    m_tabWidget->addTab(combinedWidget, model);
  }

  if (m_tabWidget->count() > 0) {
    m_tabWidget->setCurrentIndex(0);
  }
}

void InteractionInfoDocument::showNoDataMessage() {
  m_stackedLayout->setCurrentIndex(1);
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
  auto *tableView = new QTableView(this);
  auto *tableModel = new PairInteractionTableModel(this);

  // Table configuration - let system theme handle the styling
  tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  tableView->setSelectionBehavior(QAbstractItemView::SelectItems);
  tableView->setAlternatingRowColors(true);
  tableView->setSortingEnabled(true);
  tableView->setFrameShape(QFrame::NoFrame);

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
}

void InteractionInfoDocument::setupCopyAction() {
  if (!m_copyAction) {
    m_copyAction = new QAction(this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    connect(m_copyAction, &QAction::triggered, this, [this]() {
      // Find the current table view from within the container widget
      auto *currentContainer = m_tabWidget->currentWidget();
      if (!currentContainer)
        return;
        
      QTableView *currentTable = nullptr;
      QList<QTableView*> tables = currentContainer->findChildren<QTableView*>();
      if (!tables.isEmpty()) {
        currentTable = tables.first();
      }
      
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

QString InteractionInfoDocument::generateCitationHtml(const QString &model) {
  // Load references if not already loaded
  ReferenceManager& manager = ReferenceManager::instance();
  static bool referencesLoaded = false;
  if (!referencesLoaded) {
    manager.loadFromResource(":/resources/references.json");
    referencesLoaded = true;
  }
  
  // Get citations for this method
  QStringList citationKeys = manager.getCitationsForMethod(model);
  
  if (citationKeys.isEmpty()) {
    return QString();
  }
  
  // Get system palette colors for theme awareness
  QPalette pal = this->palette();
  QString textColor = pal.color(QPalette::Text).name();
  QString bgColor = pal.color(QPalette::Base).name();
  QString altBgColor = pal.color(QPalette::AlternateBase).name();
  QString linkColor = pal.color(QPalette::Link).name();
  
  QString html = QString("<html><head><style>"
                 "body { "
                 "  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; "
                 "  margin: 8px;"
                 "  background-color: %1;"
                 "}"
                 ".citation-box { "
                 "  background: %2;"
                 "  border-radius: 8px;"
                 "  padding: 16px;"
                 "  margin: 6px;"
                 "}"
                 ".citation-header {"
                 "  color: %3;"
                 "  font-size: 13pt;"
                 "  font-weight: 600;"
                 "  margin-bottom: 12px;"
                 "  padding-bottom: 8px;"
                 "  border-bottom: 1px solid %3;"
                 "}"
                 ".citation-item {"
                 "  padding: 12px 0;"
                 "  margin: 8px 0;"
                 "  font-size: 11pt;"
                 "  line-height: 1.5;"
                 "  border-bottom: 1px solid %2;"
                 "}"
                 ".citation-item:last-child {"
                 "  border-bottom: none;"
                 "}"
                 ".citation-authors { "
                 "  color: %3;"
                 "  font-weight: 600;"
                 "  font-size: 11pt;"
                 "}"
                 ".citation-title { "
                 "  color: %3;"
                 "  font-style: italic;"
                 "  font-size: 10pt;"
                 "}"
                 ".citation-journal { "
                 "  color: %3;"
                 "  font-size: 10pt;"
                 "}"
                 ".citation-year { "
                 "  color: %4;"
                 "  font-weight: 600;"
                 "  font-size: 11pt;"
                 "}"
                 ".citation-doi { "
                 "  display: inline-block;"
                 "  margin-top: 6px;"
                 "  font-size: 10pt;"
                 "}"
                 "a { "
                 "  color: %4;"
                 "  text-decoration: none;"
                 "}"
                 "a:hover { "
                 "  text-decoration: underline;"
                 "}"
                 "</style></head><body>").arg(bgColor, altBgColor, textColor, linkColor);
  
  html += "<div class='citation-box'>";
  html += "<div class='citation-header'>References for " + model + "</div>";
  
  for (const QString& key : citationKeys) {
    PublicationReference ref = manager.getReference(key);
    if (ref.key.isEmpty()) continue;
    
    html += "<div class='citation-item'>";
    
    // Format authors
    QString authors;
    if (ref.authors.size() > 3) {
      authors = QString("<span class='citation-authors'>%1 et al.</span>").arg(ref.authors.first());
    } else {
      authors = QString("<span class='citation-authors'>%1</span>").arg(ref.authors.join(", "));
    }
    
    html += authors;
    html += QString(" <span class='citation-year'>(%1)</span><br/>").arg(ref.year);
    
    if (!ref.title.isEmpty()) {
      html += QString("<span class='citation-title'>%1</span><br/>").arg(ref.title);
    }
    
    if (!ref.journal.isEmpty()) {
      html += QString("<span class='citation-journal'>%1").arg(ref.journal);
      if (!ref.volume.isEmpty()) {
        html += QString(" <b>%1</b>").arg(ref.volume);
      }
      if (!ref.pages.isEmpty()) {
        html += QString(", %1").arg(ref.pages);
      }
      html += "</span><br/>";
    }
    
    if (!ref.doi.isEmpty()) {
      html += QString("<span class='citation-doi'>DOI: <a href='https://doi.org/%1'>%1</a></span>")
              .arg(ref.doi);
    }
    
    html += "</div>";
  }
  
  html += "</div>";
  html += "</body></html>";
  
  return html;
}

QWidget* InteractionInfoDocument::createTableWithCitations(const QString &model) {
  QWidget *container = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  
  // Get the table view
  QTableView *tableView = m_views[model];
  
  // Create citation browser
  QTextBrowser *citationBrowser = new QTextBrowser(container);
  citationBrowser->setOpenExternalLinks(true);
  citationBrowser->setFrameShape(QFrame::NoFrame);
  citationBrowser->setMaximumHeight(160);
  citationBrowser->setHtml(generateCitationHtml(model));
  
  // Store reference to citation browser
  m_citationBrowsers[model] = citationBrowser;
  
  // Create splitter for resizable sections
  QSplitter *splitter = new QSplitter(Qt::Vertical, container);
  splitter->addWidget(tableView);
  splitter->addWidget(citationBrowser);
  splitter->setStretchFactor(0, 3); // Table gets more space
  splitter->setStretchFactor(1, 1); // Citations get less space
  
  layout->addWidget(splitter);
  
  return container;
}

void InteractionInfoDocument::showHeaderContextMenu(const QPoint &pos) {
  // Find the current table view from within the container widget
  auto *currentContainer = m_tabWidget->currentWidget();
  if (!currentContainer)
    return;
    
  // The table view is inside a splitter inside the container
  QTableView *currentTable = nullptr;
  QList<QTableView*> tables = currentContainer->findChildren<QTableView*>();
  if (!tables.isEmpty()) {
    currentTable = tables.first();
  }
  
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

void InteractionInfoDocument::estimateElasticTensor() {
  if (!m_scene || !m_scene->chemicalStructure()) {
    return;
  }

  auto *interactions = m_scene->chemicalStructure()->pairInteractions();
  if (!interactions || interactions->getCount() == 0) {
    QMessageBox::warning(this, "No Data",
                        "No pair interactions available. Please calculate pair energies first.");
    return;
  }

  // Pre-select current model if available
  QString currentModel = m_tabWidget->tabText(m_tabWidget->currentIndex());

  PredictElasticTensorDialog dialog(this);
  dialog.setAvailableModels(interactions->interactionModels());
  if (!currentModel.isEmpty()) {
    // Set the current model as default selection
    int idx = dialog.findChild<QComboBox*>()->findText(currentModel);
    if (idx >= 0) {
      dialog.findChild<QComboBox*>()->setCurrentIndex(idx);
    }
  }

  if (dialog.exec() == QDialog::Accepted) {
    QString model = dialog.selectedModel();
    double radius = dialog.cutoffRadius();
    if (!model.isEmpty()) {
      emit elasticTensorRequested(model, radius);
    }
  }
}

void InteractionInfoDocument::enableExperimentalFeatures(bool enable) {
  m_elasticTensorButton->setVisible(enable);
}
