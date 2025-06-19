#pragma once
#include <QDialog>
#include <QColor>
#include <QListWidget>
#include <QComboBox>
#include "plane.h"
#include "planeinstance.h"
#include "crystalplane_unified.h"

class QPushButton;
class QLabel;

struct PlanePreset {
    QString name;
    QString description;
    bool isCrystal;
    // For Cartesian planes
    QVector3D normal;
    QVector3D origin;
    // For crystal planes 
    int h, k, l;
};

/**
 * Simple dialog for selecting plane presets.
 * Users choose from a list of common planes, then configure details later.
 */
class PlaneDialog : public QDialog {
    Q_OBJECT

public:
    explicit PlaneDialog(QWidget *parent = nullptr);
    
    // Create a Plane object based on selected preset
    Plane* createPlane(QObject *parent = nullptr) const;
    
    // Get the selected preset
    PlanePreset selectedPreset() const;

private slots:
    void onPresetSelectionChanged();
    void onPlaneTypeChanged();

private:
    void setupUI();
    void updateOffsetList();
    
    // UI elements
    QComboBox *_planeTypeCombo;
    QListWidget *_presetListWidget;
    QLabel *_descriptionLabel;
    
    std::vector<PlanePreset> _cartesianPresets;
    std::vector<PlanePreset> _crystalPresets;
    
    void setupCartesianPresets();
    void setupCrystalPresets();
    void updatePresetList();
    void updateDescription();
};