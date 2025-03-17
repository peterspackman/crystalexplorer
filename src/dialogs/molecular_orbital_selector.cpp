#include "molecular_orbital_selector.h"
#include <QCheckBox>
#include <QSpinBox>

MolecularOrbitalSelector::MolecularOrbitalSelector(QWidget *parent)
    : QWidget(parent) {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  m_statusLabel = new QLabel(this);
  m_statusLabel->setText("No orbital information available from wavefunction.");
  mainLayout->addWidget(m_statusLabel);

  QHBoxLayout *filterLayout = new QHBoxLayout();
  QLabel *filterLabel = new QLabel("Filter:", this);
  filterLayout->addWidget(filterLabel);

  m_filterComboBox = new QComboBox(this);
  m_filterComboBox->addItem("All Orbitals", static_cast<int>(FilterType::All));
  m_filterComboBox->addItem("Occupied Orbitals",
                            static_cast<int>(FilterType::Occupied));
  m_filterComboBox->addItem("Virtual Orbitals",
                            static_cast<int>(FilterType::Virtual));
  filterLayout->addWidget(m_filterComboBox);

  QCheckBox *limitViewCheckBox = new QCheckBox("Show only HOMO,LUMO", this);
  limitViewCheckBox->setChecked(true);
  filterLayout->addWidget(limitViewCheckBox);

  QSpinBox *rangeSpinBox = new QSpinBox(this);
  rangeSpinBox->setRange(1, 10);
  rangeSpinBox->setValue(2);
  rangeSpinBox->setPrefix("±");
  filterLayout->addWidget(rangeSpinBox);

  filterLayout->addStretch();

  mainLayout->addLayout(filterLayout);

  m_tableView = new QTableView(this);
  m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_model = new QStandardItemModel(this);

  m_proxyModel = new QSortFilterProxyModel(this);
  m_proxyModel->setSourceModel(m_model);
  m_tableView->setModel(m_proxyModel);

  m_tableView->setSortingEnabled(true);

  // This ensures numeric sorting rather than string sorting
  m_proxyModel->setSortRole(Qt::UserRole);

  m_model->setColumnCount(4);
  m_model->setHeaderData(0, Qt::Horizontal, "Index");
  m_model->setHeaderData(1, Qt::Horizontal, "Label");
  m_model->setHeaderData(2, Qt::Horizontal, "Energy (Eh)");
  m_model->setHeaderData(3, Qt::Horizontal, "Spin");

  m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  m_tableView->verticalHeader()->setVisible(false);

  mainLayout->addWidget(m_tableView);

  connect(m_filterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MolecularOrbitalSelector::onFilterChanged);
  connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
          this, &MolecularOrbitalSelector::onSelectionChanged);

  connect(limitViewCheckBox, &QCheckBox::toggled,
          [this, rangeSpinBox](bool checked) {
            setLimitedView(checked, rangeSpinBox->value());
          });

  connect(rangeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
          [this, limitViewCheckBox](int value) {
            if (limitViewCheckBox->isChecked()) {
              setLimitedView(true, value);
            }
          });

  setWavefunctionCalculated(false);
  setNumberOfElectrons(5);
  setNumberOfBasisFunctions(10);
}

void MolecularOrbitalSelector::setWavefunctionCalculated(bool calculated) {
  m_isCalculated = calculated;
  if (calculated) {
    m_statusLabel->setText(m_wavefunctionType == WavefunctionType::Restricted
                               ? "Restricted spin orbitals"
                               : "Unrestricted spin orbitals");
  } else {
    m_statusLabel->setText(
        "No orbital information available from wavefunction.");
  }
  updateList();
}

void MolecularOrbitalSelector::setWavefunctionType(WavefunctionType type) {
  m_wavefunctionType = type;
  if (m_isCalculated) {
    m_statusLabel->setText(m_wavefunctionType == WavefunctionType::Restricted
                               ? "Restricted Wavefunction"
                               : "Unrestricted Wavefunction");
  }
  updateList();
}

void MolecularOrbitalSelector::setOrbitalData(
    const std::vector<OrbitalInfo> &orbitals) {
  m_orbitals = orbitals;
  updateList();
}

void MolecularOrbitalSelector::setNumberOfElectrons(int numElectrons) {
  m_numElectrons = numElectrons;
  if (!m_isCalculated) {
    updateList();
  }
}

void MolecularOrbitalSelector::setNumberOfBasisFunctions(int numBasis) {
  m_numBasis = numBasis;
  if (!m_isCalculated) {
    updateList();
  }
}

QList<int> MolecularOrbitalSelector::getSelectedOrbitalIndices() const {
  QList<int> selected;
  QModelIndexList indices = m_tableView->selectionModel()->selectedIndexes();
  QMap<int, int> rowToIndexMap;

  for (const QModelIndex &idx : indices) {
    int row = idx.row();
    if (idx.column() == 0) {
      QVariant data = m_proxyModel->data(idx, Qt::UserRole);
      if (data.isValid()) {
        rowToIndexMap[row] = data.toInt();
      }
    }
  }

  QMapIterator<int, int> it(rowToIndexMap);
  while (it.hasNext()) {
    it.next();
    selected.append(it.value());
  }

  return selected;
}

QStringList MolecularOrbitalSelector::getSelectedOrbitalLabels() const {
  QStringList selected;
  QModelIndexList indices = m_tableView->selectionModel()->selectedIndexes();
  QMap<int, QString> rowToLabelMap;

  for (const QModelIndex &idx : indices) {
    int row = idx.row();
    if (idx.column() == 1) {
      QString label = m_proxyModel->data(idx, Qt::DisplayRole).toString();
      rowToLabelMap[row] = label;
    }
  }

  QMapIterator<int, QString> it(rowToLabelMap);
  while (it.hasNext()) {
    it.next();
    selected.append(it.value());
  }

  return selected;
}

void MolecularOrbitalSelector::onFilterChanged(int index) {
  m_currentFilter =
      static_cast<FilterType>(m_filterComboBox->itemData(index).toInt());
  updateList();
}

void MolecularOrbitalSelector::onSelectionChanged() {
  auto indices = getSelectedOrbitalIndices();
  auto labels = getSelectedOrbitalLabels();

  emit selectionChanged(indices);
  emit selectionChangedLabels(labels);
}

void MolecularOrbitalSelector::updateList() {
  m_model->removeRows(0, m_model->rowCount());
  m_filteredOrbitals.clear();

  m_tableView->setSortingEnabled(false);

  std::vector<OrbitalInfo> orbitalsToDisplay;

  if (m_isCalculated) {
    orbitalsToDisplay = m_orbitals;
  } else {
    orbitalsToDisplay = generateUncalculatedLabels(m_numElectrons, m_numBasis);
  }

  int homoIndex = -1;
  int lumoIndex = -1;

  if (m_limitedView) {
    for (size_t i = 0; i < orbitalsToDisplay.size(); ++i) {
      if (orbitalsToDisplay[i].label == "HOMO") {
        homoIndex = i;
      } else if (orbitalsToDisplay[i].label == "LUMO") {
        lumoIndex = i;
      }
    }
  }

  for (const OrbitalInfo &orbital : orbitalsToDisplay) {
    bool includeOrbital = false;
    switch (m_currentFilter) {
    case FilterType::All:
      includeOrbital = true;
      break;
    case FilterType::Occupied:
      includeOrbital = orbital.isOccupied;
      break;
    case FilterType::Virtual:
      includeOrbital = !orbital.isOccupied;
      break;
    }

    if (m_limitedView && includeOrbital) {
      int orbitalIndex =
          &orbital - &orbitalsToDisplay[0]; // Get index in vector

      if (homoIndex >= 0 && m_currentFilter != FilterType::Virtual) {
        includeOrbital = std::abs(orbitalIndex - homoIndex) <= m_viewRange;
      } else if (lumoIndex >= 0 && m_currentFilter != FilterType::Occupied) {
        includeOrbital = std::abs(orbitalIndex - lumoIndex) <= m_viewRange;
      }
    }

    if (includeOrbital) {
      m_filteredOrbitals.push_back(orbital);

      QList<QStandardItem *> rowItems;

      QStandardItem *indexItem =
          new QStandardItem(QString::number(orbital.index));
      indexItem->setData(orbital.index, Qt::UserRole);
      rowItems.append(indexItem);

      QStandardItem *labelItem = new QStandardItem(orbital.label);
      rowItems.append(labelItem);

      QStandardItem *energyItem;
      if (m_isCalculated) {
        energyItem = new QStandardItem(QString::number(orbital.energy, 'f', 6));
        energyItem->setData(orbital.energy, Qt::UserRole);
      } else {
        energyItem = new QStandardItem("-");
      }
      rowItems.append(energyItem);

      QStandardItem *spinItem = new QStandardItem(orbital.spinLabel);
      rowItems.append(spinItem);

      m_model->appendRow(rowItems);
    }
  }

  m_tableView->setSortingEnabled(true);
  m_tableView->sortByColumn(0, Qt::DescendingOrder);
  selectDefaultOrbital();
}

std::vector<MolecularOrbitalSelector::OrbitalInfo>
MolecularOrbitalSelector::generateUncalculatedLabels(int numElectrons,
                                                     int numBasis) {
  std::vector<OrbitalInfo> orbitals;

  if (m_wavefunctionType == WavefunctionType::Restricted) {
    int numOccupied = numElectrons / 2;
    int homoIndex = numOccupied - 1;

    for (int i = 0; i < numBasis; ++i) {
      OrbitalInfo orbital;
      orbital.index = i;
      orbital.isOccupied = (i <= homoIndex);

      if (i == homoIndex) {
        orbital.label = "HOMO";
      } else if (i == homoIndex + 1) {
        orbital.label = "LUMO";
      } else if (i < homoIndex) {
        orbital.label = QString("HOMO-%1").arg(homoIndex - i);
      } else {
        orbital.label = QString("LUMO+%1").arg(i - (homoIndex + 1));
      }

      orbitals.push_back(orbital);
    }
  } else {
    int numAlpha = (numElectrons + 1) / 2;
    int numBeta = numElectrons / 2;

    int homoAlphaIndex = numAlpha - 1;
    int homoBetaIndex = numBeta - 1;

    for (int i = 0; i < numBasis; ++i) {
      OrbitalInfo orbital;
      orbital.index = i;
      orbital.isOccupied = (i <= homoAlphaIndex);
      orbital.spinLabel = "α";

      if (i == homoAlphaIndex) {
        orbital.label = "HOMO";
      } else if (i == homoAlphaIndex + 1) {
        orbital.label = "LUMO";
      } else if (i < homoAlphaIndex) {
        orbital.label = QString("HOMO-%1").arg(homoAlphaIndex - i);
      } else {
        orbital.label = QString("LUMO+%1").arg(i - (homoAlphaIndex + 1));
      }

      orbitals.push_back(orbital);
    }

    for (int i = 0; i < numBasis; ++i) {
      OrbitalInfo orbital;
      orbital.index = i + numBasis;
      orbital.isOccupied = (i <= homoBetaIndex);
      orbital.spinLabel = "β";

      if (i == homoBetaIndex) {
        orbital.label = "HOMO";
      } else if (i == homoBetaIndex + 1) {
        orbital.label = "LUMO";
      } else if (i < homoBetaIndex) {
        orbital.label = QString("HOMO-%1").arg(homoBetaIndex - i);
      } else {
        orbital.label = QString("LUMO+%1").arg(i - (homoBetaIndex + 1));
      }

      orbitals.push_back(orbital);
    }
  }

  return orbitals;
}

void MolecularOrbitalSelector::selectDefaultOrbital() {
  int homoRow = -1;
  int lumoRow = -1;

  for (int row = 0; row < m_proxyModel->rowCount(); ++row) {
    QString label = m_proxyModel->index(row, 1).data().toString();
    if (label == "HOMO") {
      homoRow = row;
    } else if (label == "LUMO") {
      lumoRow = row;
    }
  }

  if (homoRow >= 0) {
    m_tableView->selectRow(homoRow);
    m_tableView->scrollTo(m_proxyModel->index(homoRow, 0));
  } else if (lumoRow >= 0) {
    m_tableView->selectRow(lumoRow);
    m_tableView->scrollTo(m_proxyModel->index(lumoRow, 0));
  }
}

void MolecularOrbitalSelector::setLimitedView(bool limited, int range) {
  m_limitedView = limited;
  m_viewRange = range;
  updateList();
}
