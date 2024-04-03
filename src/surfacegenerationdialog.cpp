#include "surfacegenerationdialog.h"
#include "crystalx.h"
#include "globals.h" // only NEW_WAVEFUNCTION_ITEM
#include "settings.h"
#include "surfaceproperty.h"

#include <QDebug>
#include <QSignalBlocker>

SurfaceGenerationDialog::SurfaceGenerationDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::SurfaceGenerationDialog()) {
  ui->setupUi(this);
  init();
  initConnections();
}

void SurfaceGenerationDialog::init() {
  const auto &currentSurfaceAttributes =
      IsosurfaceDetails::getAttributes(IsosurfaceDetails::defaultType());
  ui->propertyComboBox->onSurfaceTypeChanged(ui->surfaceComboBox->currentType());

  ui->isovalueLineEdit->setText(
      QString::number(currentSurfaceAttributes.defaultIsovalue));
  ui->comboBoxHL->insertItems(0, orbitalLabels);

  surfaceChanged();
  _waitingOnWavefunction = false;

  _charge = 0; // default value but should be set with setChargeForCalculation
  _multiplicity = 1;
  updateSettings();
}

void SurfaceGenerationDialog::initConnections() {
  connect(ui->showDescriptionsCheckBox,
          &QCheckBox::stateChanged, [&](int state) {
              this->updateDescriptions();
          });

  connect(ui->surfaceComboBox, &SurfaceTypeDropdown::surfaceTypeChanged,
          ui->propertyComboBox, &SurfacePropertyTypeDropdown::onSurfaceTypeChanged);

  connect(ui->surfaceComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(surfaceChanged()));
  connect(this, SIGNAL(accepted()), this, SLOT(validate()));
  connect(ui->comboBoxHL, SIGNAL(activated(int)), this, SLOT(setSignLabel(int)));
  connect(ui->useUserDefinedCluster, SIGNAL(toggled(bool)),
          ui->voidClusterPaddingSpinBox, SLOT(setEnabled(bool)));
}

void SurfaceGenerationDialog::connectPropertyComboBox(bool makeConnection) {
  if (makeConnection) {
    connect(ui->propertyComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(propertyChanged()));
  } else {
    disconnect(ui->propertyComboBox, SIGNAL(currentIndexChanged(int)), this,
               SLOT(propertyChanged()));
  }
}

void SurfaceGenerationDialog::setSuitableWavefunctions(
    QVector<TransformableWavefunction> wavefunctions) {
  _wavefunctions = wavefunctions;
  updateWavefunctionComboBox(true);
}

void SurfaceGenerationDialog::setWavefunctionDone(
    TransformableWavefunction wavefunction) {
  _wavefunctions.append(wavefunction);
  _waitingOnWavefunction = false;
  updateWavefunctionComboBox(true);
  validate();
}

bool SurfaceGenerationDialog::wavefunctionIsValid(
    Wavefunction *wavefunction, const QVector<AtomId> &atoms) const {
  bool result = false;
  if (wavefunction) {
    result = wavefunction->isValid(atoms);
  }
  return result;
}

bool SurfaceGenerationDialog::mustCalculateWavefunction() {
  return needWavefunction() &&
         (ui->wavefunctionCombobox->currentText() == NEW_WAVEFUNCTION_ITEM);
}

void SurfaceGenerationDialog::validate() {
    /*
  if (mustCalculateWavefunction()) {
    Q_ASSERT(_atomsForCalculation.size() > 0);
    _waitingOnWavefunction = true;
    emit requireWavefunction(_atomsForCalculation, _charge, _multiplicity);
    return;
  }

  JobParameters jobParams;

  jobParams.jobType = JobType::surfaceGeneration;
  jobParams.surfaceType = ui->surfaceComboBox->currentType();
  jobParams.requestedPropertyType = ui->propertyComboBox->currentType();
  jobParams.isovalue = ui->isovalueLineEdit->text().toFloat();
  jobParams.editInputFile = ui->editCheckBox->isChecked();
  jobParams.resolution = ui->resolutionComboBox->currentLevel();
  if (ui->clusterBox->isEnabled() && !ui->clusterBox->isHidden()) {
    if (ui->useUnitCellPlusFiveAng->isChecked()) {
      jobParams.voidClusterPadding = VOID_UNITCELL_PADDING;
    } else {
      jobParams.voidClusterPadding = ui->voidClusterPaddingSpinBox->value();
    }
  }
  jobParams.molecularOrbitalType = static_cast<OrbitalType>(
      orbitalLabels.indexOf(ui->comboBoxHL->currentText()));
  jobParams.molecularOrbitalLevel = ui->spinBoxHL->value();
  jobParams.program = ExternalProgram::None;
  jobParams.atoms = _atomsForCalculation;
  jobParams.atomsToSuppress = _suppressedAtomsForCalculation;
  jobParams.charge = _charge;
  jobParams.multiplicity = _multiplicity;

  std::optional<Wavefunction> wfn = {};

  if (needWavefunction()) {
    TransformableWavefunction transWavefunction =
        wavefunctionForCurrentComboboxSelection();
    copyWavefunctionParamsIntoSurfaceParams(
        jobParams, transWavefunction.first.jobParameters());
    jobParams.wavefunctionTransforms.clear();
    jobParams.wavefunctionTransforms.append(transWavefunction.second);
    wfn = transWavefunction.first;
  }

  emit surfaceParametersChosen(jobParams, wfn);
  */
  // NEW

  isosurface::Parameters parameters;
  parameters.isovalue = ui->isovalueLineEdit->text().toFloat();
  parameters.kind = ui->surfaceComboBox->currentKind();
  parameters.separation = ResolutionDetails::value(ui->resolutionComboBox->currentLevel());
  qDebug() << isosurface::kindToString(parameters.kind);

  if(needWavefunction()) {
      qDebug() << "Needs wavefunction";
      wfn::Parameters wfn_params;
      emit surfaceParametersChosenNeedWavefunction(parameters, wfn_params);
  }
  else {
      emit surfaceParametersChosenNew(parameters);
  }
}

// Returns the corrsponding wavefunction for a currently selected entry in the
// wavefunction combobox
// The only tricky part is we have the entry NEW_WAVEFUNCTION_ITEM which doesn't
// correspond
// to a wavefunction. If this entries index is lower than the current index then
// we need to
// substract one to get the proper index into _wavefunctions.
TransformableWavefunction
SurfaceGenerationDialog::wavefunctionForCurrentComboboxSelection() {
  int newWavefunctionEntry =
      ui->wavefunctionCombobox->findText(NEW_WAVEFUNCTION_ITEM);

  int index = ui->wavefunctionCombobox->currentIndex();

  if (newWavefunctionEntry < index) {
    index--;
  }

  return _wavefunctions[index];
}

void SurfaceGenerationDialog::copyWavefunctionParamsIntoSurfaceParams(
    JobParameters &jobParams, const JobParameters &jobParamsForWavefunction) {
  if (jobParamsForWavefunction.program == ExternalProgram::None)
    return;
  // Did we have to generate a wavefunction for this surface?
  jobParams.program = jobParamsForWavefunction.program;
  jobParams.theory = jobParamsForWavefunction.theory;
  jobParams.basisset = jobParamsForWavefunction.basisset;
  jobParams.charge = jobParamsForWavefunction.charge;
  jobParams.multiplicity = jobParamsForWavefunction.multiplicity;
  jobParams.atoms = jobParamsForWavefunction.atoms;
}

void SurfaceGenerationDialog::updateSettings() {
  ui->resolutionComboBox->setCurrentIndex(static_cast<int>(ResolutionDetails::defaultLevel()));
  ui->comboBoxHL->setCurrentIndex(defaultOrbitalType);
  ui->surfaceOptionsBox->setHidden(defaultHideSurfaceOptionsBox);
  ui->editCheckBox->setCheckState(defaultEditTonto);
  ui->showDescriptionsCheckBox->setCheckState(defaultShowDescriptions);

  updateDescriptions();
  adjustSize();
}

void SurfaceGenerationDialog::surfaceChanged() {
  updateSurfaceOptions(ui->surfaceComboBox->currentIndex());

  // always set the isovalue since required by hirshfeld surface but shouldn't
  // be modifiable for hirshfeld.
  ui->isovalueLineEdit->setText(QString::number(ui->surfaceComboBox->currentSurfaceAttributes().defaultIsovalue));
  updateWavefunctionComboBox();
  updateDescriptions();
  adjustSize();
}

void SurfaceGenerationDialog::updatePropertyComboBox(
    IsosurfaceDetails::Type surface) {
  blockSignals(true);
  ui->propertyComboBox->clear();
  for (auto property : IsosurfaceDetails::getRequestableProperties(surface)) {
    ui->propertyComboBox->addItem(
        IsosurfacePropertyDetails::getAttributes(property).name);
  }

  ui->propertyComboBox->setEnabled(havePropertyChoices());

  blockSignals(false);

  // select default property
}

bool SurfaceGenerationDialog::havePropertyChoices() {
  Q_ASSERT(ui->propertyComboBox->count() > 0);

  return ui->propertyComboBox->count() > 1;
}

void SurfaceGenerationDialog::propertyChanged() {
  updatePropertyOptions();
  updateWavefunctionComboBox();
  updateDescriptions();
  adjustSize();
}

void SurfaceGenerationDialog::updatePropertyOptions() {
  updateOrbitalOptions();
}

void SurfaceGenerationDialog::updateSurfaceOptions(int surfaceItem) {
  bool hideSurfaceOptions = true;
  ui->isovalueBox->setHidden(true);
  ui->clusterBox->setHidden(true);

  if (needIsovalueBox()) {
    hideSurfaceOptions = false;
    ui->isovalueBox->setHidden(false);
    ui->unitLabel->setText(surfaceIsovalueUnits[surfaceItem]);
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
  const auto &currentSurface = ui->surfaceComboBox->currentSurfaceAttributes();
  const auto &currentSurfaceProperty = ui->propertyComboBox->currentSurfacePropertyAttributes();
  return currentSurface.needsIsovalue || currentSurfaceProperty.needsIsovalue;
}

bool SurfaceGenerationDialog::needClusterOptions() {
    const auto &currentSurface = ui->surfaceComboBox->currentSurfaceAttributes();
    return currentSurface.needsClusterOptions;
}

bool SurfaceGenerationDialog::needOrbitalBox() {
    const auto &currentSurface = ui->surfaceComboBox->currentSurfaceAttributes();
    const auto &currentSurfaceProperty = ui->propertyComboBox->currentSurfacePropertyAttributes();
    return currentSurface.needsOrbitals || currentSurfaceProperty.needsOrbitals;
}

void SurfaceGenerationDialog::updateWavefunctionComboBox(bool selectLast) {
  ui->wavefunctionBox->setVisible(needWavefunction());

  if (needWavefunction()) {
    ui->wavefunctionCombobox->clear();

    ui->wavefunctionCombobox->addItem(NEW_WAVEFUNCTION_ITEM);

    foreach (TransformableWavefunction wavefunction, _wavefunctions) {
      ui->wavefunctionCombobox->addItem(wavefunction.first.description());
    }
  }

  if (selectLast) {
    ui->wavefunctionCombobox->setCurrentIndex(ui->wavefunctionCombobox->count() - 1);
  }
}

bool SurfaceGenerationDialog::needWavefunction() {
    const auto &currentSurface = ui->surfaceComboBox->currentSurfaceAttributes();
    const auto &currentSurfaceProperty = ui->propertyComboBox->currentSurfacePropertyAttributes();
    return currentSurface.needsWavefunction ||
            currentSurfaceProperty.needsWavefunction;
}

void SurfaceGenerationDialog::updateDescriptions() {
  bool hideDescriptions =
      ui->showDescriptionsCheckBox->checkState() == Qt::Unchecked;
  ui->surfaceDescriptionLabel->setHidden(hideDescriptions);
  ui->propertyDescriptionLabel->setHidden(hideDescriptions);
  if (!hideDescriptions) {
      const auto &currentSurface = ui->surfaceComboBox->currentSurfaceAttributes();
      const auto &currentSurfaceProperty = ui->propertyComboBox->currentSurfacePropertyAttributes();
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


const std::vector<GenericAtomIndex>& SurfaceGenerationDialog::atomIndices() const {
    return m_atomIndices;
}
