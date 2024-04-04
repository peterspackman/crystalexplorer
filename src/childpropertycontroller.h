#pragma once
#include <QTableWidget>
#include <QTextDocument>
#include <QWidget>

#include "mesh.h"
#include "molecular_wavefunction.h"
#include "meshpropertymodel.h"
#include "ui_childpropertycontroller.h"

// The following strings *must* match those in the propertyFromString map (see
// surfacedescription.h)
const QStringList clampedProperties = QStringList() << "shape_index"
                                                    << "curvedness"
                                                    << "none";
const QVector<float> clampedMinimumScaleValues = QVector<float>()
                                                 << -1.0f << -4.0f << 0.0f;
const QVector<float> clampedMaximumScaleValues = QVector<float>()
                                                 << +1.0f << +0.4f << 0.0f;

class ChildPropertyController : public QWidget, public Ui::ChildPropertyController {
  Q_OBJECT

public:
  ChildPropertyController(QWidget *parent = 0);
  void setSurfaceInfo(float, float, float, float);
  void enableFingerprintButton(bool);
  void currentSurfaceVisibilityChanged(bool);

public slots:
  void setCurrentMesh(Mesh *);
  void setCurrentWavefunction(MolecularWavefunction *);
  void setMeshPropertyInfo(const Mesh::ScalarPropertyValues &);
  void setSelectedPropertyValue(float);

protected slots:
  void onSurfaceTransparencyChange(bool);
  void onPropertySelectionChanged(int);
  void minPropertyChanged();
  void maxPropertyChanged();
  void resetScale();
  void exportButtonClicked();

signals:
  void surfacePropertyChosen(int);
  void showFingerprint();
  void surfacePropertyRangeChanged(float, float);
  void exportCurrentSurface();

private:
  void showSurfaceTabs(bool);
  void showWavefunctionTabs(bool);
  void showTab(QWidget *, bool, QString);

  void setup();
  void setScale(float, float);
  void clampScale(float, float,
                  bool emitUpdateSurfacePropertyRangeSignal = false);
  void setMinAndMaxSpinBoxes(float, float);
  void setUnitLabels(QString);
  void updateSurfacePropertyRange();
  void clearPropertyInfo();
  QString convertToNaturalPropertyName(QString);
  void enableSurfaceControls(bool);

  bool _updateSurfacePropertyRange;

  MeshPropertyModel *m_meshPropertyModel{nullptr};
};
