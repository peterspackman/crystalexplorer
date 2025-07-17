#pragma once
#include <QCheckBox>
#include <QDialog>
#include <QStringList>
#include <optional>
#include <vector>

#include "generic_atom_index.h"
#include "isosurface_parameters.h"
#include "molecular_wavefunction.h"
#include "molecular_orbital_selector.h"
#include "ui_surfacegenerationdialog.h"
#include "wavefunction_parameters.h"

static const char *densityUnits = "e au<sup>-3</sup>";
static const QStringList surfaceIsovalueUnits =
    QStringList() << "" << densityUnits << densityUnits << densityUnits
                  << densityUnits << "au"
                  << "au"
                  << "au<sup>-3</sup>" << densityUnits;

static const bool defaultHideWavefunctionBox = true;
static const bool defaultHideSurfaceOptionsBox = true;
static const Qt::CheckState defaultShowDescriptions = Qt::Unchecked;

class SurfaceGenerationDialog : public QDialog {
  Q_OBJECT

public:
  SurfaceGenerationDialog(QWidget *parent = 0);
  void setChargeForCalculation(int charge);
  void setMultiplicityForCalculation(int multiplicity);
  void setNumberOfElectronsForCalculation(int numberOfElectrons);
  void setSuitableWavefunctions(const std::vector<WavefunctionAndTransform> &);

  void setAtomIndices(const std::vector<GenericAtomIndex> &atoms);
  [[nodiscard]] const std::vector<GenericAtomIndex> &atomIndices() const;
  
  void setStructure(ChemicalStructure *structure);

private slots:
  void surfaceChanged(QString);
  void propertyChanged(QString);
  void updateDescriptions();
  void validate();

signals:
  void surfaceParametersChosenNew(isosurface::Parameters);
  void surfaceParametersChosenNeedWavefunction(isosurface::Parameters,
                                               wfn::Parameters);

private:
  isosurface::Kind currentKind() const;
  QString currentKindName() const;
  QString currentPropertyName() const;

  void init();
  void initConnections();
  void connectPropertyComboBox(bool);
  void updateSettings();
  void updateIsovalue();
  void updatePropertyOptions();
  void updateSurfaceOptions();
  void updateOrbitalLabels();
  void setupOrbitalUI();
  void updateWavefunctionComboBox(bool selectLast = false);
  bool havePropertyChoices();
  bool needIsovalueBox();
  bool needClusterOptions();
  bool needOrbitalBox();
  bool needWavefunction();
  bool mustCalculateWavefunction();
  bool shouldAlsoCalculateNegativeIsovalue() const;
  bool needWavefunctionCalc();

  wfn::Parameters getCurrentWavefunctionParameters();
  QString generateFragmentIdentifier() const;

  std::vector<GenericAtomIndex> m_atomIndices;
  ChemicalStructure *m_structure{nullptr};
  QString m_currentSurfaceType{"Hirshfeld"};

  std::vector<WavefunctionAndTransform> m_availableWavefunctions;

  isosurface::SurfaceDescriptions m_surfaceDescriptions;
  isosurface::SurfacePropertyDescriptions m_surfacePropertyDescriptions;
  int m_charge{0}, m_multiplicity{1}, m_numElectrons{0};

  Ui::SurfaceGenerationDialog *ui;
};
