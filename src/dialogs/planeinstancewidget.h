#pragma once

#include <QWidget>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>

#include "planeinstance.h"

/**
 * Simple widget for editing PlaneInstance properties (mainly offset and visibility)
 */
class PlaneInstanceWidget : public QWidget {
    Q_OBJECT

public:
    explicit PlaneInstanceWidget(QWidget *parent = nullptr);
    
    void setPlaneInstance(PlaneInstance *instance);
    PlaneInstance *planeInstance() const { return _instance; }

private slots:
    void onOffsetChanged();
    void onVisibilityChanged(bool visible);
    void updateFromInstance();

private:
    void setupUI();
    void connectSignals();
    void disconnectSignals();
    
    PlaneInstance *_instance{nullptr};
    
    // UI elements
    QLabel *_nameLabel{nullptr};
    QLabel *_parentPlaneLabel{nullptr};
    QDoubleSpinBox *_offsetSpinBox{nullptr};
    QCheckBox *_visibleCheckBox{nullptr};
};