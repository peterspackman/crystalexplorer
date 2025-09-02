#include "surfacegenerationdialog.h"
#include "globalconfiguration.h"
#include "globals.h"
#include "settings.h"

#include <QPushButton>

#include <QDebug>
#include <QSignalBlocker>
#include <QStandardItemModel>
#include <algorithm>

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
  connect(ui->wavefunctionCombobox, &QComboBox::currentIndexChanged, this,
          &SurfaceGenerationDialog::updateOrbitalLabels);
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

void SurfaceGenerationDialog::setNumberOfElectronsForCalculation(
    int numberOfElectrons) {
  m_numElectrons = numberOfElectrons;
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
  
  // Note: Background density for slab structures is handled directly in isosurface_calculator
  
  parameters.separation =
      isosurface::resolutionValue(ui->resolutionComboBox->currentLevel());
  parameters.fragmentIdentifier = generateFragmentIdentifier();
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

    if (needOrbitalBox()) {
      parameters.orbitalLabels =
          ui->orbitalSelectionWidget->getSelectedOrbitalLabels();
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
  bool needsOrbital = needOrbitalBox();

  if (!needsOrbital) {
    ui->orbitalBox->setVisible(false);
    return;
  }
  ui->orbitalBox->setVisible(true);

  const bool haveExistingWfn = !m_availableWavefunctions.empty() &&
                               ui->wavefunctionCombobox->currentIndex() > 0;

  std::vector<MolecularOrbitalSelector::OrbitalInfo> orbs;
  int nOcc{0};
  int nOrb{0};
  if (haveExistingWfn) {
    // Get current wavefunction
    const auto &[wfn, _] =
        m_availableWavefunctions[ui->wavefunctionCombobox->currentIndex() - 1];
    nOcc = wfn->numberOfOccupiedOrbitals();
    nOrb = wfn->numberOfOrbitals();
    const auto &energies = wfn->orbitalEnergies();
    qDebug() << "nOcc nOrb" << nOcc << nOrb;

    // Generate labels based on actual wavefunction
    for (int i = 0; i < nOrb; ++i) {
      MolecularOrbitalSelector::OrbitalInfo info;
      info.isOccupied = i < nOcc;
      // Create some example data
      info.index = i;
      info.energy = 0.0;
      if (energies.size() > i) {
        info.energy = energies[i];
      }
      info.label = QString("%1 (HOMO)").arg(i);

      if (info.isOccupied) {
        info.index = i;
        if (i == nOcc - 1) {
          info.label = QString("HOMO");
        } else {
          info.label = QString("HOMO-%1").arg(nOcc - 1 - i);
        }
      } else {
        info.index = i;
        if (i == nOcc) {
          info.label = QString("LUMO");
        } else {
          info.label = QString("LUMO+%1").arg(i - nOcc);
        }
      }
      orbs.push_back(info);
    }
  }

  if (orbs.size() > 0) {
    ui->orbitalSelectionWidget->setWavefunctionCalculated(true);
    ui->orbitalSelectionWidget->setOrbitalData(orbs);
    ui->orbitalSelectionWidget->setNumberOfBasisFunctions(nOrb);
    ui->orbitalSelectionWidget->setNumberOfElectrons(nOcc);
  } else {
    ui->orbitalSelectionWidget->setWavefunctionCalculated(false);
    ui->orbitalSelectionWidget->setNumberOfElectrons(m_numElectrons);
    ui->orbitalSelectionWidget->setNumberOfBasisFunctions(
        m_numElectrons + qMin(m_numElectrons, 10));
  }
}

void SurfaceGenerationDialog::setupOrbitalUI() {}

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

void SurfaceGenerationDialog::setStructure(ChemicalStructure *structure) {
  m_structure = structure;
  
  // For slab structures, disable inappropriate surface types instead of removing them
  if (structure && structure->structureType() == ChemicalStructure::StructureType::Surface) {
    // Disable surfaces that don't work properly with 2D slab structures
    for (int i = 0; i < ui->surfaceComboBox->count(); i++) {
      QString surfaceType = ui->surfaceComboBox->itemData(i).toString();
      auto kind = isosurface::stringToKind(surfaceType);
      
      // Disable void surfaces for slab structures (3D crystal packing analysis)
      bool shouldDisable = (kind == isosurface::Kind::Void);
      
      if (shouldDisable) {
        // Disable the item and add explanatory text
        auto model = qobject_cast<QStandardItemModel*>(ui->surfaceComboBox->model());
        if (model) {
          auto item = model->item(i);
          if (item) {
            item->setEnabled(false);
            item->setToolTip("Not available for 2D slab structures");
          }
        }
      }
    }
  }
}

QString SurfaceGenerationDialog::generateFragmentIdentifier() const {
  if (!m_structure || m_atomIndices.empty()) {
    return "Fragment";
  }

  // Use the existing fragment labeling system
  return m_structure->getFragmentLabelForAtoms(m_atomIndices);
}
