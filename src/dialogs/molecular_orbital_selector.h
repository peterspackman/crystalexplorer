#pragma once

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QModelIndex>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>
#include <vector>

class MolecularOrbitalSelector : public QWidget {
  Q_OBJECT

public:
  enum class FilterType { All, Occupied, Virtual };

  enum class WavefunctionType { Restricted, Unrestricted };

  struct OrbitalInfo {
    int index;
    QString label;
    double energy = 0.0;
    bool isOccupied{false};
    QString spinLabel{"αβ"};
  };

  explicit MolecularOrbitalSelector(QWidget *parent = nullptr);

  void setWavefunctionCalculated(bool calculated);
  void setWavefunctionType(WavefunctionType type);
  void setOrbitalData(const std::vector<OrbitalInfo> &orbitals);
  void setNumberOfElectrons(int numElectrons);
  void setWavefunctionLabel(const QString &label);

  void setNumberOfBasisFunctions(int numBasis);

  QList<int> getSelectedOrbitalIndices() const;
  QStringList getSelectedOrbitalLabels() const;

signals:
  void selectionChanged(QList<int> indices);
  void selectionChangedLabels(QStringList labels);

private slots:
  void onFilterChanged(int index);
  void onSelectionChanged();
  void setLimitedView(bool, int);

private:
  void updateList();
  void selectDefaultOrbital();
  std::vector<OrbitalInfo> generateUncalculatedLabels(int numElectrons,
                                                      int numBasis);

  QTableView *m_tableView;
  QStandardItemModel *m_model;
  QSortFilterProxyModel *m_proxyModel;
  QComboBox *m_filterComboBox;
  QLabel *m_statusLabel;

  std::vector<OrbitalInfo> m_orbitals;
  std::vector<OrbitalInfo> m_filteredOrbitals;

  bool m_isCalculated = false;
  QString m_wavefunctionLabel{"Wavefunction yet to be calculated"};
  WavefunctionType m_wavefunctionType = WavefunctionType::Restricted;
  FilterType m_currentFilter = FilterType::All;

  int m_numElectrons = 0;
  int m_numBasis = 0;
  bool m_limitedView{true};
  int m_viewRange{2};
};
