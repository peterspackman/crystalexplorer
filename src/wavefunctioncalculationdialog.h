#pragma once
#include <QDialog>

#include "atomid.h"
#include "jobparameters.h"
#include "ui_wavefunctioncalculationdialog.h"

class Crystalx;

const QString GAUSSIAN_TAB_TOOLTIP =
    "This tab is not available because the Gaussian program could not be found";
const QString DIALOG_TITLE = "Wavefunction Calculation";
const ExternalProgram DEFAULT_WAVEFUNCTION_SOURCE = ExternalProgram::Gaussian;
static const int DEFAULT_MULTIPLICITY =
    1; // We can't handle non-singlet spin states so this is fixed

class WavefunctionCalculationDialog : public QDialog,
                                      public Ui::WavefunctionCalculationDialog {
  Q_OBJECT

public:
  WavefunctionCalculationDialog(QWidget *parent = 0);
  void setAtomsForCalculation(QVector<AtomId> atoms) {
    _atomsForCalculation = atoms;
  }
  void setChargeForCalculation(int charge) { _charge = charge; }
  void setMultiplicityForCalculation(int multiplicity) {
    _multiplicity = multiplicity;
  }

public slots:
  void show();

signals:
  void wavefunctionParametersChosen(const JobParameters &);

private slots:
  void accept();
  void updatesForMethodChange();

private:
  void init();
  void initPrograms();
  void initMethod();
  void initBasissets();
  void initExchangePotentials();
  void initCorrelationPotentials();
  void initConnections();

  void setDFTOptionVisibility(bool);
  ExternalProgram currentWavefunctionSource();
  Method currentMethod();
  BasisSet currentBasisset();
  ExchangePotential currentExchangePotential();
  CorrelationPotential currentCorrelationPotential();

  QVector<ExternalProgram> _programs;
  QVector<AtomId> _atomsForCalculation;
  int _charge;
  int _multiplicity;
};
