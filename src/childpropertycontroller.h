#pragma once
#include <QMap>
#include <QTableWidget>
#include <QTextDocument>
#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QLabel>

#include "frameworkoptions.h"
#include "mesh.h"
#include "meshinstance.h"
#include "meshpropertymodel.h"
#include "molecular_wavefunction.h"
#include "pair_energy_results.h"
#include "plane.h"
#include "planeinstance.h"
#include "planeinfowidget.h"
#include "planeinstancewidget.h"
#include "ui_childpropertycontroller.h"

class ChildPropertyController : public QWidget,
                                public Ui::ChildPropertyController {
  Q_OBJECT

public:
  ChildPropertyController(QWidget *parent = 0);
  void enableFingerprintButton(bool);
  void currentSurfaceVisibilityChanged(bool);
  Mesh *getCurrentMesh();
  MeshInstance *getCurrentMeshInstance();

  bool showEnergyFramework() const;
  void setShowEnergyFramework(bool);
  bool toggleShowEnergyFramework();
  void reset();

  // New unified method for handling object selection
  void setCurrentObject(QObject *obj);
public slots:

  void setCurrentMesh(Mesh *);
  void setCurrentMeshInstance(MeshInstance *);
  void setCurrentWavefunction(MolecularWavefunction *);
  void setCurrentPairInteractions(PairInteractions *);
  void setCurrentPlane(Plane *);
  void setCurrentPlaneInstance(PlaneInstance *);
  void setSelectedPropertyValue(float);

protected slots:
  void onSurfaceTransparencyChange(bool);
  void onSurfaceVarTransparecyChange(float);
  void onModelPropertySelectionChanged(QString);
  void onComboBoxPropertySelectionChanged(QString);
  void propertyRangeChanged();
  void resetScale();
  void exportButtonClicked();
  void onMeshModelUpdate();
  void onFrameworkColoringChanged();
  void onGenerateSlabRequested(int h, int k, int l, double offset);

signals:
  void surfacePropertyChosen(int);
  void showFingerprint();
  void exportCurrentSurface();
  void frameworkOptionsChanged(FrameworkOptions);
  void meshSelectionChanged();
  void generateSlabRequested(int h, int k, int l, double offset);

private:
  enum class DisplayState {
    None,
    Mesh,
    Wavefunction,
    Framework,
    Plane,
    PlaneInstance,
  };

  void setFrameworkDisplay(FrameworkOptions::Display);
  void handleStructureSelection(ChemicalStructure *structure);

  void showSurfaceTabs(bool);
  void showWavefunctionTabs(bool);
  void showFrameworkTabs(bool);
  void showPlaneTabs(bool);
  void showPlaneInstanceTabs(bool);
  void createPlanePropertiesTab();
  void createPlaneInstancePropertiesTab();
  void updatePlaneInfo(Plane *plane, PlaneInstance *instance = nullptr);
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
  FrameworkOptions::Display m_previousNonNoneDisplay{
      FrameworkOptions::Display::Tubes};

  MeshPropertyModel *m_meshPropertyModel{nullptr};
  PairInteractions *m_pairInteractions{nullptr};
  
  // Plane properties tab
  QWidget *m_planePropertiesTab{nullptr};
  PlaneInfoWidget *m_planeInfoWidget{nullptr};
  PlaneInstanceWidget *m_planeInstanceWidget{nullptr};
  
  QWidget *m_planeInstancePropertiesTab{nullptr};
  QMap<QString, Mesh::ScalarPropertyRange> m_clampedProperties;
  QColor m_customFrameworkColor{Qt::blue};
};
