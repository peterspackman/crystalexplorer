#include "planeinstancewidget.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

PlaneInstanceWidget::PlaneInstanceWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void PlaneInstanceWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    
    // Instance Info Group
    auto *infoGroup = new QGroupBox("Plane Instance");
    auto *infoLayout = new QFormLayout(infoGroup);
    
    _nameLabel = new QLabel("—");
    _nameLabel->setWordWrap(true);
    infoLayout->addRow("Name:", _nameLabel);
    
    _parentPlaneLabel = new QLabel("—");
    _parentPlaneLabel->setWordWrap(true);
    infoLayout->addRow("Parent Plane:", _parentPlaneLabel);
    
    mainLayout->addWidget(infoGroup);
    
    // Properties Group
    auto *propertiesGroup = new QGroupBox("Properties");
    auto *propertiesLayout = new QFormLayout(propertiesGroup);
    
    _visibleCheckBox = new QCheckBox("Visible");
    propertiesLayout->addRow(_visibleCheckBox);
    
    _offsetSpinBox = new QDoubleSpinBox;
    _offsetSpinBox->setRange(-1000, 1000);
    _offsetSpinBox->setDecimals(3);
    _offsetSpinBox->setSuffix(" Å");
    _offsetSpinBox->setToolTip("Distance along the plane normal from the origin");
    propertiesLayout->addRow("Offset:", _offsetSpinBox);
    
    mainLayout->addWidget(propertiesGroup);
    
    // Note: Instance deletion is handled via right-click context menu in the object tree
    
    mainLayout->addStretch();
}

void PlaneInstanceWidget::setPlaneInstance(PlaneInstance *instance)
{
    if (_instance == instance)
        return;
        
    disconnectSignals();
    _instance = instance;
    
    if (_instance) {
        connectSignals();
        updateFromInstance();
    } else {
        // Clear UI when no instance
        _nameLabel->setText("—");
        _parentPlaneLabel->setText("—");
        _offsetSpinBox->setValue(0.0);
        _visibleCheckBox->setChecked(false);
        setEnabled(false);
    }
}

void PlaneInstanceWidget::connectSignals()
{
    if (!_instance)
        return;
        
    // Connect widget signals to slots
    connect(_offsetSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PlaneInstanceWidget::onOffsetChanged);
    connect(_visibleCheckBox, &QCheckBox::toggled,
            this, &PlaneInstanceWidget::onVisibilityChanged);
    
    // Connect instance signals to update UI
    connect(_instance, &PlaneInstance::offsetChanged,
            this, &PlaneInstanceWidget::updateFromInstance);
    connect(_instance, &PlaneInstance::visibilityChanged,
            this, &PlaneInstanceWidget::updateFromInstance);
    connect(_instance, &PlaneInstance::nameChanged,
            this, &PlaneInstanceWidget::updateFromInstance);
}

void PlaneInstanceWidget::disconnectSignals()
{
    if (!_instance)
        return;
        
    disconnect(_instance, nullptr, this, nullptr);
}

void PlaneInstanceWidget::updateFromInstance()
{
    if (!_instance)
        return;
        
    setEnabled(true);
    
    // Block signals to prevent loops
    _offsetSpinBox->blockSignals(true);
    _visibleCheckBox->blockSignals(true);
    
    // Update values
    _nameLabel->setText(_instance->name());
    if (_instance->plane()) {
        _parentPlaneLabel->setText(_instance->plane()->name());
    } else {
        _parentPlaneLabel->setText("Invalid");
    }
    
    _offsetSpinBox->setValue(_instance->offset());
    _visibleCheckBox->setChecked(_instance->isVisible());
    
    
    // Unblock signals
    _offsetSpinBox->blockSignals(false);
    _visibleCheckBox->blockSignals(false);
}

void PlaneInstanceWidget::onOffsetChanged()
{
    if (_instance) {
        _instance->setOffset(_offsetSpinBox->value());
    }
}

void PlaneInstanceWidget::onVisibilityChanged(bool visible)
{
    if (_instance) {
        _instance->setVisible(visible);
    }
}

