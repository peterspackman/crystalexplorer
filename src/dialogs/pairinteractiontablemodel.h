#pragma once
#include "pair_energy_results.h"
#include <QAbstractTableModel>
#include <QStringList>

class PairInteractionTableModel : public QAbstractTableModel {
  Q_OBJECT
public:
  explicit PairInteractionTableModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  void copyToClipboard(const QModelIndexList &indexes) const;
  void sort(int column, Qt::SortOrder order) override;

  void
  setInteractionData(const PairInteractions::PairInteractionMap &interactions);
  void setTitle(const QString &);
  void setEnergyPrecision(int precision);
  void setDistancePrecision(int precision);

  void setColumnVisible(int column, bool visible);
  bool isColumnVisible(int column) const;
  QStringList availableColumns() const;
  void setColumnVisibleByName(const QString &columnName, bool visible);
  bool isColumnVisibleByName(const QString &columnName) const;
  QStringList getVisibleColumnNames() const;
  QStringList getAllColumnNames() const;

  // Fixed columns first, followed by dynamic component columns
  enum FixedColumns {
    ColorColumn = 0,
    LabelColumn,
    CountColumn,
    DistanceColumn,
    DescriptionColumn,
    FixedColumnCount
  };

private:
  QString getColumnName(int column) const;
  int actualToVisibleColumn(int actualColumn) const;
  int visibleToActualColumn(int visibleColumn) const;
  void updateVisibleColumns();

  QString m_title{"Interaction Energies"};
  QStringList m_componentColumns;
  QVector<int> m_visibleColumns;
  QVector<bool> m_columnVisibility;
  std::vector<PairInteraction *> m_interactions;
  int m_energyPrecision{1};
  int m_distancePrecision{2};
};
