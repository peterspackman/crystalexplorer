#pragma once
#include <QMap>
#include <QTableWidget>
#include <QTextDocument>
#include <QWidget>

#include "mesh.h"
#include "meshinstance.h"
#include "meshpropertymodel.h"
#include "molecular_wavefunction.h"
#include "pair_energy_results.h"
#include "frameworkoptions.h"
#include "ui_childpropertycontroller.h"

class ChildPropertyController : public QWidget,
                                public Ui::ChildPropertyController {
  Q_OBJECT

public:
  ChildPropertyController(QWidget *parent = 0);
  void enableFingerprintButton(bool);
  void currentSurfaceVisibilityChanged(bool);
  Mesh *getCurrentMesh();

  bool showEnergyFramework() const;
  void setShowEnergyFramework(bool);
  bool toggleShowEnergyFramework();
  void reset();

public slots:
  void setCurrentMesh(Mesh *);
  void setCurrentMeshInstance(MeshInstance *);
  void setCurrentWavefunction(MolecularWavefunction *);
  void setCurrentPairInteractions(PairInteractions *);
  void setSelectedPropertyValue(float);

protected slots:
  void onSurfaceTransparencyChange(bool);
  void onModelPropertySelectionChanged(QString);
  void onComboBoxPropertySelectionChanged(QString);
  void propertyRangeChanged();
  void resetScale();
  void exportButtonClicked();
  void onMeshModelUpdate();
  void onFrameworkColoringChanged();

signals:
  void surfacePropertyChosen(int);
  void showFingerprint();
  void exportCurrentSurface();
  void frameworkOptionsChanged(FrameworkOptions);

private:

  enum class DisplayState {
    None,
    Mesh,
    Wavefunction,
    Framework,
  };

  void setFrameworkDisplay(FrameworkOptions::Display);

  void showSurfaceTabs(bool);
  void showWavefunctionTabs(bool);
  void showFrameworkTabs(bool);
  void showTab(QWidget *, bool, QString);
  void emitFrameworkOptions();

  void setup();
  void setScale(Mesh::ScalarPropertyRange);

  void setMinAndMaxSpinBoxes(float, float);
  void setUnitLabels(QString);

  void clearPropertyInfo();
  void enableSurfaceControls(bool);

  void updatePairInteractionModels();
  void updatePairInteractionComponents();

  DisplayState m_state{DisplayState::None};
  FrameworkOptions::Display m_frameworkDisplay{FrameworkOptions::Display::None};
  FrameworkOptions::Display m_previousNonNoneDisplay{FrameworkOptions::Display::Tubes};

  MeshPropertyModel *m_meshPropertyModel{nullptr};
  PairInteractions *m_pairInteractions{nullptr};
  QMap<QString, Mesh::ScalarPropertyRange> m_clampedProperties;
  QColor m_customFrameworkColor{Qt::blue};
};
