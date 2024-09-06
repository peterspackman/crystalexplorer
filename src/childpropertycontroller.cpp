#include "childpropertycontroller.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QDebug>
#include <QLocale>

#include "settings.h"

ChildPropertyController::ChildPropertyController(QWidget *parent)
    : QWidget(parent), m_meshPropertyModel(new MeshPropertyModel(this)) {

  setupUi(this);

  m_clampedProperties = {{"shape_index", {-1.0f, 1.0f}},
                         {"curvedness", {-4.0f, 0.4f}},
                         {"None", {0.0f, 0.0f}}};
  setup();
}

void ChildPropertyController::setup() {

  surfacePropertyComboBox->setModel(m_meshPropertyModel);
  surfacePropertyComboBox2->setModel(m_meshPropertyModel);

  connect(m_meshPropertyModel, &QAbstractItemModel::modelReset, this,
          &ChildPropertyController::onMeshModelUpdate);
  connect(m_meshPropertyModel, &QAbstractItemModel::dataChanged, this,
          &ChildPropertyController::onMeshModelUpdate);

  // Options page = 0
  tabWidget->setCurrentIndex(0);

  connect(enableTransparencyCheckBox, &QCheckBox::toggled, this,
          &ChildPropertyController::onSurfaceTransparencyChange);

  connect(surfacePropertyComboBox, &QComboBox::currentTextChanged, this,
          &ChildPropertyController::onComboBoxPropertySelectionChanged);

  connect(surfacePropertyComboBox2, &QComboBox::currentTextChanged, this,
          &ChildPropertyController::onComboBoxPropertySelectionChanged);

  // Assuming your model emits a signal when the current selection changes
  connect(m_meshPropertyModel, &MeshPropertyModel::propertySelectionChanged,
          this, &ChildPropertyController::onModelPropertySelectionChanged);

  connect(showFingerprintButton, &QToolButton::clicked, this,
          &ChildPropertyController::showFingerprint);

  connect(minPropSpinBox, &QDoubleSpinBox::valueChanged, this,
          [&](double x) { propertyRangeChanged(); });
  connect(maxPropSpinBox, &QDoubleSpinBox::valueChanged, this,
          [&](double x) { propertyRangeChanged(); });

  connect(resetPropScaleButton, &QPushButton::clicked, this,
          &ChildPropertyController::resetScale);
  connect(exportSurfaceButton, &QPushButton::clicked, this,
          &ChildPropertyController::exportButtonClicked);

  frameworkColorComboBox->blockSignals(true);
  QStringList colorValues = availableFrameworkColoringOptions();
  frameworkColorComboBox->insertItems(0, colorValues);
  componentComboBox->setCurrentIndex(0);
  frameworkColorComboBox->blockSignals(false);

  frameworkConnectionComboBox->blockSignals(true);
  QStringList connectionValues = availableFrameworkConnectionModeOptions();
  frameworkConnectionComboBox->insertItems(0, connectionValues);
  frameworkConnectionComboBox->blockSignals(false);

  frameworkColorToolButton->hide();
  connect(frameworkColorToolButton, &QAbstractButton::clicked, [this]() {
    QColor color = QColorDialog::getColor(m_customFrameworkColor, this);
    if (color.isValid()) {
      m_customFrameworkColor = color;
      onFrameworkColoringChanged();
    }
  });

  // framework
  connect(showLinesButton, &QRadioButton::clicked,
          [this]() { setFrameworkDisplay(FrameworkOptions::Display::Lines); });

  connect(showNoneButton, &QRadioButton::clicked,
          [this]() { setFrameworkDisplay(FrameworkOptions::Display::None); });

  connect(showTubesButton, &QRadioButton::clicked,
          [this]() { setFrameworkDisplay(FrameworkOptions::Display::Tubes); });

  connect(modelComboBox, &QComboBox::currentIndexChanged,
          [this]() { updatePairInteractionComponents(); });

  connect(frameworkTubeSizeSpinBox, &QSpinBox::valueChanged,
          [this]() { emitFrameworkOptions(); });

  connect(frameworkCutoffSpinBox, &QDoubleSpinBox::valueChanged,
          [this]() { emitFrameworkOptions(); });

  connect(componentComboBox, &QComboBox::currentIndexChanged,
          [this]() { emitFrameworkOptions(); });

  connect(frameworkColorComboBox, &QComboBox::currentIndexChanged, this,
          &ChildPropertyController::onFrameworkColoringChanged);

  connect(frameworkConnectionComboBox, &QComboBox::currentIndexChanged,
          [this]() { emitFrameworkOptions(); });

  connect(frameworkShowOnlySelectionCheckBox, &QCheckBox::checkStateChanged,
          [this]() { emitFrameworkOptions(); });

  showSurfaceTabs(false);
  showWavefunctionTabs(false);
  showFrameworkTabs(false);

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
}

void ChildPropertyController::showTab(QWidget *tab, bool show, QString title) {
  if (show) {
    tabWidget->insertTab(0, tab, title);
  } else {
    int index = tabWidget->indexOf(tab);
    if (index > -1) { // If the widget is found within the tab widget
      tabWidget->removeTab(index);
    }
  }
}

void ChildPropertyController::showFrameworkTabs(bool show) {
  // inserted at 0, so show them in reverse order
  showTab(frameworkTab, show, "Framework");
  tabWidget->setCurrentIndex(0);
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
  showFrameworkTabs(false);
  m_state = ChildPropertyController::DisplayState::Mesh;

  m_meshPropertyModel->setMesh(mesh);
}

void ChildPropertyController::setCurrentPairInteractions(PairInteractions *p) {
  if (m_pairInteractions) {
    disconnect(m_pairInteractions, &PairInteractions::interactionAdded, this,
               &ChildPropertyController::updatePairInteractionModels);
    disconnect(m_pairInteractions, &PairInteractions::interactionRemoved, this,
               &ChildPropertyController::updatePairInteractionModels);
  }
  showSurfaceTabs(false);
  showWavefunctionTabs(false);
  showFrameworkTabs(true);
  m_pairInteractions = p;
  m_state = ChildPropertyController::DisplayState::Framework;

  if (m_pairInteractions) {
    setEnabled(m_pairInteractions->getCount() > 0);
    if (m_pairInteractions) {
      connect(m_pairInteractions, &PairInteractions::interactionAdded, this,
              &ChildPropertyController::updatePairInteractionModels);
      connect(m_pairInteractions, &PairInteractions::interactionRemoved, this,
              &ChildPropertyController::updatePairInteractionModels);
    }
    updatePairInteractionModels();
  }
}

void ChildPropertyController::updatePairInteractionModels() {
  if (!m_pairInteractions)
    return;
  QString currentModel = modelComboBox->currentText();
  modelComboBox->blockSignals(true);
  auto values = m_pairInteractions->interactionModels();
  modelComboBox->clear();
  modelComboBox->insertItems(0, values);
  auto idx = values.indexOf(currentModel);
  modelComboBox->setCurrentIndex((idx >= 0) ? idx : 0);
  modelComboBox->blockSignals(false);
  updatePairInteractionComponents();
}

void ChildPropertyController::updatePairInteractionComponents() {
  if (!m_pairInteractions)
    return;

  QString currentComponent = componentComboBox->currentText();
  componentComboBox->blockSignals(true);
  auto values =
      m_pairInteractions->interactionComponents(modelComboBox->currentText());
  qDebug() << "Available components" << values;
  componentComboBox->clear();
  componentComboBox->insertItems(0, values);
  auto idx = values.indexOf(currentComponent);
  componentComboBox->setCurrentIndex((idx >= 0) ? idx : 0);
  componentComboBox->blockSignals(false);

  emitFrameworkOptions();
}

bool ChildPropertyController::showEnergyFramework() const {
  return m_frameworkDisplay != FrameworkOptions::Display::None;
}

bool ChildPropertyController::toggleShowEnergyFramework() {
  setShowEnergyFramework(!showEnergyFramework());
  return showEnergyFramework();
}

void ChildPropertyController::setShowEnergyFramework(bool show) {
  if (!m_pairInteractions || !m_pairInteractions->haveInteractions()) {
    return;
  }

  if (show && m_frameworkDisplay == FrameworkOptions::Display::None) {
    setFrameworkDisplay(m_previousNonNoneDisplay);
  } else if (!show && m_frameworkDisplay != FrameworkOptions::Display::None) {
    m_previousNonNoneDisplay = m_frameworkDisplay;
    setFrameworkDisplay(FrameworkOptions::Display::None);
  }
}

inline void setButtonColor(QAbstractButton *colorButton, QColor color) {
  QPixmap pixmap = QPixmap(colorButton->iconSize());
  pixmap.fill(color);
  colorButton->setIcon(QIcon(pixmap));
}

void ChildPropertyController::onFrameworkColoringChanged() {
  auto coloring =
      frameworkColoringFromString(frameworkColorComboBox->currentText());
  if (coloring == FrameworkOptions::Coloring::Custom) {
    setButtonColor(frameworkColorToolButton, m_customFrameworkColor);
    frameworkColorToolButton->show();
  } else {
    frameworkColorToolButton->hide();
  }
  emitFrameworkOptions();
}

void ChildPropertyController::emitFrameworkOptions() {
  FrameworkOptions options;
  options.model = modelComboBox->currentText();
  options.coloring =
      frameworkColoringFromString(frameworkColorComboBox->currentText());
  options.connectionMode = frameworkConnectionModeFromString(
      frameworkConnectionComboBox->currentText());
  options.customColor = m_customFrameworkColor;
  options.component = componentComboBox->currentText();
  options.scale =
      0.001 * frameworkTubeSizeSpinBox->value(); // convert to A per kJ/mol
  options.cutoff = frameworkCutoffSpinBox->value();
  options.display = m_frameworkDisplay;
  options.showOnlySelectedFragmentInteractions =
      frameworkShowOnlySelectionCheckBox->isChecked();
  emit frameworkOptionsChanged(options);
}

void ChildPropertyController::setCurrentMeshInstance(MeshInstance *mi) {
  showSurfaceTabs(true);
  showWavefunctionTabs(false);
  showFrameworkTabs(false);

  m_state = ChildPropertyController::DisplayState::Mesh;
  m_meshPropertyModel->setMeshInstance(mi);
}

void ChildPropertyController::setCurrentWavefunction(
    MolecularWavefunction *wfn) {
  showWavefunctionTabs(true);
  showSurfaceTabs(false);
  showFrameworkTabs(false);

  m_state = ChildPropertyController::DisplayState::Wavefunction;

  bool valid = wfn != nullptr;
  if (valid) {
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

void ChildPropertyController::onComboBoxPropertySelectionChanged(
    QString property) {
  m_meshPropertyModel->setSelectedProperty(property);
}

void ChildPropertyController::onModelPropertySelectionChanged(
    QString property) {
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
  setScale(range);

  setUnitLabels("units");
  setSelectedPropertyValue(0.0);
}

void ChildPropertyController::enableFingerprintButton(bool enable) {
  // TODO add a tooltip to show why it's not enabled
  showFingerprintButton->setEnabled(enable);
}

// Only called when the property has changed.
// Warning, if called at other times it might not do what you expect since
// (i) auto color scale always gets turned on (ii) scale range values get
// clamped
void ChildPropertyController::setScale(Mesh::ScalarPropertyRange range) {
  QString currentProperty = surfacePropertyComboBox->currentText();
  if (m_clampedProperties.contains(currentProperty)) {
    Mesh::ScalarPropertyRange clampValues =
        m_clampedProperties[currentProperty];
    range.lower = clampValues.lower;
    range.upper = clampValues.upper;
  }

  setMinAndMaxSpinBoxes(range.lower, range.upper);
}

void ChildPropertyController::resetScale() {
  // TODO
  qDebug() << "Scale reset";
}

void ChildPropertyController::setMinAndMaxSpinBoxes(float min, float max) {
  minPropSpinBox->blockSignals(true);
  maxPropSpinBox->blockSignals(true);

  maxPropSpinBox->setValue(max);
  minPropSpinBox->setValue(min);

  minPropSpinBox->blockSignals(false);
  maxPropSpinBox->blockSignals(false);
}

void ChildPropertyController::propertyRangeChanged() {
  double minValue = minPropSpinBox->value();
  double maxValue = maxPropSpinBox->value();

  // Prevent min value from exceeding the max value.
  if (minValue >= maxValue) {
    minPropSpinBox->blockSignals(true);
    minPropSpinBox->setValue(maxValue - minPropSpinBox->singleStep());
    minPropSpinBox->blockSignals(false);
  }
  m_meshPropertyModel->setSelectedPropertyRange(
      {static_cast<float>(minPropSpinBox->value()),
       static_cast<float>(maxPropSpinBox->value())});
}

void ChildPropertyController::currentSurfaceVisibilityChanged(bool visible) {
  enableSurfaceControls(visible);
}

void ChildPropertyController::exportButtonClicked() {
  emit exportCurrentSurface();
}

Mesh *ChildPropertyController::getCurrentMesh() {
  if (m_state != ChildPropertyController::DisplayState::Mesh)
    return nullptr;
  return m_meshPropertyModel->getMesh();
}

void ChildPropertyController::setFrameworkDisplay(
    FrameworkOptions::Display choice) {
  showNoneButton->blockSignals(true);
  showTubesButton->blockSignals(true);
  showLinesButton->blockSignals(true);
  bool noneState = false;
  bool tubeState = false;
  bool lineState = false;
  switch (choice) {
  case FrameworkOptions::Display::Tubes:
    tubeState = true;
    break;
  case FrameworkOptions::Display::Lines:
    lineState = true;
    break;
  default:
    // None
    noneState = true;
    break;
  }
  showNoneButton->setChecked(noneState);
  showTubesButton->setChecked(tubeState);
  showLinesButton->setChecked(lineState);
  showNoneButton->blockSignals(false);
  showTubesButton->blockSignals(false);
  showLinesButton->blockSignals(false);

  // Update the previous non-None display if applicable
  if (choice != FrameworkOptions::Display::None) {
    m_previousNonNoneDisplay = choice;
  }
  m_frameworkDisplay = choice;
  emitFrameworkOptions();
}
