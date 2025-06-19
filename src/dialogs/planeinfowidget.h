#pragma once

#include <QWidget>
#include <QColorDialog>
#include "ui_planeinfowidget.h"
#include "plane.h"

// Forward declaration to avoid circular dependency
class CrystalPlaneUnified;

class PlaneInfoWidget : public QWidget, public Ui::PlaneInfoWidget {
    Q_OBJECT

public:
    explicit PlaneInfoWidget(QWidget *parent = nullptr);
    
    void setPlane(Plane *plane);
    Plane *plane() const { return _plane; }

private slots:
    void onCreateInstanceClicked();
    void onNameChanged();
    void onColorButtonClicked();
    void onOriginChanged();
    void onNormalChanged();
    void onGridPropertiesChanged();
    void onBoundsChanged();
    void onMillerIndicesChanged();
    void onGenerateSlabClicked();
    
    // Slots to update UI when plane properties change
    void updateFromPlane();

signals:
    void createInstanceRequested(double offset);
    void generateSlabRequested(int h, int k, int l, double offset);

private:
    void connectUISignals();
    void updateColorButton();
    void updateUIMode();
    void updateCrystalProperties();
    void updateOffsetUnits();
    void updateGridUnits();
    void clearUI();
    
    bool isCrystalPlane() const;
    CrystalPlaneUnified* crystalPlane() const;
    
    Plane *_plane{nullptr};
    bool _updatingFromPlane{false};
};