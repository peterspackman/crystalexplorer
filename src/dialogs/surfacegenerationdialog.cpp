#include "surfacegenerationdialog.h"
#include "globalconfiguration.h"
#include "globals.h"
#include "settings.h"

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

  ui->surfaceComboBox->setDescriptions(m_surfaceDescriptions);
  ui->propertyComboBox->setDescriptions(m_surfaceDescriptions,
                                        m_surfacePropertyDescriptions);

  updateIsovalue();
  // TODO orbital labels
  // ui->comboBoxHL->insertItems(0, orbitalLabels);

  surfaceChanged(m_currentSurfaceType);

  m_charge = 0; // default value but should be set with setChargeForCalculation
  m_multiplicity = 1;

  updateSettings();
}

void SurfaceGenerationDialog::initConnections() {
  connect(ui->showDescriptionsCheckBox, &QCheckBox::stateChanged,
          [&](int state) { this->updateDescriptions(); });

  connect(ui->surfaceComboBox, &SurfaceTypeDropdown::selectionChanged,
          ui->propertyComboBox,
          &SurfacePropertyTypeDropdown::onSurfaceSelectionChanged);
  ui->surfaceComboBox->setCurrent(m_currentSurfaceType);

  connect(ui->surfaceComboBox, &SurfaceTypeDropdown::selectionChanged, this,
          &SurfaceGenerationDialog::surfaceChanged);

  connect(ui->propertyComboBox, &SurfacePropertyTypeDropdown::selectionChanged, this,
          &SurfaceGenerationDialog::propertyChanged);

  connect(this, &SurfaceGenerationDialog::accepted, this,
          &SurfaceGenerationDialog::validate);
  connect(ui->comboBoxHL, QOverload<int>::of(&QComboBox::activated), this,
          &SurfaceGenerationDialog::setSignLabel);
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
}

QString SurfaceGenerationDialog::currentKindName() const {
  const auto &currentSurface = ui->surfaceComboBox->currentSurfaceDescription();
  return currentSurface.occName;
}
isosurface::Kind SurfaceGenerationDialog::currentKind() const {
  return isosurface::stringToKind(ui->surfaceComboBox->current());
}

QString SurfaceGenerationDialog::currentPropertyName() const {
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
  if (prop != "none") {
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
  ui->comboBoxHL->setCurrentIndex(0);
  ui->surfaceOptionsBox->setHidden(defaultHideSurfaceOptionsBox);
  ui->showDescriptionsCheckBox->setCheckState(defaultShowDescriptions);

  updateDescriptions();
  adjustSize();
}

void SurfaceGenerationDialog::updateIsovalue() {
  double defaultIsovalue =
      m_surfaceDescriptions.value(m_currentSurfaceType).defaultIsovalue;
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

void SurfaceGenerationDialog::updatePropertyOptions() {
  updateOrbitalOptions();
}

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

  updateOrbitalOptions();
}

void SurfaceGenerationDialog::updateOrbitalOptions() {
  ui->orbitalBox->setVisible(needOrbitalBox());
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
  qDebug() << "Current property: " << currentSurfaceProperty.occName
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

void SurfaceGenerationDialog::setSignLabel(int option) {
  if (option == 0) { // HOMO
    ui->signLabel->setText("-");
  } else { // LUMO
    ui->signLabel->setText("+");
  }
}

const std::vector<GenericAtomIndex> &
SurfaceGenerationDialog::atomIndices() const {
  return m_atomIndices;
}
