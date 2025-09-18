#include "childpropertycontroller.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QDebug>
#include <QFormLayout>
#include <QLabel>
#include <QLocale>
#include <QVBoxLayout>

#include "plane.h"
#include "planeinstance.h"
#include "settings.h"
#include "icosphere_mesh.h"
#include "meshinstance.h"
#include <occ/core/element.h>

ChildPropertyController::ChildPropertyController(QWidget *parent)
    : QWidget(parent), m_meshPropertyModel(new MeshPropertyModel(this)) {

  setupUi(this);

  m_clampedProperties = {{"shape_index", {-1.0f, 1.0f}},
                         {"curvedness", {-4.0f, 0.4f}},
                         {"None", {0.0f, 0.0f}}};
  
  setup();
}

void ChildPropertyController::reset() {
  qDebug() << "Reset called on childPropertyController";
  // Clear each type without affecting enabled state
  m_meshPropertyModel->setMesh(nullptr);
  m_pairInteractions = nullptr;

  // Reset plane widgets
  if (m_planeInfoWidget) {
    m_planeInfoWidget->setPlane(nullptr);
  }
  if (m_planeInstanceWidget) {
    m_planeInstanceWidget->setPlaneInstance(nullptr);
  }

  // Reset elastic tensor
  m_currentElasticTensor = nullptr;

  // Reset tabs
  showSurfaceTabs(false);
  showWavefunctionTabs(false);
  showFrameworkTabs(false);
  showPlaneTabs(false);
  showPlaneInstanceTabs(false);
  showElasticTensorTabs(false);

  m_state = DisplayState::None;
  setEnabled(false);
}

void ChildPropertyController::setup() {

  surfacePropertyComboBox->setModel(m_meshPropertyModel);
  int lineHeight = surfacePropertyComboBox->sizeHint().height();
  surfacePropertyComboBox->setIconSize(
      QSize(lineHeight * 4.0 / 3, lineHeight * 0.5));
  surfacePropertyComboBox2->setModel(m_meshPropertyModel);
  surfacePropertyComboBox2->setIconSize(
      QSize(lineHeight * 4.0 / 3, lineHeight * 0.5));

  connect(m_meshPropertyModel, &QAbstractItemModel::modelReset, this,
          &ChildPropertyController::onMeshModelUpdate);
  connect(m_meshPropertyModel, &QAbstractItemModel::dataChanged, this,
          &ChildPropertyController::onMeshModelUpdate);

  // Options page = 0
  tabWidget->setCurrentIndex(0);

  connect(enableTransparencyCheckBox, &QCheckBox::toggled, this,
          &ChildPropertyController::onSurfaceTransparencyChange);
  connect(transparencySpinBox, &QDoubleSpinBox::valueChanged, this,
          &ChildPropertyController::onSurfaceVarTransparecyChange);

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
  frameworkColorComboBox->setCurrentIndex(0);
  frameworkColorComboBox->blockSignals(false);

  frameworkLabelDisplayComboBox->blockSignals(true);
  QStringList labelDisplayValues = availableFrameworkLabelDisplayOptions();
  frameworkLabelDisplayComboBox->insertItems(0, labelDisplayValues);
  frameworkLabelDisplayComboBox->setCurrentIndex(0);
  frameworkLabelDisplayComboBox->blockSignals(false);

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

  connect(frameworkTubeSizeSpinBox, &QDoubleSpinBox::valueChanged,
          [this]() { emitFrameworkOptions(); });

  connect(frameworkCutoffSpinBox, &QDoubleSpinBox::valueChanged,
          [this]() { emitFrameworkOptions(); });

  connect(componentComboBox, &QComboBox::currentIndexChanged,
          [this]() { emitFrameworkOptions(); });

  connect(frameworkColorComboBox, &QComboBox::currentIndexChanged, this,
          &ChildPropertyController::onFrameworkColoringChanged);

  connect(frameworkConnectionComboBox, &QComboBox::currentIndexChanged,
          [this]() { emitFrameworkOptions(); });

  connect(frameworkLabelDisplayComboBox, &QComboBox::currentIndexChanged,
          [this]() { emitFrameworkOptions(); });

  connect(frameworkShowOnlySelectionCheckBox, &QCheckBox::checkStateChanged,
          [this]() { emitFrameworkOptions(); });

  showSurfaceTabs(false);
  showWavefunctionTabs(false);
  showFrameworkTabs(false);
  showPlaneTabs(false);
  showPlaneInstanceTabs(false);

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

void ChildPropertyController::showPlaneTabs(bool show) {
  if (show && !m_planePropertiesTab) {
    createPlanePropertiesTab();
  }
  showTab(m_planePropertiesTab, show, "Plane Properties");
  if (show) {
    tabWidget->setCurrentIndex(0);
  }
}

void ChildPropertyController::showPlaneInstanceTabs(bool show) {
  if (show && !m_planeInstancePropertiesTab) {
    createPlaneInstancePropertiesTab();
  }
  showTab(m_planeInstancePropertiesTab, show, "Instance Properties");
  if (show) {
    tabWidget->setCurrentIndex(0);
  }
}

void ChildPropertyController::showElasticTensorTabs(bool show) {
  if (show && !m_elasticTensorPropertiesTab) {
    createElasticTensorPropertiesTab();
  }
  showTab(m_elasticTensorPropertiesTab, show, "Elastic Tensor");
  if (show) {
    tabWidget->setCurrentIndex(0);
  }
}

void ChildPropertyController::setUnitLabels(QString units) {
  unitText->setText(units);
  unitsLabel->setText(units);
}

void ChildPropertyController::setCurrentMesh(Mesh *mesh) {
  showSurfaceTabs(true);
  showWavefunctionTabs(false);
  showFrameworkTabs(false);
  showPlaneTabs(false);
  showPlaneInstanceTabs(false);
  m_state = DisplayState::Mesh;

  m_meshPropertyModel->setMesh(mesh);
  setEnabled(mesh != nullptr); // Enable/disable based on mesh validity
  emit meshSelectionChanged();
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
  showPlaneTabs(false);
  showPlaneInstanceTabs(false);

  m_pairInteractions = p;
  m_state = DisplayState::Framework;

  // Only enable if we have valid interactions
  bool hasValidInteractions = (p && p->getCount() > 0);
  setEnabled(hasValidInteractions);

  if (m_pairInteractions) {
    connect(m_pairInteractions, &PairInteractions::interactionAdded, this,
            &ChildPropertyController::updatePairInteractionModels);
    connect(m_pairInteractions, &PairInteractions::interactionRemoved, this,
            &ChildPropertyController::updatePairInteractionModels);
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
  if (idx < 0) {
    idx = values.indexOf("Total");
  }
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
  options.labels = frameworkLabelDisplayFromString(
      frameworkLabelDisplayComboBox->currentText());
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
  showPlaneTabs(false);
  showPlaneInstanceTabs(false);

  m_state = ChildPropertyController::DisplayState::Mesh;
  m_meshPropertyModel->setMeshInstance(mi);
  emit meshSelectionChanged();
}

void ChildPropertyController::setCurrentWavefunction(
    MolecularWavefunction *wfn) {
  showWavefunctionTabs(true);
  showSurfaceTabs(false);
  showFrameworkTabs(false);
  showPlaneTabs(false);
  showPlaneInstanceTabs(false);

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

void ChildPropertyController::onSurfaceVarTransparecyChange(
    float transparency) {
  m_meshPropertyModel->setTransparency(transparency);
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

MeshInstance *ChildPropertyController::getCurrentMeshInstance() {
  if (m_state != ChildPropertyController::DisplayState::Mesh)
    return nullptr;
  return m_meshPropertyModel->getMeshInstance();
}

ElasticTensorResults *ChildPropertyController::getCurrentElasticTensor() {
  if (m_state != ChildPropertyController::DisplayState::ElasticTensor)
    return nullptr;
  return m_currentElasticTensor;
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

void ChildPropertyController::setCurrentObject(QObject *obj) {
  if (!obj) {
    reset();
    return;
  }

  if (auto *mesh = qobject_cast<Mesh *>(obj)) {
    qDebug() << "Setting current mesh to" << mesh;
    setCurrentMesh(mesh);
  } else if (auto *meshInstance = qobject_cast<MeshInstance *>(obj)) {
    qDebug() << "Setting current mesh instance to" << meshInstance;
    setCurrentMeshInstance(meshInstance);
  } else if (auto *wfn = qobject_cast<MolecularWavefunction *>(obj)) {
    qDebug() << "Setting current wfn to" << wfn;
    setCurrentWavefunction(wfn);
  } else if (auto *interactions = qobject_cast<PairInteractions *>(obj)) {
    qDebug() << "Setting pair interactions to" << interactions;
    setCurrentPairInteractions(interactions);
  } else if (auto *plane = qobject_cast<Plane *>(obj)) {
    qDebug() << "Setting current plane to" << plane;
    setCurrentPlane(plane);
  } else if (auto *planeInstance = qobject_cast<PlaneInstance *>(obj)) {
    qDebug() << "Setting current plane instance to" << planeInstance;
    setCurrentPlaneInstance(planeInstance);
  } else if (auto *tensor = qobject_cast<ElasticTensorResults *>(obj)) {
    qDebug() << "Setting current elastic tensor to" << tensor;
    setCurrentElasticTensor(tensor);
  } else if (auto *structure = qobject_cast<ChemicalStructure *>(obj)) {
    handleStructureSelection(structure);
  }
}

void ChildPropertyController::handleStructureSelection(
    ChemicalStructure *structure) {
  // Set pair interactions
  setCurrentPairInteractions(structure->pairInteractions());

  // Find and set first mesh if available
  for (auto *child : structure->children()) {
    if (auto *mesh = qobject_cast<Mesh *>(child)) {
      setCurrentMesh(mesh);
      break;
    }
  }
}

void ChildPropertyController::setCurrentPlane(Plane *plane) {
  showSurfaceTabs(false);
  showWavefunctionTabs(false);
  showFrameworkTabs(false);
  showPlaneInstanceTabs(false);  // Hide instance tabs since this is a plane
  showPlaneTabs(true);
  m_state = DisplayState::Plane;

  setEnabled(plane != nullptr);
  
  // Widget is created when tab is shown, so check if it exists
  if (m_planeInfoWidget) {
    m_planeInfoWidget->setPlane(plane);
  }
  
  if (plane) {
    qDebug() << "Selected plane:" << plane->name();
    qDebug() << "  Color:" << plane->color();
    qDebug() << "  Visible:" << plane->isVisible();
    qDebug() << "  Show Grid:" << plane->showGrid();
    qDebug() << "  Show Axes:" << plane->showAxes();
    qDebug() << "  Show Bounds:" << plane->showBounds();
  }
}

void ChildPropertyController::setCurrentPlaneInstance(PlaneInstance *instance) {
  showSurfaceTabs(false);
  showWavefunctionTabs(false);
  showFrameworkTabs(false);
  showPlaneTabs(false);  // Hide plane tabs since this is an instance
  showPlaneInstanceTabs(true);
  m_state = DisplayState::PlaneInstance;

  setEnabled(instance != nullptr);
  
  // Widget is created when tab is shown, so check if it exists
  if (m_planeInstanceWidget) {
    m_planeInstanceWidget->setPlaneInstance(instance);
  }
  
  if (instance) {
    qDebug() << "Selected plane instance:" << instance->name();
    qDebug() << "  Offset:" << instance->offset();
    qDebug() << "  Visible:" << instance->isVisible();
    if (instance->plane()) {
      qDebug() << "  Parent plane:" << instance->plane()->name();
    }
  }
}

void ChildPropertyController::setCurrentElasticTensor(ElasticTensorResults *tensor) {
  showSurfaceTabs(false);
  showWavefunctionTabs(false);
  showFrameworkTabs(false);
  showPlaneTabs(false);
  showPlaneInstanceTabs(false);
  showElasticTensorTabs(true);
  m_state = DisplayState::ElasticTensor;

  setEnabled(tensor != nullptr);
  m_currentElasticTensor = tensor;
  
  if (tensor) {
    qDebug() << "Selected elastic tensor:" << tensor->name();
    qDebug() << "  Average Young's Modulus:" << tensor->averageYoungsModulus() << "GPa";
    qDebug() << "  Average Shear Modulus:" << tensor->averageShearModulus() << "GPa";
    qDebug() << "  Stable:" << (tensor->isStable() ? "Yes" : "No");
    
    updateElasticTensorInfo(tensor);
  }
  
  // Emit signal for info dialog
  emit elasticTensorSelectionChanged();
}

void ChildPropertyController::createPlanePropertiesTab() {
  m_planePropertiesTab = new QWidget;
  
  auto *layout = new QVBoxLayout(m_planePropertiesTab);
  
  m_planeInfoWidget = new PlaneInfoWidget(m_planePropertiesTab);
  layout->addWidget(m_planeInfoWidget);
  
  // Connect the generate slab signal
  connect(m_planeInfoWidget, &PlaneInfoWidget::generateSlabRequested,
          this, &ChildPropertyController::onGenerateSlabRequested);
  
  layout->addStretch();
  
  // Ensure the widget geometry is properly calculated before adding to tabs
  m_planePropertiesTab->updateGeometry();
  m_planeInfoWidget->updateGeometry();
}

void ChildPropertyController::createPlaneInstancePropertiesTab() {
  m_planeInstancePropertiesTab = new QWidget;
  
  auto *layout = new QVBoxLayout(m_planeInstancePropertiesTab);
  
  m_planeInstanceWidget = new PlaneInstanceWidget(m_planeInstancePropertiesTab);
  layout->addWidget(m_planeInstanceWidget);
  
  layout->addStretch();
}

void ChildPropertyController::updatePlaneInfo(Plane *plane, PlaneInstance *instance) {
  if (m_planeInfoWidget) {
    m_planeInfoWidget->setPlane(plane);
  }
}

void ChildPropertyController::createElasticTensorPropertiesTab() {
  m_elasticTensorPropertiesTab = new QWidget;
  
  auto *layout = new QVBoxLayout(m_elasticTensorPropertiesTab);
  
  // Title
  auto *titleLabel = new QLabel("Elastic Tensor Properties");
  titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; margin-bottom: 10px;");
  layout->addWidget(titleLabel);
  
  // Properties display (will be populated in updateElasticTensorInfo)
  auto *propsLayout = new QFormLayout;
  
  auto *nameLabel = new QLabel("--");
  nameLabel->setObjectName("tensorNameLabel");
  propsLayout->addRow("Name:", nameLabel);
  
  auto *stableLabel = new QLabel("--");
  stableLabel->setObjectName("tensorStableLabel");  
  propsLayout->addRow("Stability:", stableLabel);
  
  auto *bulkLabel = new QLabel("--");
  bulkLabel->setObjectName("tensorBulkLabel");
  propsLayout->addRow("Bulk Modulus:", bulkLabel);
  
  auto *shearLabel = new QLabel("--");
  shearLabel->setObjectName("tensorShearLabel");
  propsLayout->addRow("Shear Modulus:", shearLabel);
  
  auto *youngLabel = new QLabel("--");
  youngLabel->setObjectName("tensorYoungLabel");
  propsLayout->addRow("Young's Modulus:", youngLabel);
  
  auto *poissonLabel = new QLabel("--");
  poissonLabel->setObjectName("tensorPoissonLabel");
  propsLayout->addRow("Poisson Ratio:", poissonLabel);
  
  layout->addLayout(propsLayout);
  
  // Mesh generation section
  layout->addWidget(new QLabel("Mesh Generation"));
  
  auto *meshLayout = new QFormLayout;
  
  auto *propertyCombo = new QComboBox;
  propertyCombo->setObjectName("tensorPropertyCombo");
  propertyCombo->addItems({"Young's Modulus", 
                           "Shear Modulus (Max)", 
                           "Shear Modulus (Min)", 
                           "Linear Compressibility", 
                           "Poisson's Ratio (Max)",
                           "Poisson's Ratio (Min)"});
  meshLayout->addRow("Property:", propertyCombo);
  
  auto *subdivisionsSpinBox = new QSpinBox;
  subdivisionsSpinBox->setObjectName("tensorSubdivisionsSpinBox");
  subdivisionsSpinBox->setRange(0, 7);
  subdivisionsSpinBox->setValue(5);
  meshLayout->addRow("Subdivisions:", subdivisionsSpinBox);
  
  auto *radiusSpinBox = new QDoubleSpinBox;
  radiusSpinBox->setObjectName("tensorRadiusSpinBox");
  radiusSpinBox->setRange(0.1, 100.0);
  radiusSpinBox->setValue(10.0);
  radiusSpinBox->setSuffix(" Å");
  meshLayout->addRow("Radius:", radiusSpinBox);
  
  // Add center point controls
  auto *centerWidget = new QWidget;
  auto *centerLayout = new QHBoxLayout(centerWidget);
  centerLayout->setContentsMargins(0, 0, 0, 0);
  
  auto *centerXSpinBox = new QDoubleSpinBox;
  centerXSpinBox->setObjectName("tensorCenterX");
  centerXSpinBox->setRange(-100.0, 100.0);
  centerXSpinBox->setValue(0.0);
  centerXSpinBox->setSingleStep(0.1);
  centerXSpinBox->setPrefix("X: ");
  centerLayout->addWidget(centerXSpinBox);
  
  auto *centerYSpinBox = new QDoubleSpinBox;
  centerYSpinBox->setObjectName("tensorCenterY");
  centerYSpinBox->setRange(-100.0, 100.0);
  centerYSpinBox->setValue(0.0);
  centerYSpinBox->setSingleStep(0.1);
  centerYSpinBox->setPrefix("Y: ");
  centerLayout->addWidget(centerYSpinBox);
  
  auto *centerZSpinBox = new QDoubleSpinBox;
  centerZSpinBox->setObjectName("tensorCenterZ");
  centerZSpinBox->setRange(-100.0, 100.0);
  centerZSpinBox->setValue(0.0);
  centerZSpinBox->setSingleStep(0.1);
  centerZSpinBox->setPrefix("Z: ");
  centerLayout->addWidget(centerZSpinBox);
  
  meshLayout->addRow("Center:", centerWidget);
  
  // Add button to center on selected atoms
  auto *centerOnSelectionButton = new QPushButton("Center on Selection");
  centerOnSelectionButton->setObjectName("tensorCenterOnSelection");
  connect(centerOnSelectionButton, &QPushButton::clicked, this, [this, centerXSpinBox, centerYSpinBox, centerZSpinBox]() {
    if (!m_currentElasticTensor || !m_currentElasticTensor->parent()) {
      qDebug() << "No elastic tensor or parent structure";
      return;
    }
    
    auto *structure = qobject_cast<ChemicalStructure*>(m_currentElasticTensor->parent());
    if (!structure) {
      qDebug() << "Parent is not a ChemicalStructure";
      return;
    }
    
    // Get selected atoms
    auto selectedAtoms = structure->atomsWithFlags(AtomFlag::Selected);
    if (selectedAtoms.empty()) {
      qDebug() << "No atoms selected";
      return;
    }
    
    // Calculate center of mass
    auto positions = structure->atomicPositionsForIndices(selectedAtoms);
    auto atomicNumbers = structure->atomicNumbersForIndices(selectedAtoms);
    
    double totalMass = 0.0;
    Eigen::Vector3d centerOfMass = Eigen::Vector3d::Zero();
    
    for (int i = 0; i < selectedAtoms.size(); ++i) {
      double mass = occ::core::Element(atomicNumbers(i)).mass();
      totalMass += mass;
      centerOfMass += mass * positions.col(i);
    }
    
    if (totalMass > 0.0) {
      centerOfMass /= totalMass;
      centerXSpinBox->setValue(centerOfMass.x());
      centerYSpinBox->setValue(centerOfMass.y());
      centerZSpinBox->setValue(centerOfMass.z());
      qDebug() << "Centered on selection center of mass:" << centerOfMass.x() << centerOfMass.y() << centerOfMass.z();
    }
  });
  meshLayout->addRow("", centerOnSelectionButton);
  
  auto *generateButton = new QPushButton("Generate Mesh");
  generateButton->setObjectName("tensorGenerateButton");
  
  // Connect the generate button
  connect(generateButton, &QPushButton::clicked, this, [this]() {
    if (!m_currentElasticTensor) {
      qDebug() << "No current elastic tensor for mesh generation";
      return;
    }
    
    auto *combo = m_elasticTensorPropertiesTab->findChild<QComboBox*>("tensorPropertyCombo");
    auto *subdivisions = m_elasticTensorPropertiesTab->findChild<QSpinBox*>("tensorSubdivisionsSpinBox");
    auto *radius = m_elasticTensorPropertiesTab->findChild<QDoubleSpinBox*>("tensorRadiusSpinBox");
    auto *centerX = m_elasticTensorPropertiesTab->findChild<QDoubleSpinBox*>("tensorCenterX");
    auto *centerY = m_elasticTensorPropertiesTab->findChild<QDoubleSpinBox*>("tensorCenterY");
    auto *centerZ = m_elasticTensorPropertiesTab->findChild<QDoubleSpinBox*>("tensorCenterZ");
    
    if (!combo || !subdivisions || !radius || !centerX || !centerY || !centerZ) {
      qDebug() << "Missing UI controls for mesh generation";
      return;
    }
    
    // Validate parent structure exists
    if (!m_currentElasticTensor->parent()) {
      qDebug() << "Elastic tensor has no parent structure";
      return;
    }
    
    ElasticTensorResults::PropertyType propType;
    switch (combo->currentIndex()) {
    case 0: propType = ElasticTensorResults::PropertyType::YoungsModulus; break;
    case 1: propType = ElasticTensorResults::PropertyType::ShearModulusMax; break;
    case 2: propType = ElasticTensorResults::PropertyType::ShearModulusMin; break;
    case 3: propType = ElasticTensorResults::PropertyType::LinearCompressibility; break;
    case 4: propType = ElasticTensorResults::PropertyType::PoissonRatioMax; break;
    case 5: propType = ElasticTensorResults::PropertyType::PoissonRatioMin; break;
    default: 
      qDebug() << "Invalid property type index:" << combo->currentIndex();
      propType = ElasticTensorResults::PropertyType::YoungsModulus;
    }
    
    try {
      // Get the desired center offset
      Eigen::Vector3d centerOffset(centerX->value(), centerY->value(), centerZ->value());

      Mesh* mesh = m_currentElasticTensor->createPropertyMesh(propType, subdivisions->value(), radius->value(), centerOffset);
      if (mesh) {
        // Set the mesh's parent to the same as the elastic tensor (the ChemicalStructure)
        mesh->setParent(m_currentElasticTensor->parent());

        // Create MeshInstance with identity transform since offset is already in base mesh
        MeshInstance *instance = new MeshInstance(mesh, MeshTransform::Identity());
        instance->setObjectName(QString("%1 - %2").arg(combo->currentText(), m_currentElasticTensor->name()));
        
        qDebug() << "Successfully generated mesh for" << combo->currentText() << "from elastic tensor" << m_currentElasticTensor->name();
        qDebug() << "Mesh has" << mesh->numberOfVertices() << "vertices and" << mesh->numberOfFaces() << "faces";
        qDebug() << "Created MeshInstance for visibility";
      } else {
        qDebug() << "Failed to create mesh - createPropertyMesh returned nullptr";
      }
    } catch (const std::exception& e) {
      qDebug() << "Exception during mesh generation:" << e.what();
    }
  });
  
  meshLayout->addRow("", generateButton);
  layout->addLayout(meshLayout);
  
  layout->addStretch();
}

void ChildPropertyController::updateElasticTensorInfo(ElasticTensorResults *tensor) {
  if (!m_elasticTensorPropertiesTab || !tensor) return;
  
  auto *nameLabel = m_elasticTensorPropertiesTab->findChild<QLabel*>("tensorNameLabel");
  auto *stableLabel = m_elasticTensorPropertiesTab->findChild<QLabel*>("tensorStableLabel");
  auto *bulkLabel = m_elasticTensorPropertiesTab->findChild<QLabel*>("tensorBulkLabel");
  auto *shearLabel = m_elasticTensorPropertiesTab->findChild<QLabel*>("tensorShearLabel");
  auto *youngLabel = m_elasticTensorPropertiesTab->findChild<QLabel*>("tensorYoungLabel");
  auto *poissonLabel = m_elasticTensorPropertiesTab->findChild<QLabel*>("tensorPoissonLabel");
  
  if (nameLabel) nameLabel->setText(tensor->name());
  if (stableLabel) {
    bool stable = tensor->isStable();
    stableLabel->setText(stable ? "Stable" : "Unstable");
    stableLabel->setStyleSheet(stable ? "color: green;" : "color: red;");
  }
  if (bulkLabel) bulkLabel->setText(QString("%1 GPa").arg(tensor->averageBulkModulus(), 0, 'f', 2));
  if (shearLabel) shearLabel->setText(QString("%1 GPa").arg(tensor->averageShearModulus(), 0, 'f', 2));
  if (youngLabel) youngLabel->setText(QString("%1 GPa").arg(tensor->averageYoungsModulus(), 0, 'f', 2));
  if (poissonLabel) poissonLabel->setText(QString("%1").arg(tensor->averagePoissonRatio(), 0, 'f', 3));
}

void ChildPropertyController::onGenerateSlabRequested(int h, int k, int l, double offset) {
  // Simply forward the signal to higher-level components
  emit generateSlabRequested(h, k, l, offset);
}
