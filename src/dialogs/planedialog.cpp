#include "planedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QLabel>
#include <QComboBox>
#include "crystalstructure.h"

PlaneDialog::PlaneDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Select Plane");
    setModal(true);
    setMinimumSize(400, 300);
    
    setupCartesianPresets();
    setupCrystalPresets();
    setupUI();
    
    // Start with Cartesian planes by default
    _planeTypeCombo->setCurrentIndex(0);
    updatePresetList();
}

void PlaneDialog::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    
    // Plane type selection
    auto *typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Plane Type:"));
    _planeTypeCombo = new QComboBox(this);
    _planeTypeCombo->addItem("Cartesian Planes", 0);
    _planeTypeCombo->addItem("Crystal Planes (Miller Indices)", 1);
    connect(_planeTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PlaneDialog::onPlaneTypeChanged);
    typeLayout->addWidget(_planeTypeCombo);
    typeLayout->addStretch();
    mainLayout->addLayout(typeLayout);
    
    // Preset selection list
    mainLayout->addWidget(new QLabel("Select a plane preset:"));
    _presetListWidget = new QListWidget(this);
    connect(_presetListWidget, &QListWidget::itemSelectionChanged,
            this, &PlaneDialog::onPresetSelectionChanged);
    mainLayout->addWidget(_presetListWidget);
    
    // Description label
    _descriptionLabel = new QLabel("Select a preset to see its description.", this);
    _descriptionLabel->setWordWrap(true);
    _descriptionLabel->setStyleSheet("QLabel { border: 1px solid gray; padding: 8px; background-color: #f0f0f0; }");
    _descriptionLabel->setMinimumHeight(60);
    mainLayout->addWidget(_descriptionLabel);
    
    // Dialog buttons
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false); // Enable only when selection is made
    connect(_presetListWidget, &QListWidget::itemSelectionChanged,
            [buttonBox, this]() {
                buttonBox->button(QDialogButtonBox::Ok)->setEnabled(_presetListWidget->currentRow() >= 0);
            });
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void PlaneDialog::setupCartesianPresets() {
    _cartesianPresets = {
        {"XY Plane (Z normal)", "Plane with normal pointing in +Z direction, placed at origin", false, QVector3D(0,0,1), QVector3D(0,0,0), 0,0,0},
        {"XZ Plane (Y normal)", "Plane with normal pointing in +Y direction, placed at origin", false, QVector3D(0,1,0), QVector3D(0,0,0), 0,0,0},
        {"YZ Plane (X normal)", "Plane with normal pointing in +X direction, placed at origin", false, QVector3D(1,0,0), QVector3D(0,0,0), 0,0,0},
        {"Diagonal XY-Z", "Plane with normal pointing diagonally (1,1,1)", false, QVector3D(1,1,1).normalized(), QVector3D(0,0,0), 0,0,0},
        {"Custom Cartesian", "Define your own normal vector and origin position", false, QVector3D(0,0,1), QVector3D(0,0,0), 0,0,0}
    };
}

void PlaneDialog::setupCrystalPresets() {
    _crystalPresets = {
        {"(100) - a-axis normal", "Plane perpendicular to the a-axis (most common)", true, QVector3D(), QVector3D(), 1,0,0},
        {"(010) - b-axis normal", "Plane perpendicular to the b-axis", true, QVector3D(), QVector3D(), 0,1,0},
        {"(001) - c-axis normal", "Plane perpendicular to the c-axis", true, QVector3D(), QVector3D(), 0,0,1},
        {"(110) - diagonal a,b", "Plane diagonal to a and b axes", true, QVector3D(), QVector3D(), 1,1,0},
        {"(101) - diagonal a,c", "Plane diagonal to a and c axes", true, QVector3D(), QVector3D(), 1,0,1},
        {"(011) - diagonal b,c", "Plane diagonal to b and c axes", true, QVector3D(), QVector3D(), 0,1,1},
        {"(111) - cubic diagonal", "Plane diagonal to all three axes (common in cubic systems)", true, QVector3D(), QVector3D(), 1,1,1},
        {"(200) - a-axis half-period", "Plane parallel to (100) with half the d-spacing", true, QVector3D(), QVector3D(), 2,0,0},
        {"(220) - high-index diagonal", "Higher index diagonal plane", true, QVector3D(), QVector3D(), 2,2,0},
        {"Custom Miller Indices", "Define your own (h k l) Miller indices", true, QVector3D(), QVector3D(), 1,0,0}
    };
}

void PlaneDialog::updatePresetList() {
    _presetListWidget->clear();
    
    const auto &presets = (_planeTypeCombo->currentData().toInt() == 1) ? _crystalPresets : _cartesianPresets;
    
    for (const auto &preset : presets) {
        _presetListWidget->addItem(preset.name);
    }
    
    updateDescription();
}

void PlaneDialog::updateDescription() {
    int currentRow = _presetListWidget->currentRow();
    if (currentRow < 0) {
        _descriptionLabel->setText("Select a preset to see its description.");
        return;
    }
    
    const auto &presets = (_planeTypeCombo->currentData().toInt() == 1) ? _crystalPresets : _cartesianPresets;
    if (currentRow < static_cast<int>(presets.size())) {
        _descriptionLabel->setText(presets[currentRow].description);
    }
}

void PlaneDialog::onPlaneTypeChanged() {
    updatePresetList();
}

void PlaneDialog::onPresetSelectionChanged() {
    updateDescription();
}

PlanePreset PlaneDialog::selectedPreset() const {
    int currentRow = _presetListWidget->currentRow();
    if (currentRow < 0) {
        return {}; // Empty preset
    }
    
    const auto &presets = (_planeTypeCombo->currentData().toInt() == 1) ? _crystalPresets : _cartesianPresets;
    if (currentRow < static_cast<int>(presets.size())) {
        return presets[currentRow];
    }
    
    return {}; // Empty preset
}

Plane* PlaneDialog::createPlane(QObject *parent) const {
    PlanePreset preset = selectedPreset();
    if (preset.name.isEmpty()) {
        return nullptr;
    }
    
    Plane *plane = nullptr;
    
    if (preset.isCrystal) {
        // Create crystal plane - but only if parent is a CrystalStructure
        auto *crystal = qobject_cast<CrystalStructure*>(parent);
        if (crystal) {
            plane = new CrystalPlaneUnified(preset.h, preset.k, preset.l, crystal);
        } else {
            // Fallback: create regular plane with default orientation
            plane = new Plane(QString("(%1%2%3)").arg(preset.h).arg(preset.k).arg(preset.l), parent);
            PlaneSettings settings = plane->settings();
            settings.normal = QVector3D(0, 0, 1); // Default normal
            settings.origin = QVector3D(0, 0, 0);
            plane->updateSettings(settings);
        }
    } else {
        // Create Cartesian plane
        plane = new Plane(preset.name, parent);
        PlaneSettings settings = plane->settings();
        settings.normal = preset.normal.normalized();
        settings.origin = preset.origin;
        plane->updateSettings(settings);
    }
    
    if (plane) {
        PlaneSettings settings = plane->settings();
        settings.color = Qt::blue;
        settings.visible = true;
        plane->updateSettings(settings);
        // Create initial instance at offset 0
        plane->createInstance(0.0);
    }
    
    return plane;
}