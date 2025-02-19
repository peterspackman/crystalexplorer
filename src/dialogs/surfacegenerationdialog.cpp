#include "surfacegenerationdialog.h"
#include "globalconfiguration.h"
#include "globals.h"
#include "settings.h"

#include <QPushButton>

#include <QDebug>
#include <QSignalBlocker>

SurfaceGenerationDialog::SurfaceGenerationDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::SurfaceGenerationDialog()) {
  ui->setupUi(this);
  init();
  initConnections();
}

void SurfaceGenerationDialog::init() {
  auto *g = GlobalConfiguration::getInstance();
  if (g) {
    m_surfaceDescriptions = g->getSurfaceDescriptions();
    m_surfacePropertyDescriptions = g->getPropertyDescriptions();
  }

  for (const auto &k : m_surfaceDescriptions.descriptions) {
    qDebug() << k.displayName << k.occName;
  }
  ui->surfaceComboBox->setDescriptions(m_surfaceDescriptions);
  ui->propertyComboBox->setDescriptions(m_surfaceDescriptions,
                                        m_surfacePropertyDescriptions);

  for (int i = 0; i < ui->surfaceComboBox->count(); i++) {
    qDebug() << ui->surfaceComboBox->itemText(i);
  }

  updateIsovalue();
  setupOrbitalUI();

  surfaceChanged(m_currentSurfaceType);

  m_charge = 0; // default value but should be set with setChargeForCalculation
  m_multiplicity = 1;

  updateSettings();
}

void SurfaceGenerationDialog::initConnections() {
  connect(ui->showDescriptionsCheckBox, &QCheckBox::checkStateChanged,
          [&](int state) { this->updateDescriptions(); });

  connect(ui->surfaceComboBox, &SurfaceTypeDropdown::selectionChanged,
          ui->propertyComboBox,
          &SurfacePropertyTypeDropdown::onSurfaceSelectionChanged);
  ui->surfaceComboBox->setCurrent(m_currentSurfaceType);

  connect(ui->surfaceComboBox, &SurfaceTypeDropdown::selectionChanged, this,
          &SurfaceGenerationDialog::surfaceChanged);

  connect(ui->propertyComboBox, &SurfacePropertyTypeDropdown::selectionChanged,
          this, &SurfaceGenerationDialog::propertyChanged);

  connect(this, &SurfaceGenerationDialog::accepted, this,
          &SurfaceGenerationDialog::validate);
  connect(ui->useUserDefinedCluster, &QCheckBox::toggled,
          ui->voidClusterPaddingSpinBox, &QDoubleSpinBox::setEnabled);
}

void SurfaceGenerationDialog::setAtomIndices(
    const std::vector<GenericAtomIndex> &atoms) {
  m_atomIndices = atoms;
}

void SurfaceGenerationDialog::setChargeForCalculation(int charge) {
  m_charge = charge;
}

void SurfaceGenerationDialog::setMultiplicityForCalculation(int multiplicity) {
  m_multiplicity = multiplicity;
}

void SurfaceGenerationDialog::setSuitableWavefunctions(
    const std::vector<WavefunctionAndTransform> &wfns) {
  m_availableWavefunctions = wfns;
  updateWavefunctionComboBox(true);
  updateOrbitalLabels();
}

QString SurfaceGenerationDialog::currentKindName() const {
  const auto &currentSurface = ui->surfaceComboBox->currentSurfaceDescription();
  return currentSurface.occName;
}
isosurface::Kind SurfaceGenerationDialog::currentKind() const {
  return isosurface::stringToKind(ui->surfaceComboBox->current());
}

QString SurfaceGenerationDialog::currentPropertyName() const {
  if (ui->propertyComboBox->currentText() == "None")
    return "None";

  const auto &currentSurfaceProperty =
      ui->propertyComboBox->currentSurfacePropertyDescription();
  return currentSurfaceProperty.occName;
}

void SurfaceGenerationDialog::validate() {

  isosurface::Parameters parameters;
  parameters.isovalue = ui->isovalueLineEdit->text().toFloat();
  parameters.kind = currentKind();
  parameters.computeNegativeIsovalue = shouldAlsoCalculateNegativeIsovalue();
  QString prop = currentPropertyName();
  if (prop != "None") {
    parameters.additionalProperties.append(prop);
  }
  parameters.separation =
      isosurface::resolutionValue(ui->resolutionComboBox->currentLevel());
  qDebug() << isosurface::kindToString(parameters.kind);

  if (needWavefunction()) {
    qDebug() << "Needs wavefunction";
    wfn::Parameters wfn_params;
    wfn_params.charge = m_charge;
    wfn_params.multiplicity = m_multiplicity;

    const int wfnIndex = ui->wavefunctionCombobox->currentIndex() - 1;
    if (wfnIndex > -1) {
      qDebug() << "wfnIndex" << wfnIndex;
      const auto &[wfn, transform] = m_availableWavefunctions[wfnIndex];
      qDebug() << "Have existing wavefunction: " << wfn->description();
      wfn_params = wfn->parameters();
      wfn_params.accepted = true;
      parameters.wfn = wfn;
      parameters.wfn_transform = transform;
    }
    emit surfaceParametersChosenNeedWavefunction(parameters, wfn_params);
  } else {
    emit surfaceParametersChosenNew(parameters);
  }
}

void SurfaceGenerationDialog::updateSettings() {
  ui->resolutionComboBox->setCurrentIndex(
      static_cast<int>(isosurface::Resolution::High));
  // TODO defaultOrbitalType
  ui->surfaceOptionsBox->setHidden(defaultHideSurfaceOptionsBox);
  ui->showDescriptionsCheckBox->setCheckState(defaultShowDescriptions);

  updateDescriptions();
  adjustSize();
}

void SurfaceGenerationDialog::updateIsovalue() {
  double defaultIsovalue =
      m_surfaceDescriptions.get(m_currentSurfaceType).defaultIsovalue;
  ui->isovalueLineEdit->setText(QString::number(defaultIsovalue));
}

void SurfaceGenerationDialog::surfaceChanged(QString selection) {
  m_currentSurfaceType = selection;
  updateSurfaceOptions();

  updateIsovalue();
  updateWavefunctionComboBox();
  updateDescriptions();
  adjustSize();
}

bool SurfaceGenerationDialog::havePropertyChoices() {
  Q_ASSERT(ui->propertyComboBox->count() > 0);

  return ui->propertyComboBox->count() > 1;
}

void SurfaceGenerationDialog::propertyChanged(QString property) {
  updatePropertyOptions();
  updateWavefunctionComboBox();
  updateDescriptions();
  adjustSize();
}

void SurfaceGenerationDialog::updatePropertyOptions() { updateOrbitalLabels(); }

void SurfaceGenerationDialog::updateSurfaceOptions() {
  bool hideSurfaceOptions = true;
  ui->isovalueBox->setHidden(true);
  ui->clusterBox->setHidden(true);

  if (needIsovalueBox()) {
    hideSurfaceOptions = false;
    ui->isovalueBox->setHidden(false);
    const auto &currentSurface =
        ui->surfaceComboBox->currentSurfaceDescription();
    ui->unitLabel->setText(currentSurface.units);
  }
  if (needClusterOptions()) {
    hideSurfaceOptions = false;
    ui->clusterBox->setHidden(false);
    ui->useUnitCellPlusFiveAng->setChecked(true);
  }
  ui->surfaceOptionsBox->setHidden(hideSurfaceOptions);

  updateOrbitalLabels();
}

void SurfaceGenerationDialog::updateOrbitalLabels() {
  ui->orbitalSelectionListWidget->clear();
  m_orbitalLabels.clear();
  bool needsOrbital = needOrbitalBox();

  if (!needsOrbital) {
    ui->orbitalBox->setVisible(false);
    return;
  }
  ui->orbitalBox->setVisible(true);

  const bool haveExistingWfn = !m_availableWavefunctions.empty() &&
                               ui->wavefunctionCombobox->currentIndex() > 0;

  if (haveExistingWfn) {
    // Get current wavefunction
    const auto &[wfn, _] =
        m_availableWavefunctions[ui->wavefunctionCombobox->currentIndex() - 1];
    const int nOcc = wfn->numberOfOccupiedOrbitals();
    const int nOrb = wfn->numberOfOrbitals();
    const auto &energies = wfn->orbitalEnergies();

    // Generate labels based on actual wavefunction
    for (int i = 0; i < nOrb; ++i) {
      isosurface::OrbitalDetails info;
      info.occupied = i < nOcc;

      if (info.occupied) {
        info.index = i;
        if (i == nOcc - 1) {
          info.label = QString("%1 (HOMO)").arg(i);
        } else {
          info.label = QString("%1 (HOMO-%2)").arg(i).arg(nOcc - 1 - i);
        }
      } else {
        info.index = i;
        if (i == nOcc) {
          info.label = QString("%1 (LUMO)").arg(i);
        } else {
          info.label = QString("%1 (LUMO+%2)").arg(i).arg(i - nOcc);
        }
      }

      // Filter based on orbital type selection
      const int typeIndex = ui->orbitalTypeComboBox->currentIndex();
      if (typeIndex == 0 && !info.occupied)
        continue; // Occupied only
      if (typeIndex == 1 && info.occupied)
        continue; // Virtual only

      m_orbitalLabels.push_back(info);

      // Add to list widget with energy if available
      QString displayText = info.label;
      if (energies.size() > i) {
        double energy = energies[i];
        displayText += QString(" (%.3f au)").arg(energy);
      }

      auto *item = new QListWidgetItem(displayText);
      item->setData(Qt::UserRole, i); // Store orbital index
      ui->orbitalSelectionListWidget->addItem(item);
    }
  } else {
    // No wavefunction yet - show generic labels
    const int defaultNumOrbitals = 4; // Show reasonable number of options

    for (int i = defaultNumOrbitals - 1; i >= 0; --i) {
      isosurface::OrbitalDetails info;
      info.occupied = true;
      info.index = i;
      info.label = (i == defaultNumOrbitals - 1)
                       ? "HOMO"
                       : QString("HOMO-%1").arg(defaultNumOrbitals - 1 - i);

      if (ui->orbitalTypeComboBox->currentIndex() != 1) { // Not virtual only
        m_orbitalLabels.push_back(info);
        ui->orbitalSelectionListWidget->addItem(info.label);
      }
    }

    for (int i = 0; i < defaultNumOrbitals; ++i) {
      isosurface::OrbitalDetails info;
      info.occupied = false;
      info.index = defaultNumOrbitals + i;
      info.label = (i == 0) ? "LUMO" : QString("LUMO+%1").arg(i);

      if (ui->orbitalTypeComboBox->currentIndex() != 0) { // Not occupied only
        m_orbitalLabels.push_back(info);
        ui->orbitalSelectionListWidget->addItem(info.label);
      }
    }
  }
}

void SurfaceGenerationDialog::setupOrbitalUI() {
  // Replace the simple combobox with a more flexible list widget
  ui->orbitalSelectionListWidget->setSelectionMode(
      QAbstractItemView::ExtendedSelection);

  // Add orbital type selector
  ui->orbitalTypeComboBox->addItems({"Occupied", "Virtual", "All"});

  connect(ui->orbitalTypeComboBox,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &SurfaceGenerationDialog::updateOrbitalLabels);

  connect(
      ui->orbitalSelectionListWidget, &QListWidget::itemSelectionChanged, this,
      [this]() {
        // Enable the OK button only if at least one orbital is selected
        bool hasSelection =
            !ui->orbitalSelectionListWidget->selectedItems().isEmpty();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(hasSelection);
      });
}

bool SurfaceGenerationDialog::needIsovalueBox() {
  const auto &currentSurface = ui->surfaceComboBox->currentSurfaceDescription();
  const auto &currentSurfaceProperty =
      ui->propertyComboBox->currentSurfacePropertyDescription();
  return currentSurface.needsIsovalue;
}

bool SurfaceGenerationDialog::shouldAlsoCalculateNegativeIsovalue() const {
  const auto &currentSurface = ui->surfaceComboBox->currentSurfaceDescription();
  return currentSurface.computeNegativeIsovalue;
}

bool SurfaceGenerationDialog::needClusterOptions() {
  const auto &currentSurface = ui->surfaceComboBox->currentSurfaceDescription();
  return currentSurface.needsCluster;
}

bool SurfaceGenerationDialog::needOrbitalBox() {
  const auto &currentSurface = ui->surfaceComboBox->currentSurfaceDescription();
  const auto &currentSurfaceProperty =
      ui->propertyComboBox->currentSurfacePropertyDescription();
  return currentSurface.needsOrbital || currentSurfaceProperty.needsOrbital;
}

void SurfaceGenerationDialog::updateWavefunctionComboBox(bool selectLast) {
  ui->wavefunctionBox->setVisible(needWavefunction());

  if (needWavefunction()) {
    ui->wavefunctionCombobox->clear();

    ui->wavefunctionCombobox->addItem(tr("New wavefunction"));

    for (const auto &[wavefunction, transform] : m_availableWavefunctions) {
      if (wavefunction) {
        ui->wavefunctionCombobox->addItem(wavefunction->description());
      }
    }
  }

  if (selectLast) {
    ui->wavefunctionCombobox->setCurrentIndex(
        ui->wavefunctionCombobox->count() - 1);
  }
}

bool SurfaceGenerationDialog::needWavefunction() {
  const auto &currentSurface = ui->surfaceComboBox->currentSurfaceDescription();
  const auto &currentSurfaceProperty =
      ui->propertyComboBox->currentSurfacePropertyDescription();
  qDebug() << "Current surface" << currentSurface.displayName
           << currentSurface.needsWavefunction
           << "Current property: " << currentSurfaceProperty.occName
           << currentSurfaceProperty.needsWavefunction;
  return currentSurface.needsWavefunction ||
         currentSurfaceProperty.needsWavefunction;
}

void SurfaceGenerationDialog::updateDescriptions() {
  bool hideDescriptions =
      ui->showDescriptionsCheckBox->checkState() == Qt::Unchecked;
  ui->surfaceDescriptionLabel->setHidden(hideDescriptions);
  ui->propertyDescriptionLabel->setHidden(hideDescriptions);
  if (!hideDescriptions) {
    const auto &currentSurface =
        ui->surfaceComboBox->currentSurfaceDescription();
    const auto &currentSurfaceProperty =
        ui->propertyComboBox->currentSurfacePropertyDescription();
    ui->surfaceDescriptionLabel->setText(currentSurface.description);
    ui->propertyDescriptionLabel->setText(currentSurfaceProperty.description);
  }
  adjustSize();
}

const std::vector<GenericAtomIndex> &
SurfaceGenerationDialog::atomIndices() const {
  return m_atomIndices;
}
