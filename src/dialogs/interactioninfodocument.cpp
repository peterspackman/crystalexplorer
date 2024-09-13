#include "interactioninfodocument.h"
#include "infotable.h"
#include "pair_energy_results.h"
#include <QLabel>
#include <QShowEvent>
#include <QStackedLayout>
#include <QTabWidget>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextTable>

InteractionInfoDocument::InteractionInfoDocument(QWidget *parent)
    : QWidget(parent), m_tabWidget(new QTabWidget(this)),
      m_noDataLabel(new QLabel("No interaction information found", this)) {

  QStackedLayout *stackedLayout = new QStackedLayout(this);

  // Create a container widget for the "No data" label
  QWidget *noDataContainer = new QWidget(this);
  QVBoxLayout *noDataLayout = new QVBoxLayout(noDataContainer);
  noDataLayout->addWidget(m_noDataLabel, 0, Qt::AlignCenter);

  stackedLayout->addWidget(m_tabWidget);
  stackedLayout->addWidget(noDataContainer);

  setLayout(stackedLayout);

  // Center the text in the label
  m_noDataLabel->setAlignment(Qt::AlignCenter);

  showNoDataMessage();

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
  // Use a single-shot timer to update colors after the widget is fully shown
  QTimer::singleShot(0, this, &InteractionInfoDocument::updateTableColors);
}

void InteractionInfoDocument::updateContent() {
  m_tabWidget->clear();

  if (!m_scene || !m_scene->chemicalStructure()) {
    showNoDataMessage();
    return;
  }

  auto *interactions = m_scene->chemicalStructure()->pairInteractions();

  if (!interactions || interactions->getCount() == 0) {
    showNoDataMessage();
    return;
  }
  static_cast<QStackedLayout*>(layout())->setCurrentIndex(0); // Index of m_tabWidget

  QList<QString> sortedModels = interactions->interactionModels();
  std::sort(sortedModels.begin(), sortedModels.end());

  for (const QString &model : sortedModels) {
    QWidget *tab = new QWidget(m_tabWidget);
    QVBoxLayout *tabLayout = new QVBoxLayout(tab);

    QTextEdit *textEdit = new QTextEdit(tab);
    textEdit->setReadOnly(true);
    tabLayout->addWidget(textEdit);

    QTextCursor cursor(textEdit->document());
    cursor.beginEditBlock();
    insertInteractionEnergiesForModel(interactions, cursor, model);
    cursor.endEditBlock();

    m_tabWidget->addTab(tab, model);
  }

  if (m_tabWidget->count() > 0) {
    m_tabWidget->setCurrentIndex(0);
  }

  // Update colors immediately after creating content
  updateTableColors();
}

void InteractionInfoDocument::showNoDataMessage() {
    static_cast<QStackedLayout*>(layout())->setCurrentIndex(1); // Index of noDataContainer
}

void InteractionInfoDocument::updateSettings(InteractionInfoSettings settings) {
  m_settings = settings;
  updateContent();
}

void InteractionInfoDocument::insertInteractionEnergiesForModel(
    PairInteractions *results, QTextCursor &cursor, const QString &model) {

  cursor.insertHtml("<h1>Interaction Energies - " + model + "</h1>");

  QSet<QString> uniqueComponents;
  for (const auto &[index, result] : results->filterByModel(model)) {
    for (const auto &component : result->components()) {
      uniqueComponents.insert(component.first);
    }
  }

  QList<QString> sortedComponents = getOrderedComponents(uniqueComponents);
  QStringList tableHeader{"Color", "N", "Distance", "Description"};
  tableHeader.append(sortedComponents);

  InfoTable infoTable(cursor, results->filterByModel(model).size() + 1,
                      tableHeader.size());
  infoTable.insertTableHeader(tableHeader);

  int row = 1;
  for (const auto &[index, result] : results->filterByModel(model)) {
    int column = 0;

    infoTable.insertColorBlock(row, column++, result->color());
    infoTable.insertCellValue(row, column++, QString::number(result->count()),
                              Qt::AlignRight);
    infoTable.insertCellValue(row, column++,
                              QString::number(result->centroidDistance(), 'f',
                                              m_settings.distancePrecision),
                              Qt::AlignRight);
    infoTable.insertCellValue(row, column++, result->dimerDescription(),
                              Qt::AlignRight);

    for (const QString &component : sortedComponents) {
      bool found = false;
      for (const auto &pair : result->components()) {
        if (pair.first == component) {
          infoTable.insertCellValue(
              row, column++,
              QString("%1").arg(pair.second, 6, 'f',
                                m_settings.energyPrecision),
              Qt::AlignRight);
          found = true;
          break;
        }
      }
      if (!found) {
        infoTable.insertCellValue(row, column++, "-", Qt::AlignRight);
      }
    }
    row++;
  }

  cursor.movePosition(QTextCursor::End);
  cursor.insertText("\n\n");
}

QList<QString> InteractionInfoDocument::getOrderedComponents(
    const QSet<QString> &uniqueComponents) {
  QList<QString> knownComponentsOrder;
  knownComponentsOrder << "coulomb" << "repulsion" << "exchange"
                       << "polarization" << "dispersion";

  QList<QString> sortedComponents;

  // Add known components in the desired order
  for (const QString &component : knownComponentsOrder) {
    if (uniqueComponents.contains(component)) {
      sortedComponents << component;
    }
  }

  // Add remaining components (excluding "total") in ascending order
  QList<QString> remainingComponents = uniqueComponents.values();
  remainingComponents.removeOne("total");
  std::sort(remainingComponents.begin(), remainingComponents.end());
  sortedComponents << remainingComponents;

  // Add "total" component at the end if it exists
  if (uniqueComponents.contains("total")) {
    sortedComponents << "total";
  }

  return sortedComponents;
}

void InteractionInfoDocument::updateTableColors() {
  if (!m_scene || !m_scene->chemicalStructure()) {
    return;
  }

  auto *interactions = m_scene->chemicalStructure()->pairInteractions();
  if (!interactions || interactions->getCount() == 0) {
    return;
  }

  for (int i = 0; i < m_tabWidget->count(); ++i) {
    QTextEdit *textEdit = qobject_cast<QTextEdit *>(
        m_tabWidget->widget(i)->layout()->itemAt(0)->widget());
    if (!textEdit)
      continue;

    QTextDocument *doc = textEdit->document();
    QTextCursor cursor(doc);
    cursor.beginEditBlock();

    QTextTable *table = cursor.currentTable();
    if (!table)
      continue;

    QString model = m_tabWidget->tabText(i);
    int row = 1; // Start from 1 to skip the header row

    for (const auto &[index, result] : interactions->filterByModel(model)) {
      QTextTableCell cell = table->cellAt(row, 0);
      QTextCharFormat format = cell.format();
      format.setBackground(result->color());
      cell.setFormat(format);
      QTextTableCell countCell = table->cellAt(row, 1);
      QTextCursor countCursor = countCell.firstCursorPosition();
      countCursor.select(QTextCursor::BlockUnderCursor);
      countCursor.removeSelectedText();
      countCursor.insertText(QString::number(result->count()));
      ++row;
    }
    cursor.endEditBlock();
  }
}
