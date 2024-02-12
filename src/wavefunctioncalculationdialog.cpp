#include <QDialog>
#include <QtDebug>

#include "gaussianinterface.h"
#include "nwcheminterface.h"
#include "wavefunctioncalculationdialog.h"

WavefunctionCalculationDialog::WavefunctionCalculationDialog(QWidget *parent)
    : QDialog(parent) {
  setupUi(this);
  init();
}

void WavefunctionCalculationDialog::init() {
  setWindowTitle(DIALOG_TITLE);
  setModal(true);

  // Initialize the charge (though this should always be overridden with a call
  // to setChargeForCalculation)
  _charge = 0;

  // put available options in the dialog
  initPrograms();
  initMethod();
  initConnections();
  initBasissets();
  initExchangePotentials();
  initCorrelationPotentials();
  adjustSize();
}

void WavefunctionCalculationDialog::initPrograms() {
  programCombobox->clear();
  _programs = availableExternalPrograms; // defined in JobParameters

  // Remove options if program unavailable
  if (!GaussianInterface::executableInstalled()) {
    _programs.removeOne(ExternalProgram::Gaussian);
  }

  if (!NWChemInterface::executableInstalled()) {
    _programs.removeOne(ExternalProgram::NWChem);
  }

  ExternalProgram preferred = JobParameters::prefferedWavefunctionSource();
  int idx = 0;
  for (const auto &source : _programs) {
    programCombobox->addItem(externalProgramLabel(source));
    if (source == preferred)
      programCombobox->setCurrentIndex(idx);
    idx++;
  }
}

void WavefunctionCalculationDialog::initMethod() {
  for (const auto &i : includeMethod) {
    methodCombobox->addItem(methodLabels[static_cast<int>(i)]);
  }

  setDFTOptionVisibility(currentMethod() == Method::kohnSham);
}

void WavefunctionCalculationDialog::initBasissets() {
  for (const auto &basis : includeBasisset) {
    basissetCombobox->addItem(basisSetLabel(basis));
  }
}

void WavefunctionCalculationDialog::initExchangePotentials() {
  QVectorIterator<ExchangePotential> i(includeExchangePotential);
  while (i.hasNext()) {
    exchangeCombobox->addItem(
        exchangePotentialLabels[static_cast<int>(i.next())]);
  }
}

void WavefunctionCalculationDialog::initCorrelationPotentials() {
  QVectorIterator<CorrelationPotential> i(includeCorrelationPotential);
  while (i.hasNext()) {
    correlationCombobox->addItem(
        correlationPotentialLabels[static_cast<int>(i.next())]);
  }
}

void WavefunctionCalculationDialog::initConnections() {
  connect(methodCombobox, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(updatesForMethodChange()));
}

void WavefunctionCalculationDialog::show() {
  initPrograms(); // wavefunction program availability might have changed so
                  // reinitialise
  QWidget::show();
}

void WavefunctionCalculationDialog::accept() {
  // Setup jobParameters
  JobParameters jobParams;
  jobParams.jobType = JobType::wavefunction;
  jobParams.program = currentWavefunctionSource();
  jobParams.theory = currentMethod();
  jobParams.exchangePotential = currentExchangePotential();
  jobParams.correlationPotential = currentCorrelationPotential();
  jobParams.basisset = currentBasisset();
  jobParams.editInputFile =
      (editInputFileCheckbox->checkState() == Qt::Checked);

  jobParams.atoms = _atomsForCalculation;
  jobParams.charge = _charge;
  jobParams.multiplicity = _multiplicity;

  emit wavefunctionParametersChosen(jobParams);

  QDialog::accept();
}

void WavefunctionCalculationDialog::updatesForMethodChange() {
  setDFTOptionVisibility(currentMethod() == Method::kohnSham);
  adjustSize();
}

void WavefunctionCalculationDialog::setDFTOptionVisibility(bool visible) {
  DFTOptions->setVisible(visible);
}

ExternalProgram WavefunctionCalculationDialog::currentWavefunctionSource() {
  return _programs[programCombobox->currentIndex()];
}

Method WavefunctionCalculationDialog::currentMethod() {
  return includeMethod[methodCombobox->currentIndex()];
}

BasisSet WavefunctionCalculationDialog::currentBasisset() {
  return includeBasisset[basissetCombobox->currentIndex()];
}

ExchangePotential WavefunctionCalculationDialog::currentExchangePotential() {
  return includeExchangePotential[exchangeCombobox->currentIndex()];
}

CorrelationPotential
WavefunctionCalculationDialog::currentCorrelationPotential() {
  return includeCorrelationPotential[correlationCombobox->currentIndex()];
}
