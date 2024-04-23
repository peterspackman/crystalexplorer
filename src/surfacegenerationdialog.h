#pragma once
#include <QCheckBox>
#include <QDialog>
#include <QStringList>
#include <optional>
#include <vector>

#include "generic_atom_index.h"
#include "atomid.h"
#include "jobparameters.h"
#include "molecular_wavefunction.h"
#include "surfacedescription.h"
#include "transformablewavefunction.h"
#include "ui_surfacegenerationdialog.h"
#include "wavefunction.h"
#include "isosurface_parameters.h"
#include "wavefunction_parameters.h"

static const char *densityUnits = "e au<sup>-3</sup>";
static const QStringList surfaceIsovalueUnits =
    QStringList() << "" << densityUnits << densityUnits << densityUnits
                  << densityUnits << "au"
                  << "au"
                  << "au<sup>-3</sup>" << densityUnits;

static const bool defaultHideWavefunctionBox = true;
static const bool defaultHideSurfaceOptionsBox = true;
static const Qt::CheckState defaultEditTonto = Qt::Unchecked;
static const Qt::CheckState defaultShowDescriptions = Qt::Unchecked;

class SurfaceGenerationDialog : public QDialog {
  Q_OBJECT

public:
  SurfaceGenerationDialog(QWidget *parent = 0);
  void setWavefunction(Wavefunction *);
  void setAtomsForCalculation(const QVector<AtomId> &atoms) {
    _atomsForCalculation = atoms;
  }
  void setChargeForCalculation(int charge) { _charge = charge; }
  void setMultiplicityForCalculation(int multiplicity) {
    _multiplicity = multiplicity;
  }
  void setSuppressedAtomsForCalculation(QVector<int> atomsIndices) {
    _suppressedAtomsForCalculation = atomsIndices;
  }
  void setSuitableWavefunctions(QVector<TransformableWavefunction>);
  void setSuitableWavefunctions(const std::vector<WavefunctionAndTransform> &);
  void setWavefunctionDone(TransformableWavefunction);
  const QVector<AtomId> &atomsForCalculation() { return _atomsForCalculation; }
  const QVector<int> &suppressedAtomsForCalculation() {
    return _suppressedAtomsForCalculation;
  }

  bool waitingOnWavefunction() { return _waitingOnWavefunction; }

  [[nodiscard]] const std::vector<GenericAtomIndex> &atomIndices() const;

private slots:
  void surfaceChanged(QString);
  void propertyChanged();
  void updateDescriptions();
  void validate();
  void setSignLabel(int);

signals:
  void surfaceParametersChosen(const JobParameters &,
                               std::optional<Wavefunction>);
  void surfaceParametersChosenNew(isosurface::Parameters);

  void surfaceParametersChosenNeedWavefunction(isosurface::Parameters, wfn::Parameters);

  void requireWavefunction(const QVector<AtomId> &, int, int);

private:
  void init();
  void initConnections();
  void connectPropertyComboBox(bool);
  void updateSettings();
  void updateIsovalue();
  void updatePropertyOptions();
  void updateSurfaceOptions();
  void updateOrbitalOptions();
  void updateWavefunctionComboBox(bool selectLast = false);
  bool havePropertyChoices();
  bool needIsovalueBox();
  bool needClusterOptions();
  bool needOrbitalBox();
  bool needWavefunction();
  bool wavefunctionIsValid(Wavefunction *, const QVector<AtomId> &) const;
  bool mustCalculateWavefunction();
  bool needWavefunctionCalc();
  void copyWavefunctionParamsIntoSurfaceParams(JobParameters &,
                                               const JobParameters &);
  TransformableWavefunction wavefunctionForCurrentComboboxSelection();

  std::vector<GenericAtomIndex> m_atomIndices;
  QString m_currentSurfaceType{"hirshfeld"};

  QVector<TransformableWavefunction> _wavefunctions;
  std::vector<WavefunctionAndTransform> m_availableWavefunctions;

  QVector<AtomId> _atomsForCalculation;
  QVector<int> _suppressedAtomsForCalculation;
  QVector<IsosurfaceDetails::Type> m_indexToSurfaceType;

  QMap<QString, isosurface::SurfaceDescription> m_surfaceDescriptions;
  QMap<QString, isosurface::SurfacePropertyDescription> m_surfacePropertyDescriptions;

  bool _waitingOnWavefunction;
  int _charge, _multiplicity;

  Ui::SurfaceGenerationDialog *ui;
};
