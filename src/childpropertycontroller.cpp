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

  // Options page = 0
  tabWidget->setCurrentIndex(0);

  connect(enableTransparencyCheckBox, &QCheckBox::toggled, this,
          &ChildPropertyController::onSurfaceTransparencyChange);

  connect(surfacePropertyComboBox, &QComboBox::currentIndexChanged,
          this, &ChildPropertyController::onPropertySelectionChanged);

  connect(surfacePropertyComboBox2, &QComboBox::currentIndexChanged,
          this, &ChildPropertyController::onPropertySelectionChanged);

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

void ChildPropertyController::setSurfaceInfo(float volume, float area,
                                       float globularity, float asphericity) {
  volumeValue->setValue(volume);
  areaValue->setValue(area);

  globularityValue->setValue(globularity);
  asphericityValue->setValue(asphericity);
}

void ChildPropertyController::clearPropertyInfo() {
  selectedPropValue->setValue(0.0);
  minPropValue->setValue(0.0);
  meanPropValue->setValue(0.0);
  maxPropValue->setValue(0.0);
  setScale(0.0, 0.0);
}

void ChildPropertyController::setSelectedPropertyValue(float value) {
  selectedPropValue->setValue(value);
}

void ChildPropertyController::setMeshPropertyInfo(const Mesh::ScalarPropertyValues &values) {
  Q_ASSERT(values.size() > 0);

  minPropValue->setValue(values.minCoeff());
  meanPropValue->setValue(values.mean());
  maxPropValue->setValue(values.maxCoeff());

  setScale(values.minCoeff(), values.maxCoeff());

  setUnitLabels("units");
  setSelectedPropertyValue(0.0);

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
    bool enableController = false;
    bool enableControls = false;
    bool enableFingerprint = false;

    float volume = 0.0;
    float area = 0.0;
    float globularity = 0.0;
    float asphericity = 0.0;
    bool transparent = false;

    if (mesh) {
	enableController = true;
	enableControls = true;
	enableFingerprint = false; //surface->isFingerprintable();

	volume = mesh->volume();
	area = mesh->surfaceArea();
	globularity = mesh->globularity();
	asphericity = mesh->asphericity();
	transparent = mesh->isTransparent();
	if (!mesh->availableVertexProperties().isEmpty()) {
	    // TODO change default property based on surface type
	    // Optionally, set the first item as selected in your comboBox
	    surfacePropertyComboBox->setCurrentIndex(0);

	    // Manually trigger the property info update for the initial selection
	    Mesh::ScalarPropertyValues initialValues = mesh->vertexProperty(mesh->availableVertexProperties().first());
	    setMeshPropertyInfo(initialValues);
	}
    }

    // Enable widgets
    setEnabled(enableController);
    enableSurfaceControls(enableControls);
    enableFingerprintButton(enableFingerprint);
    enableTransparencyCheckBox->setChecked(transparent);

    // Update surface info
    setSurfaceInfo(volume, area, globularity, asphericity);
    m_meshPropertyModel->setMesh(mesh);
}

void ChildPropertyController::setCurrentMeshInstance(MeshInstance *mi) {
    showSurfaceTabs(true);
    showWavefunctionTabs(false);
    bool enableController = false;
    bool enableControls = false;
    bool enableFingerprint = false;

    float volume = 0.0;
    float area = 0.0;
    float globularity = 0.0;
    float asphericity = 0.0;
    bool transparent = false;

    if (mi && mi->mesh()) {
	auto * mesh = mi->mesh();
	enableController = true;
	enableControls = true;
	enableFingerprint = false; //surface->isFingerprintable();

	volume = mesh->volume();
	area = mesh->surfaceArea();
	globularity = mesh->globularity();
	asphericity = mesh->asphericity();
	transparent = mesh->isTransparent();
	if (!mesh->availableVertexProperties().isEmpty()) {
	    // TODO change default property based on surface type
	    // Optionally, set the first item as selected in your comboBox
	    surfacePropertyComboBox->setCurrentIndex(0);

	    // Manually trigger the property info update for the initial selection
	    Mesh::ScalarPropertyValues initialValues = mesh->vertexProperty(mesh->availableVertexProperties().first());
	    setMeshPropertyInfo(initialValues);
	}
    }

    // Enable widgets
    setEnabled(enableController);
    enableSurfaceControls(enableControls);
    enableFingerprintButton(enableFingerprint);
    enableTransparencyCheckBox->setChecked(transparent);

    // Update surface info
    setSurfaceInfo(volume, area, globularity, asphericity);
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

void ChildPropertyController::onPropertySelectionChanged(int propertyIndex) {
    if (propertyIndex < 0) return;
    m_meshPropertyModel->setSelectedProperty(propertyIndex);
    setMeshPropertyInfo(m_meshPropertyModel->getPropertyValuesAtIndex(propertyIndex));
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
