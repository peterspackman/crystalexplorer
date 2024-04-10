#include "childpropertycontroller.h"

#include <QCheckBox>
#include <QDebug>
#include <QLocale>

#include "settings.h"
#include "surfacedescription.h"

ChildPropertyController::ChildPropertyController(QWidget *parent) : QWidget(parent),
    m_meshPropertyModel(new MeshPropertyModel(this)) {

  setupUi(this);
  setup();
}

void ChildPropertyController::setup() {
  _updateSurfacePropertyRange = true;

  surfacePropertyComboBox->setModel(m_meshPropertyModel);
  surfacePropertyComboBox2->setModel(m_meshPropertyModel);

  connect(m_meshPropertyModel, &QAbstractItemModel::modelReset, this, &ChildPropertyController::onMeshModelUpdate);
  connect(m_meshPropertyModel, &QAbstractItemModel::dataChanged, this, &ChildPropertyController::onMeshModelUpdate);

  // Options page = 0
  tabWidget->setCurrentIndex(0);

  connect(enableTransparencyCheckBox, &QCheckBox::toggled, this,
          &ChildPropertyController::onSurfaceTransparencyChange);

  connect(surfacePropertyComboBox, &QComboBox::currentTextChanged,
          this, &ChildPropertyController::onComboBoxPropertySelectionChanged);

  connect(surfacePropertyComboBox2, &QComboBox::currentTextChanged,
          this, &ChildPropertyController::onComboBoxPropertySelectionChanged);

  // Assuming your model emits a signal when the current selection changes
  connect(m_meshPropertyModel, &MeshPropertyModel::propertySelectionChanged,
            this, &ChildPropertyController::onModelPropertySelectionChanged);

  connect(showFingerprintButton, &QToolButton::clicked, this,
          &ChildPropertyController::showFingerprint);

  connect(minPropSpinBox, &QDoubleSpinBox::valueChanged, this,
          [&](double x) { minPropertyChanged(); });
  connect(maxPropSpinBox, &QDoubleSpinBox::valueChanged, this,
          [&](double x) { maxPropertyChanged(); });

  connect(resetPropScaleButton, &QPushButton::clicked, this,
          &ChildPropertyController::resetScale);
  connect(exportSurfaceButton, &QPushButton::clicked, this,
          &ChildPropertyController::exportButtonClicked);

  showSurfaceTabs(false);
  showWavefunctionTabs(false);

  enableFingerprintButton(false);
}

// Doesn't apply to fingerprint button
void ChildPropertyController::enableSurfaceControls(bool enable) {
  enableTransparencyCheckBox->setEnabled(enable);
  surfacePropertyComboBox->setEnabled(enable);
  surfacePropertyComboBox2->setEnabled(enable);
  minPropSpinBox->setEnabled(enable);
  maxPropSpinBox->setEnabled(enable);
  resetPropScaleButton->setEnabled(enable);
}

void ChildPropertyController::setSelectedPropertyValue(float value) {
    selectedPropValue->setValue(value);
}

void ChildPropertyController::onMeshModelUpdate() {
    qDebug() << "Called onMeshModelUpdate";
    bool valid = m_meshPropertyModel->isValid();
    // Enable widgets
    setEnabled(valid);
    enableSurfaceControls(valid);

    volumeValue->setValue(m_meshPropertyModel->volume());
    areaValue->setValue(m_meshPropertyModel->area());

    globularityValue->setValue(m_meshPropertyModel->globularity());
    asphericityValue->setValue(m_meshPropertyModel->asphericity());

    enableFingerprintButton(m_meshPropertyModel->isFingerprintable());
    enableTransparencyCheckBox->setChecked(m_meshPropertyModel->isTransparent());
    qDebug() << "Finished onMeshModelUpdate";

}

void ChildPropertyController::showTab(QWidget *tab, bool show, QString title) {
    if(show) {
	tabWidget->insertTab(0, tab, title);
    }
    else {
	int index = tabWidget->indexOf(tab);
	if (index > -1) { // If the widget is found within the tab widget
	    tabWidget->removeTab(index);
	}
    }
}

void ChildPropertyController::showSurfaceTabs(bool show) {
    // inserted at 0, so show them in reverse order
    showTab(surfacePropertyTab, show, "Property");
    showTab(surfaceInformationTab, show, "Info");
    showTab(surfaceOptionsTab, show, "Options");
    tabWidget->setCurrentIndex(0);
}

void ChildPropertyController::showWavefunctionTabs(bool show) {
    // inserted at 0, so show them in reverse order
    showTab(wavefunctionTab, show, "Wavefunction");
    tabWidget->setCurrentIndex(0);
}

void ChildPropertyController::setUnitLabels(QString units) {
  unitText->setText(units);
  unitsLabel->setText(units);
}

void ChildPropertyController::setCurrentMesh(Mesh *mesh) {
    showSurfaceTabs(true);
    showWavefunctionTabs(false);

    m_meshPropertyModel->setMesh(mesh);
}

void ChildPropertyController::setCurrentMeshInstance(MeshInstance *mi) {
    showSurfaceTabs(true);
    showWavefunctionTabs(false);
    m_meshPropertyModel->setMeshInstance(mi);
}

void ChildPropertyController::setCurrentWavefunction(MolecularWavefunction *wfn) {
    showWavefunctionTabs(true);
    showSurfaceTabs(false);
    bool valid = wfn != nullptr;
    if(valid) {
	QLocale locale;
	chargeValue->setValue(wfn->charge());
	multiplicityValue->setValue(wfn->multiplicity());
	methodValue->setText(wfn->method());
	basisValue->setText(wfn->basis());
	fileSizeValue->setText(locale.formattedDataSize(wfn->fileSize()));
	numBasisValue->setValue(wfn->numberOfBasisFunctions());
	scfValue->setValue(wfn->totalEnergy());
	fileFormatValue->setText(wfn::fileFormatString(wfn->fileFormat()));
    }
    setEnabled(valid);
}

void ChildPropertyController::onSurfaceTransparencyChange(bool transparent) {
    m_meshPropertyModel->setTransparent(transparent);
}

void ChildPropertyController::onComboBoxPropertySelectionChanged(QString property) {
    m_meshPropertyModel->setSelectedProperty(property);
}

void ChildPropertyController::onModelPropertySelectionChanged(QString property) {
    surfacePropertyComboBox->blockSignals(true);
    surfacePropertyComboBox->setCurrentText(property);
    surfacePropertyComboBox->blockSignals(false);

    surfacePropertyComboBox2->blockSignals(true);
    surfacePropertyComboBox2->setCurrentText(property);
    surfacePropertyComboBox2->blockSignals(false);

    const auto &stats = m_meshPropertyModel->getSelectedPropertyStatistics();
    const auto &range = m_meshPropertyModel->getSelectedPropertyRange();

    minPropValue->setValue(stats.lower);
    meanPropValue->setValue(stats.mean);
    maxPropValue->setValue(stats.upper);
    setScale(range.lower, range.upper);

    setUnitLabels("units");
    setSelectedPropertyValue(0.0);

}

void ChildPropertyController::enableFingerprintButton(bool enable) {
  showFingerprintButton->setEnabled(enable);
}

// Only called when the property has changed.
// Warning, if called at other times it might not do what you expect since
// (i) auto color scale always gets turned on (ii) scale range values get
// clamped
void ChildPropertyController::setScale(float minScale, float maxScale) {
  clampScale(minScale, maxScale);
}

/*!
 Converts the encoded name to a natural name via the surface propertymap,
 'propertyFromString'
 defined in surfacedescription.h
 */
QString ChildPropertyController::convertToNaturalPropertyName(QString encodedName) {
  auto prop = IsosurfacePropertyDetails::typeFromTontoName(encodedName);
  Q_ASSERT(prop != IsosurfacePropertyDetails::Type::Unknown);
  return IsosurfacePropertyDetails::getAttributes(prop).name;
}

void ChildPropertyController::resetScale() {
  // Complete hack
  // when resetting I want to go back to the min and max property values
  // I don't have directly but I can copy them from the
  // 3rd tab of surface controller which contains values
  // (including min and max) for the current property
  clampScale(minPropValue->value(), maxPropValue->value(), true);
}

void ChildPropertyController::clampScale(float minScale, float maxScale,
                                   bool emitUpdateSurfacePropertyRangeSignal) {
  // Stop the SpinBoxes firing signals and so redrawing the surface.
  _updateSurfacePropertyRange = false;

  // Certain properties have fixed ranges for the values they can take
  // These properties are stored in the clampedProperties list as *encoded*
  // names
  // since these are invariant (the natural names may change so we don't want to
  // compare against them).
  bool appliedClamp = false;
  for (int i = 0; i < clampedProperties.count(); i++) {
    QString clampedProperty =
        convertToNaturalPropertyName(clampedProperties[i]);
    QString currentProperty =
        surfacePropertyComboBox->currentText();
    if (clampedProperty == currentProperty) {
      setMinAndMaxSpinBoxes(clampedMinimumScaleValues[i],
                            clampedMaximumScaleValues[i]);
      appliedClamp = true;
    }
  }
  // If we didn't find a default clamp then we resort to clamping the scale
  // range
  // to the original scale range. This was saved when the property was first
  // selected.
  if (!appliedClamp) {
    setMinAndMaxSpinBoxes(minScale, maxScale);
  }
  _updateSurfacePropertyRange = true;
  if (emitUpdateSurfacePropertyRangeSignal) {
    updateSurfacePropertyRange();
  }
}

void ChildPropertyController::setMinAndMaxSpinBoxes(float min, float max) {
  const float DEFAULT_MIN_SCALE = -99.99f;
  minPropSpinBox->setValue(
      DEFAULT_MIN_SCALE);        // workaround to prevent issues with min >= max
  maxPropSpinBox->setValue(max); // maxValue need to be set before minValue
  minPropSpinBox->setValue(min);
}

void ChildPropertyController::minPropertyChanged() {
  double minValue = minPropSpinBox->value();
  double maxValue = maxPropSpinBox->value();

  // Prevent min value from exceeding the max value.
  if (minValue >= maxValue) {
    minPropSpinBox->setValue(maxValue - minPropSpinBox->singleStep());
  }
  if (_updateSurfacePropertyRange) {
    updateSurfacePropertyRange();
  }
}

void ChildPropertyController::maxPropertyChanged() {
  double minValue = minPropSpinBox->value();
  double maxValue = maxPropSpinBox->value();

  // Prevent max value from being smaller than the min value.
  if (maxValue <= minValue) {
    maxPropSpinBox->setValue(minValue + maxPropSpinBox->singleStep());
  }
  if (_updateSurfacePropertyRange) {
    updateSurfacePropertyRange();
  }
}

void ChildPropertyController::updateSurfacePropertyRange() {
  emit surfacePropertyRangeChanged(minPropSpinBox->value(),
                                   maxPropSpinBox->value());
}

void ChildPropertyController::currentSurfaceVisibilityChanged(bool visible) {
  enableSurfaceControls(visible);
}

void ChildPropertyController::exportButtonClicked() { emit exportCurrentSurface(); }
