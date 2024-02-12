#pragma once
#include "atomid.h"
#include "chargemultiplicitypair.h"
#include "deprecatedcrystal.h"
#include "jobparameters.h"
#include "transformablewavefunction.h"
#include "ui_energycalculationdialog.h"
#include "wavefunction.h"
#include <QDialog>

enum WavefunctionRequirement {
  noWavefunctionRequired,
  choosableWavefunctionForA,
  complementaryWavefunctionForA,
  complementaryWavefunctionForB
};

class EnergyCalculationDialog : public QDialog,
                                public Ui::EnergyCalculationDialog {
  Q_OBJECT

public:
  EnergyCalculationDialog(QWidget *);
  void setAtomsForCalculation(const QVector<AtomId> &, const QVector<AtomId> &);
  void
  setChargesAndMultiplicitiesForCalculation(const ChargeMultiplicityPair &cmA,
                                            const ChargeMultiplicityPair &cmB) {
    _chargeA = cmA.charge;
    _chargeB = cmB.charge;
    _multiplicityA = cmA.multiplicity;
    _multiplicityB = cmB.multiplicity;
  }
  void setWavefunctions(
      const QVector<QPair<TransformableWavefunction, TransformableWavefunction>>
          &,
      bool);
  inline bool waitingOnWavefunction() { return _waitingOnWavefunction; }
  QVector<AtomId> atomsForWavefunction();
  QVector<AtomId> atomsForFragmentA();
  QVector<AtomId> atomsForFragmentB();
  bool calculatedEnergiesForAllPairs();
  void setAtomsForRemainingFragments(const QVector<QVector<AtomId>> &);
  void setMethodAndBasis(Method, BasisSet);
  QVector<AtomId> nextFragmentAtoms();
  void calculate();
  void setCrystal(DeprecatedCrystal *);
  int currentStep() const { return _currentCalculationIndex; }

signals:
  void energyParametersChosen(const JobParameters &,
                              const QVector<Wavefunction> &);
  void requireWavefunction(const QVector<AtomId> &, int, int);
  void requireSpecifiedWavefunction(const JobParameters &);
  void requireMonomerEnergy(const JobParameters &);

private slots:
  void validate();
  void modelChemistryChanged();

private:
  bool findMatchingWavefunctions();
  bool findMatchingMonomerEnergies();
  bool needWavefunctionCalculationDialog();
  void init();
  void initConnections();
  void updateWavefunctionComboBox();
  void calculateDLPNO();
  void calculateGFN();

  JobParameters createWavefunctionCalculationJobParameters(
      const QVector<AtomId> &, int charge = 0, int multiplicity = 1) const;
  JobParameters createMonomerEnergyCalculationJobParameters(
      const QVector<AtomId> &, int charge = 0, int multiplicity = 1) const;
  int chargeForFragmentA() { return _chargeA; }
  int multiplicityForFragmentA() { return _multiplicityA; }
  int chargeForFragmentB() { return _chargeB; }
  int multiplicityForFragmentB() { return _multiplicityB; }
  void showEvent(QShowEvent *);

  std::vector<TransformableWavefunction> m_waveFunctions;
  std::vector<MonomerEnergy> m_monomerEnergies;
  QVector<AtomId> _atomsForCalculation;
  QVector<QVector<AtomId>> _atomsForRemainingFragments;
  QVector<int> _atomGroups;
  DeprecatedCrystal *m_crystal{nullptr};
  bool _AandBSymmetryRelated;
  bool _waitingOnWavefunction;
  bool m_foundA{false};
  bool m_foundB{false};
  int _numberOfCalculations;
  int _currentCalculationIndex;
  int _chargeA;
  int _chargeB;
  int _multiplicityA;
  int _multiplicityB;
  int _numWavefunctionsComputed{0};
  Method m_method{Method::b3lyp};
  BasisSet m_basis{BasisSet::Pople6_31Gdp};
};
