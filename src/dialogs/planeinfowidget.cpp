#include "planeinfowidget.h"
#include "planeinstance.h"
#include "crystalplane_unified.h"
#include <QTimer>

PlaneInfoWidget::PlaneInfoWidget(QWidget *parent) : QWidget(parent) {
  setupUi(this);
  
  // Ensure all widgets are properly initialized before proceeding
  if (!nameEdit || !colorButton || !originXSpinBox || !normalXSpinBox) {
    qWarning() << "PlaneInfoWidget: Critical UI elements not initialized properly";
    return;
  }
  
  // Connect UI signals once during construction
  connectUISignals();
  
  // Start with widget disabled since no plane is set initially
  setEnabled(false);
}

void PlaneInfoWidget::setPlane(Plane *plane) {
  if (_plane == plane)
    return;
  
  // Disconnect from previous plane
  if (_plane) {
    disconnect(_plane, nullptr, this, nullptr);
  }
  
  _plane = plane;

  if (_plane) {
    // Connect to plane's model change notifications
    connect(_plane, &Plane::settingsChanged, this, &PlaneInfoWidget::updateFromPlane);
    
    updateUIMode(); // Show/hide appropriate sections
    
    // Initialize UI from plane state
    updateFromPlane();
  } else {
    clearUI();
  }
}

void PlaneInfoWidget::connectUISignals() {
  // Connect UI elements directly to controller methods
  connect(nameEdit, &QLineEdit::textChanged, this, &PlaneInfoWidget::onNameChanged);
  connect(colorButton, &QPushButton::clicked, this, &PlaneInfoWidget::onColorButtonClicked);

  connect(originXSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &PlaneInfoWidget::onOriginChanged);
  connect(originYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &PlaneInfoWidget::onOriginChanged);
  connect(originZSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &PlaneInfoWidget::onOriginChanged);

  connect(normalXSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &PlaneInfoWidget::onNormalChanged);
  connect(normalYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &PlaneInfoWidget::onNormalChanged);
  connect(normalZSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &PlaneInfoWidget::onNormalChanged);

  connect(showGridCheckBox, &QCheckBox::toggled, this, &PlaneInfoWidget::onGridPropertiesChanged);
  connect(gridSpacingSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &PlaneInfoWidget::onGridPropertiesChanged);

  connect(boundsAMinSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &PlaneInfoWidget::onBoundsChanged);
  connect(boundsAMaxSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &PlaneInfoWidget::onBoundsChanged);
  connect(boundsBMinSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &PlaneInfoWidget::onBoundsChanged);
  connect(boundsBMaxSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &PlaneInfoWidget::onBoundsChanged);

  connect(createInstanceButton, &QPushButton::clicked, this,
          &PlaneInfoWidget::onCreateInstanceClicked);

  connect(millerHSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
          this, &PlaneInfoWidget::onMillerIndicesChanged);
  connect(millerKSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
          this, &PlaneInfoWidget::onMillerIndicesChanged);
  connect(millerLSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
          this, &PlaneInfoWidget::onMillerIndicesChanged);

  connect(generateSlabButton, &QPushButton::clicked, this,
          &PlaneInfoWidget::onGenerateSlabClicked);
}


void PlaneInfoWidget::updateFromPlane() {
  if (!_plane)
    return;

  // Enable the widget since we have a valid plane
  setEnabled(true);

  // Set flag to prevent updatePlaneFromUI from firing
  _updatingFromPlane = true;

  // Block signals to prevent loops
  nameEdit->blockSignals(true);
  originXSpinBox->blockSignals(true);
  originYSpinBox->blockSignals(true);
  originZSpinBox->blockSignals(true);
  normalXSpinBox->blockSignals(true);
  normalYSpinBox->blockSignals(true);
  normalZSpinBox->blockSignals(true);
  showGridCheckBox->blockSignals(true);
  gridSpacingSpinBox->blockSignals(true);
  boundsAMinSpinBox->blockSignals(true);
  boundsAMaxSpinBox->blockSignals(true);
  boundsBMinSpinBox->blockSignals(true);
  boundsBMaxSpinBox->blockSignals(true);

  // Update values
  nameEdit->setText(_plane->name());
  updateColorButton();

  QVector3D origin = _plane->origin();
  originXSpinBox->setValue(origin.x());
  originYSpinBox->setValue(origin.y());
  originZSpinBox->setValue(origin.z());

  QVector3D normal = _plane->normal();
  normalXSpinBox->setValue(normal.x());
  normalYSpinBox->setValue(normal.y());
  normalZSpinBox->setValue(normal.z());

  showGridCheckBox->setChecked(_plane->showGrid());
  gridSpacingSpinBox->setValue(_plane->gridSpacing());

  QVector2D boundsA = _plane->boundsA();
  QVector2D boundsB = _plane->boundsB();
  boundsAMinSpinBox->setValue(boundsA.x());
  boundsAMaxSpinBox->setValue(boundsA.y());
  boundsBMinSpinBox->setValue(boundsB.x());
  boundsBMaxSpinBox->setValue(boundsB.y());

  // Unblock signals
  nameEdit->blockSignals(false);
  originXSpinBox->blockSignals(false);
  originYSpinBox->blockSignals(false);
  originZSpinBox->blockSignals(false);
  normalXSpinBox->blockSignals(false);
  normalYSpinBox->blockSignals(false);
  normalZSpinBox->blockSignals(false);
  showGridCheckBox->blockSignals(false);
  gridSpacingSpinBox->blockSignals(false);
  boundsAMinSpinBox->blockSignals(false);
  boundsAMaxSpinBox->blockSignals(false);
  boundsBMinSpinBox->blockSignals(false);
  boundsBMaxSpinBox->blockSignals(false);
  
  // Update crystal-specific properties if this is a CrystalPlane
  if (isCrystalPlane()) {
    updateCrystalProperties();
  }
  
  // Reset flag
  _updatingFromPlane = false;
}

bool PlaneInfoWidget::isCrystalPlane() const {
  return qobject_cast<CrystalPlaneUnified*>(_plane) != nullptr;
}

CrystalPlaneUnified* PlaneInfoWidget::crystalPlane() const {
  return qobject_cast<CrystalPlaneUnified*>(_plane);
}

void PlaneInfoWidget::updateUIMode() {
  bool isCrystal = isCrystalPlane();
  
  // Show/hide appropriate sections
  millerGroup->setVisible(isCrystal);
  slabGroup->setVisible(isCrystal);
  
  // Update offset and grid units based on plane type
  updateOffsetUnits();
  updateGridUnits();
  
  // Adjust bounds labels for crystal vs Cartesian
  if (isCrystal) {
    boundsGroup->setTitle("Crystal Bounds");
    boundsALabel->setText("A repeats:");
    boundsBLabel->setText("B repeats:");
    boundsALabel->setToolTip("Number of unit cell repetitions along A axis");
    boundsBLabel->setToolTip("Number of unit cell repetitions along B axis");
  } else {
    boundsGroup->setTitle("Bounds");
    boundsALabel->setText("A:");
    boundsBLabel->setText("B:");
    boundsALabel->setToolTip("Minimum and maximum A bounds");
    boundsBLabel->setToolTip("Minimum and maximum B bounds");
  }
}

void PlaneInfoWidget::updateCrystalProperties() {
  auto *cp = crystalPlane();
  if (!cp) return;
  
  // Update Miller indices display
  millerHSpinBox->blockSignals(true);
  millerKSpinBox->blockSignals(true);
  millerLSpinBox->blockSignals(true);
  
  millerHSpinBox->setValue(cp->millerH());
  millerKSpinBox->setValue(cp->millerK());
  millerLSpinBox->setValue(cp->millerL());
  
  millerHSpinBox->blockSignals(false);
  millerKSpinBox->blockSignals(false);
  millerLSpinBox->blockSignals(false);
  
  // Update d-spacing
  double spacing = cp->interplanarSpacing();
  spacingValueLabel->setText(QString("%1 Å").arg(spacing, 0, 'f', 3));
}

void PlaneInfoWidget::updateColorButton() {
  if (!_plane)
    return;

  QColor color = _plane->color();
  QString styleSheet = QString("background-color: %1; border: 1px solid #888;")
                           .arg(color.name());
  colorButton->setStyleSheet(styleSheet);
}

// Controller methods that directly update the model (Plane)
void PlaneInfoWidget::onNameChanged() {
  if (!_plane || _updatingFromPlane)
    return;
  
  PlaneSettings settings = _plane->settings();
  settings.name = nameEdit->text();
  _plane->updateSettings(settings);
}

void PlaneInfoWidget::onColorButtonClicked() {
  if (!_plane || _updatingFromPlane)
    return;

  QColor currentColor = _plane->color();
  QColor newColor = QColorDialog::getColor(currentColor, this, "Select Plane Color");

  if (newColor.isValid()) {
    PlaneSettings settings = _plane->settings();
    settings.color = newColor;
    _plane->updateSettings(settings);
  }
}

void PlaneInfoWidget::onOriginChanged() {
  if (!_plane || _updatingFromPlane)
    return;

  PlaneSettings settings = _plane->settings();
  settings.origin = QVector3D(originXSpinBox->value(), originYSpinBox->value(), originZSpinBox->value());
  _plane->updateSettings(settings);
}

void PlaneInfoWidget::onNormalChanged() {
  if (!_plane || _updatingFromPlane)
    return;

  PlaneSettings settings = _plane->settings();
  QVector3D normal(normalXSpinBox->value(), normalYSpinBox->value(), normalZSpinBox->value());
  
  // Only normalize for Cartesian planes, not crystal planes
  if (!isCrystalPlane()) {
    normal = normal.normalized();
  }
  
  settings.normal = normal;
  _plane->updateSettings(settings);
}

void PlaneInfoWidget::onGridPropertiesChanged() {
  if (!_plane || _updatingFromPlane)
    return;

  PlaneSettings settings = _plane->settings();
  settings.showGrid = showGridCheckBox->isChecked();
  settings.gridSpacing = gridSpacingSpinBox->value();
  _plane->updateSettings(settings);
}

void PlaneInfoWidget::onBoundsChanged() {
  if (!_plane || _updatingFromPlane)
    return;

  PlaneSettings settings = _plane->settings();
  settings.boundsA = QVector2D(boundsAMinSpinBox->value(), boundsAMaxSpinBox->value());
  settings.boundsB = QVector2D(boundsBMinSpinBox->value(), boundsBMaxSpinBox->value());
  _plane->updateSettings(settings);
}

void PlaneInfoWidget::onMillerIndicesChanged() {
  if (!_plane || _updatingFromPlane)
    return;
  
  auto *cp = crystalPlane();
  if (!cp) return;
  
  // Update the CrystalPlane Miller indices
  cp->setMillerIndices(millerHSpinBox->value(), 
                       millerKSpinBox->value(), 
                       millerLSpinBox->value());
  
  // Update d-spacing display and offset units since d-spacing may have changed
  updateCrystalProperties();
  updateOffsetUnits();
  updateGridUnits();
}

void PlaneInfoWidget::onCreateInstanceClicked() {
  if (!_plane) {
    qWarning() << "No plane available to create instance";
    return;
  }

  double offsetValue = newOffsetSpinBox->value();

  // Create the instance directly on the plane using the offset value
  // For crystal planes, offset is in d-spacing units and the normal vector has appropriate length
  // For Cartesian planes, offset is in Angstrom units and normal is unit length
  auto *instance = _plane->createInstance(offsetValue);
  qDebug() << "Created plane instance:" << instance->name()
           << "with offset:" << offsetValue << _plane->offsetUnit();

  // Defer signal emission to avoid potential event loop issues
  QTimer::singleShot(0, this, [this, offsetValue]() {
    emit createInstanceRequested(offsetValue);
  });
}

void PlaneInfoWidget::clearUI() {
  // Safety check - make sure UI is fully constructed
  if (!nameEdit) {
    return;
  }
  
  // Block signals to prevent unwanted emissions
  nameEdit->blockSignals(true);
  originXSpinBox->blockSignals(true);
  originYSpinBox->blockSignals(true);
  originZSpinBox->blockSignals(true);
  normalXSpinBox->blockSignals(true);
  normalYSpinBox->blockSignals(true);
  normalZSpinBox->blockSignals(true);
  showGridCheckBox->blockSignals(true);
  gridSpacingSpinBox->blockSignals(true);
  boundsAMinSpinBox->blockSignals(true);
  boundsAMaxSpinBox->blockSignals(true);
  boundsBMinSpinBox->blockSignals(true);
  boundsBMaxSpinBox->blockSignals(true);
  
  // Clear all values to defaults
  nameEdit->setText("");
  colorButton->setStyleSheet("background-color: gray; border: 1px solid #888;");
  
  originXSpinBox->setValue(0.0);
  originYSpinBox->setValue(0.0);
  originZSpinBox->setValue(0.0);
  
  normalXSpinBox->setValue(0.0);
  normalYSpinBox->setValue(0.0);
  normalZSpinBox->setValue(1.0);  // Default to Z axis
  
  showGridCheckBox->setChecked(false);
  
  // Set default grid spacing based on plane type
  if (isCrystalPlane()) {
    gridSpacingSpinBox->setValue(0.1); // 0.1 unit cells
  } else {
    gridSpacingSpinBox->setValue(1.0); // 1 Angstrom
  }
  
  // Set default bounds based on plane type
  if (isCrystalPlane()) {
    // Crystal planes use unit cell repetitions (0-1)
    boundsAMinSpinBox->setValue(0.0);
    boundsAMaxSpinBox->setValue(1.0);
    boundsBMinSpinBox->setValue(0.0);
    boundsBMaxSpinBox->setValue(1.0);
  } else {
    // Cartesian planes use Angstrom bounds (-10 to 10)
    boundsAMinSpinBox->setValue(-10.0);
    boundsAMaxSpinBox->setValue(10.0);
    boundsBMinSpinBox->setValue(-10.0);
    boundsBMaxSpinBox->setValue(10.0);
  }
  
  newOffsetSpinBox->setValue(0.0);
  
  // Disable all controls when no plane is selected
  setEnabled(false);
  
  // Unblock signals
  nameEdit->blockSignals(false);
  originXSpinBox->blockSignals(false);
  originYSpinBox->blockSignals(false);
  originZSpinBox->blockSignals(false);
  normalXSpinBox->blockSignals(false);
  normalYSpinBox->blockSignals(false);
  normalZSpinBox->blockSignals(false);
  showGridCheckBox->blockSignals(false);
  gridSpacingSpinBox->blockSignals(false);
  boundsAMinSpinBox->blockSignals(false);
  boundsAMaxSpinBox->blockSignals(false);
  boundsBMinSpinBox->blockSignals(false);
  boundsBMaxSpinBox->blockSignals(false);
}

void PlaneInfoWidget::updateOffsetUnits() {
  if (_plane) {
    QString unit = _plane->offsetUnit();
    newOffsetSpinBox->setSuffix(unit);
    
    if (unit == "d") {
      newOffsetSpinBox->setToolTip("Offset in units of d-spacing (interplanar spacing)");
    } else {
      newOffsetSpinBox->setToolTip("Distance from the main plane in Angstroms");
    }
  }
}

void PlaneInfoWidget::updateGridUnits() {
  if (_plane) {
    QString unit = _plane->gridUnit();
    gridSpacingSpinBox->setSuffix(unit);
    
    if (unit == "uc") {
      gridSpacingSpinBox->setToolTip("Grid spacing in units of crystal basis vectors");
      // Set appropriate default for crystal planes
      if (qFuzzyCompare(gridSpacingSpinBox->value(), 1.0)) {
        gridSpacingSpinBox->setValue(0.1); // 0.1 unit cells = reasonable grid density
      }
    } else {
      gridSpacingSpinBox->setToolTip("Grid spacing in Angstroms");
      // Set appropriate default for Cartesian planes
      if (qFuzzyCompare(gridSpacingSpinBox->value(), 0.1)) {
        gridSpacingSpinBox->setValue(1.0); // 1 Angstrom = reasonable grid density
      }
    }
  }
}

void PlaneInfoWidget::onGenerateSlabClicked() {
  auto *cp = crystalPlane();
  if (!cp) {
    qWarning() << "Generate slab clicked but no crystal plane available";
    return;
  }
  
  // Get current Miller indices and calculate offset from origin
  int h = cp->millerH();
  int k = cp->millerK();
  int l = cp->millerL();
  
  // Use the current plane's offset (distance from zero plane)
  // For crystal planes, the normal length represents d-spacing scaling
  double offset = 0.0; // Could be enhanced to calculate actual offset from origin
  
  emit generateSlabRequested(h, k, l, offset);
}
