#include "surfacecontroller.h"

#include <QCheckBox>
#include <QDebug>

#include "settings.h"
#include "surfacedescription.h"

SurfaceController::SurfaceController(QWidget *parent) : QWidget(parent),
    m_meshPropertyModel(new MeshPropertyModel(this)) {

  setupUi(this);
  setup();
}

void SurfaceController::setup() {
  _updateSurfacePropertyRange = true;

  surfacePropertyComboBox->setModel(m_meshPropertyModel);
  surfacePropertyComboBox2->setModel(m_meshPropertyModel);

  tabWidget->setCurrentIndex(OPTIONS_PAGE);

  connect(enableTransparencyCheckBox, &QCheckBox::toggled, this,
          &SurfaceController::updateSurfaceTransparency);

  connect(surfacePropertyComboBox, &QComboBox::currentIndexChanged,
	  this, &SurfaceController::onPropertySelectionChanged);

  connect(surfacePropertyComboBox2, &QComboBox::currentIndexChanged,
	  this, &SurfaceController::onPropertySelectionChanged);

  connect(showFingerprintButton, &QToolButton::clicked, this,
          &SurfaceController::showFingerprint);

  connect(minPropSpinBox, &QDoubleSpinBox::valueChanged, this,
          [&](double x) { minPropertyChanged(); });
  connect(maxPropSpinBox, &QDoubleSpinBox::valueChanged, this,
          [&](double x) { maxPropertyChanged(); });

  connect(resetPropScaleButton, &QPushButton::clicked, this,
	  &SurfaceController::resetScale);
  connect(exportSurfaceButton, &QPushButton::clicked, this,
          &SurfaceController::exportButtonClicked);

  enableFingerprintButton(false);
}

// Doesn't apply to fingerprint button
void SurfaceController::enableSurfaceControls(bool enable) {
  enableTransparencyCheckBox->setEnabled(enable);
  surfacePropertyComboBox->setEnabled(enable);
  surfacePropertyComboBox2->setEnabled(enable);
  minPropSpinBox->setEnabled(enable);
  maxPropSpinBox->setEnabled(enable);
  resetPropScaleButton->setEnabled(enable);
}

void SurfaceController::setSurfaceInfo(float volume, float area,
                                       float globularity, float asphericity) {
  volumeValue->setValue(volume);
  areaValue->setValue(area);

  globularityValue->setValue(globularity);
  asphericityValue->setValue(asphericity);
}

void SurfaceController::clearPropertyInfo() {
  selectedPropValue->setValue(0.0);
  minPropValue->setValue(0.0);
  meanPropValue->setValue(0.0);
  maxPropValue->setValue(0.0);
  setScale(0.0, 0.0);
}

void SurfaceController::setSelectedPropertyValue(float value) {
  selectedPropValue->setValue(value);
}

void SurfaceController::setMeshPropertyInfo(const Mesh::ScalarPropertyValues &values) {
  Q_ASSERT(values.size() > 0);

  minPropValue->setValue(values.minCoeff());
  meanPropValue->setValue(values.mean());
  maxPropValue->setValue(values.maxCoeff());

  setScale(values.minCoeff(), values.maxCoeff());

  setUnitLabels("units");
  setSelectedPropertyValue(0.0);

}

void SurfaceController::setUnitLabels(QString units) {
  unitText->setText(units);
  unitsLabel->setText(units);
}

void SurfaceController::setCurrentMesh(Mesh *mesh) {
  bool enableController = false;
  bool enableControls = false;
  bool enableFingerprint = false;

  float volume = 0.0;
  float area = 0.0;
  float globularity = 0.0;
  float asphericity = 0.0;
  bool transparent = false;
  QStringList propertyNames;
  _currentPropertyIndex = 0;

  if (mesh) {
    enableController = true;
    enableControls = true;
    enableFingerprint = false; //surface->isFingerprintable();

    volume = mesh->volume();
    area = mesh->surfaceArea();
    globularity = mesh->globularity();
    asphericity = mesh->asphericity();
    transparent = mesh->isTransparent();
    propertyNames = mesh->availableVertexProperties();
    _currentPropertyIndex = mesh->currentVertexPropertyIndex();
    if (!mesh->availableVertexProperties().isEmpty()) {
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

void SurfaceController::onPropertySelectionChanged(int propertyIndex) {
    if (propertyIndex < 0) return;
    m_meshPropertyModel->setSelectedProperty(propertyIndex);
    setMeshPropertyInfo(m_meshPropertyModel->getPropertyValuesAtIndex(propertyIndex));
}

void SurfaceController::enableFingerprintButton(bool enable) {
  showFingerprintButton->setEnabled(enable);
}

// Only called when the property has changed.
// Warning, if called at other times it might not do what you expect since
// (i) auto color scale always gets turned on (ii) scale range values get
// clamped
void SurfaceController::setScale(float minScale, float maxScale) {
  clampScale(minScale, maxScale);
}

/*!
 Converts the encoded name to a natural name via the surface propertymap,
 'propertyFromString'
 defined in surfacedescription.h
 */
QString SurfaceController::convertToNaturalPropertyName(QString encodedName) {
  auto prop = IsosurfacePropertyDetails::typeFromTontoName(encodedName);
  Q_ASSERT(prop != IsosurfacePropertyDetails::Type::Unknown);
  return IsosurfacePropertyDetails::getAttributes(prop).name;
}

void SurfaceController::resetScale() {
  // Complete hack
  // when resetting I want to go back to the min and max property values
  // I don't have directly but I can copy them from the
  // 3rd tab of surface controller which contains values
  // (including min and max) for the current property
  clampScale(minPropValue->value(), maxPropValue->value(), true);
}

void SurfaceController::clampScale(float minScale, float maxScale,
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
        surfacePropertyComboBox->itemText(_currentPropertyIndex);
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

void SurfaceController::setMinAndMaxSpinBoxes(float min, float max) {
  const float DEFAULT_MIN_SCALE = -99.99f;
  minPropSpinBox->setValue(
      DEFAULT_MIN_SCALE);        // workaround to prevent issues with min >= max
  maxPropSpinBox->setValue(max); // maxValue need to be set before minValue
  minPropSpinBox->setValue(min);
}

void SurfaceController::minPropertyChanged() {
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

void SurfaceController::maxPropertyChanged() {
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

void SurfaceController::updateSurfacePropertyRange() {
  emit surfacePropertyRangeChanged(minPropSpinBox->value(),
                                   maxPropSpinBox->value());
}

void SurfaceController::currentSurfaceVisibilityChanged(bool visible) {
  enableSurfaceControls(visible);
}

void SurfaceController::exportButtonClicked() { emit exportCurrentSurface(); }
