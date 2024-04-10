#pragma once
#include <QTableWidget>
#include <QTextDocument>
#include <QMap>
#include <QWidget>

#include "mesh.h"
#include "meshinstance.h"
#include "molecular_wavefunction.h"
#include "meshpropertymodel.h"
#include "ui_childpropertycontroller.h"

class ChildPropertyController : public QWidget, public Ui::ChildPropertyController {
  Q_OBJECT

public:
  ChildPropertyController(QWidget *parent = 0);
  void enableFingerprintButton(bool);
  void currentSurfaceVisibilityChanged(bool);

public slots:
  void setCurrentMesh(Mesh *);
  void setCurrentMeshInstance(MeshInstance *);
  void setCurrentWavefunction(MolecularWavefunction *);
  void setSelectedPropertyValue(float);

protected slots:
  void onSurfaceTransparencyChange(bool);
  void onModelPropertySelectionChanged(QString);
  void onComboBoxPropertySelectionChanged(QString);
  void propertyRangeChanged();
  void resetScale();
  void exportButtonClicked();
  void onMeshModelUpdate();

signals:
  void surfacePropertyChosen(int);
  void showFingerprint();
  void exportCurrentSurface();

private:
  void showSurfaceTabs(bool);
  void showWavefunctionTabs(bool);
  void showTab(QWidget *, bool, QString);

  void setup();
  void setScale(Mesh::ScalarPropertyRange);

  void setMinAndMaxSpinBoxes(float, float);
  void setUnitLabels(QString);

  void clearPropertyInfo();
  void enableSurfaceControls(bool);

  bool _updateSurfacePropertyRange;

  MeshPropertyModel *m_meshPropertyModel{nullptr};
  QMap<QString, Mesh::ScalarPropertyRange> m_clampedProperties;
};
