#pragma once
#include <QTableWidget>
#include <QTextDocument>
#include <QWidget>

#include "mesh.h"
#include "meshpropertymodel.h"
#include "ui_surfacecontroller.h"

// The following strings *must* match those in the propertyFromString map (see
// surfacedescription.h)
const QStringList clampedProperties = QStringList() << "shape_index"
                                                    << "curvedness"
                                                    << "none";
const QVector<float> clampedMinimumScaleValues = QVector<float>()
                                                 << -1.0f << -4.0f << 0.0f;
const QVector<float> clampedMaximumScaleValues = QVector<float>()
                                                 << +1.0f << +0.4f << 0.0f;

class SurfaceController : public QWidget, public Ui::SurfaceController {
  Q_OBJECT

public:
  SurfaceController(QWidget *parent = 0);
  void setSurfaceInfo(float, float, float, float);
  void enableFingerprintButton(bool);
  void currentSurfaceVisibilityChanged(bool);

public slots:
  void setCurrentMesh(Mesh *);
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
