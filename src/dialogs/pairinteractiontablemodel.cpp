#include "pairinteractiontablemodel.h"
#include "infotable.h"
#include <QMimeData>
#include <QTextDocument>
#include <QTextEdit>

PairInteractionTableModel::PairInteractionTableModel(QObject *parent)
    : QAbstractTableModel(parent) {
  m_columnVisibility.fill(true, FixedColumnCount);
  m_collator.setNumericMode(true);
}

int PairInteractionTableModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return m_interactions.size();
}

int PairInteractionTableModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return m_visibleColumns.size();
}

QVariant PairInteractionTableModel::data(const QModelIndex &index,
                                         int role) const {
  if (!index.isValid())
    return QVariant();
  if (index.row() >= m_interactions.size())
    return QVariant();

  const int actualColumn = visibleToActualColumn(index.column());

  const auto *interaction = m_interactions[index.row()];

  if (actualColumn == ColorColumn) {
    switch (role) {
    case Qt::BackgroundRole:
      return interaction->color();
    case Qt::DisplayRole:
      return "     "; // Five spaces to create the color block
    case Qt::DecorationRole:
      return interaction->color();
    default:
      return QVariant();
    }
  }
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    if (actualColumn < FixedColumnCount) {
      switch (actualColumn) {
      case ColorColumn:
        return QVariant();
      case LabelColumn:
        return interaction->label();
      case CountColumn:
        return interaction->count();
      case DistanceColumn:
        return QString::number(interaction->centroidDistance(), 'f',
                               m_distancePrecision);
      case DescriptionColumn:
        return interaction->dimerDescription();
      }
    } else {
      const QString &key = m_componentColumns[actualColumn - FixedColumnCount];

      if (interaction->components().contains(key)) {
        double value = interaction->getComponent(key);
        return QString::number(value, 'f', m_energyPrecision);
      }

      if (interaction->metadata().contains(key)) {
        return interaction->getMetadata(key);
      }
      return QVariant();
    }
  } else if (role == Qt::BackgroundRole && actualColumn == ColorColumn) {
    return interaction->color();
  } else if (role == Qt::TextAlignmentRole) {
    return Qt::AlignRight;
  }

  return QVariant();
}

QVariant PairInteractionTableModel::headerData(int section,
                                               Qt::Orientation orientation,
                                               int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    return getColumnName(visibleToActualColumn(section));
  }
  return QVariant();
}

void PairInteractionTableModel::setInteractionData(
    const PairInteractions::PairInteractionMap &interactions) {
  beginResetModel();

  m_interactions.clear();

  for (const auto &[_, interaction] : interactions) {
    m_interactions.push_back(interaction);
  }

  m_componentColumns.clear();
  if (!m_interactions.empty()) {
    const auto *firstInteraction = m_interactions[0];
    for (const auto &[component, _] : firstInteraction->components()) {
      m_componentColumns << component;
    }

    for (const auto &[key, _] : firstInteraction->metadata()) {
      m_componentColumns << key;
    }
    std::sort(m_componentColumns.begin(), m_componentColumns.end());
  }

  if (m_columnVisibility.size() <
      FixedColumnCount + m_componentColumns.size()) {
    int oldSize = m_columnVisibility.size();
    m_columnVisibility.resize(FixedColumnCount + m_componentColumns.size());
    for (int i = oldSize; i < m_columnVisibility.size(); ++i) {
      m_columnVisibility[i] = true;
    }
  }

  updateVisibleColumns();
  endResetModel();
}

void PairInteractionTableModel::sort(int column, Qt::SortOrder order) {
  beginResetModel();

  const int actualColumn = visibleToActualColumn(column);
  std::sort(m_interactions.begin(), m_interactions.end(),
            [this, actualColumn, order](const PairInteraction *a,
                                        const PairInteraction *b) {
              if (order == Qt::DescendingOrder)
                std::swap(a, b);

              switch (actualColumn) {
              case ColorColumn:
                return a->color().name() < b->color().name();
              case LabelColumn:
                return m_collator.compare(a->label(), b->label()) < 0;
              case CountColumn:
                return a->count() < b->count();
              case DistanceColumn:
                return a->centroidDistance() < b->centroidDistance();
              case DescriptionColumn:
                return a->dimerDescription() < b->dimerDescription();
              default: {
                if (actualColumn >= FixedColumnCount) {
                  const QString &component =
                      m_componentColumns[actualColumn - FixedColumnCount];
                  return a->getComponent(component) <
                         b->getComponent(component);
                }
                return false;
              }
              }
            });

  endResetModel();
}

void PairInteractionTableModel::setColumnVisible(int column, bool visible) {
  if (column >= 0 && column < m_columnVisibility.size() &&
      m_columnVisibility[column] != visible) {
    beginResetModel();
    m_columnVisibility[column] = visible;
    updateVisibleColumns();
    endResetModel();
  }
}

void PairInteractionTableModel::updateVisibleColumns() {
  m_visibleColumns.clear();
  for (int i = 0; i < m_columnVisibility.size(); ++i) {
    if (m_columnVisibility[i]) {
      m_visibleColumns << i;
    }
  }
}

QString PairInteractionTableModel::getColumnName(int column) const {
  if (column < FixedColumnCount) {
    static const QStringList names = {"Color", "Label", "Count", "Distance",
                                      "Description"};
    return names[column];
  }
  return m_componentColumns[column - FixedColumnCount];
}

void PairInteractionTableModel::setEnergyPrecision(int precision) {
  if (m_energyPrecision != precision) {
    m_energyPrecision = precision;
    emit dataChanged(index(0, FixedColumnCount),
                     index(rowCount() - 1, columnCount() - 1));
  }
}

void PairInteractionTableModel::setDistancePrecision(int precision) {
  if (m_distancePrecision != precision) {
    m_distancePrecision = precision;
    emit dataChanged(index(0, DistanceColumn),
                     index(rowCount() - 1, DistanceColumn));
  }
}

int PairInteractionTableModel::visibleToActualColumn(int visibleColumn) const {
  if (visibleColumn < 0 || visibleColumn >= m_visibleColumns.size()) {
    return -1;
  }
  return m_visibleColumns[visibleColumn];
}

int PairInteractionTableModel::actualToVisibleColumn(int actualColumn) const {
  auto it =
      std::find(m_visibleColumns.begin(), m_visibleColumns.end(), actualColumn);
  if (it == m_visibleColumns.end()) {
    return -1;
  }
  return std::distance(m_visibleColumns.begin(), it);
}

Qt::ItemFlags PairInteractionTableModel::flags(const QModelIndex &index) const {
  return QAbstractTableModel::flags(index) | Qt::ItemIsEnabled |
         Qt::ItemIsSelectable;
}

void PairInteractionTableModel::setTitle(const QString &title) {
  m_title = title;
}

/* This method probably looks convoluted
 * but it basically had to be like this to get excel
 * to actually render the table as html.
 *
 * If you don't have the <h3> tag at the beginning for example
 * it won't recognise it as html and use the colored cells...
 *
 */
void PairInteractionTableModel::copyToClipboard(
    const QModelIndexList &indexes) const {
  if (indexes.empty())
    return;

  QMimeData *mimeData = new QMimeData;

  // Sort indexes by row and column
  QList<QModelIndex> sortedIndexes = indexes;
  std::sort(sortedIndexes.begin(), sortedIndexes.end(),
            [](const QModelIndex &a, const QModelIndex &b) {
              if (a.row() != b.row())
                return a.row() < b.row();
              return a.column() < b.column();
            });

  QTextEdit tempEdit;
  tempEdit.setReadOnly(true);
  QTextCursor cursor(tempEdit.document());

  cursor.beginEditBlock();

  // need this so excel copy/paste works with formatting...
  cursor.insertHtml("<h3>" + m_title + "</h3>");
  int firstRow = sortedIndexes.first().row();
  QSet<int> selectedColumns;
  for (const auto &idx : sortedIndexes) {
    if (idx.row() == firstRow) {
      selectedColumns.insert(idx.column());
    }
  }

  InfoTable infoTable(cursor, sortedIndexes.size() / selectedColumns.size() + 1,
                      selectedColumns.size());

  QStringList headers;
  for (const QModelIndex &index : sortedIndexes) {
    if (index.row() == firstRow) {
      headers << headerData(index.column(), Qt::Horizontal, Qt::DisplayRole)
                     .toString();
    }
  }
  infoTable.insertTableHeader(headers);

  int currentRow = firstRow;
  int col = 0;
  int tableRow = 1;

  for (const QModelIndex &index : sortedIndexes) {
    if (currentRow != index.row()) {
      currentRow = index.row();
      tableRow++;
      col = 0;
    }

    const int actualColumn = visibleToActualColumn(index.column());
    if (actualColumn == ColorColumn) {
      const auto interaction = m_interactions[index.row()];
      infoTable.insertColorBlock(tableRow, col, interaction->color());
    } else {
      infoTable.insertCellValue(tableRow, col,
                                data(index, Qt::DisplayRole).toString(),
                                Qt::AlignRight);
    }
    col++;
  }

  cursor.endEditBlock();

  tempEdit.selectAll();
  tempEdit.copy();
}

void PairInteractionTableModel::setColumnVisibleByName(
    const QString &columnName, bool visible) {
  // Find the actual column index for this name
  for (int i = 0; i < m_columnVisibility.size(); ++i) {
    if (getColumnName(i) == columnName) {
      setColumnVisible(i, visible);
      break;
    }
  }
}

bool PairInteractionTableModel::isColumnVisibleByName(
    const QString &columnName) const {
  for (int i = 0; i < m_columnVisibility.size(); ++i) {
    if (getColumnName(i) == columnName) {
      return m_columnVisibility[i];
    }
  }
  return false;
}

QStringList PairInteractionTableModel::getVisibleColumnNames() const {
  QStringList names;
  for (int actualCol : m_visibleColumns) {
    names << getColumnName(actualCol);
  }
  return names;
}

QStringList PairInteractionTableModel::getAllColumnNames() const {
  QStringList names;
  for (int i = 0; i < FixedColumnCount; ++i) {
    names << getColumnName(i);
  }
  names << m_componentColumns;
  return names;
}
