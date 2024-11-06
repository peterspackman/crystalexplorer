#include <QDialog>
#include <QtDebug>
#include <ankerl/unordered_dense.h>

#include "wavefunctioncalculationdialog.h"
#include "settings.h"
#include "xtb_parameters.h"

const QString WavefunctionCalculationDialog::customEntry{"Custom..."};

WavefunctionCalculationDialog::WavefunctionCalculationDialog(QWidget *parent)
    : QDialog(parent) {
  setupUi(this);
  init();
}

void WavefunctionCalculationDialog::init() {
  setWindowTitle("Wavefunction Calculation");
  setModal(true);

  // put available options in the dialog
  initPrograms();
  updateMethodOptions();
  updateBasisSetOptions();
  adjustSize();
}

void WavefunctionCalculationDialog::initPrograms() {
  programComboBox->clear();

  ankerl::unordered_dense::map<QString, QString> programs {
    {"OCC", settings::readSetting(settings::keys::OCC_EXECUTABLE).toString()},
    {"Gaussian", settings::readSetting(settings::keys::GAUSSIAN_EXECUTABLE).toString()},
    {"Orca", settings::readSetting(settings::keys::ORCA_EXECUTABLE).toString()},
    {"XTB", settings::readSetting(settings::keys::XTB_EXECUTABLE).toString()},
  };

  QString preferred = settings::readSetting(settings::keys::PREFERRED_WAVEFUNCTION_SOURCE).toString();
  for (const auto &[source, exe] : programs) {
    if(exe.isEmpty()) continue;
    programComboBox->addItem(source);
    if (source == preferred) {
      programComboBox->setCurrentText(source);
    }
  }
  connect(programComboBox, &QComboBox::currentTextChanged,
          this, &WavefunctionCalculationDialog::updateMethodOptions);

  connect(methodComboBox, QOverload<int>::of(&QComboBox::activated),
          [&](int index) {
            if (methodComboBox->itemText(index) == customEntry) {
              methodComboBox->setEditable(true);
              methodComboBox->clearEditText();
              methodComboBox->setFocus();
              methodComboBox->showPopup();
              methodComboBox->setToolTip(
                  tr("Type here to enter a custom value"));
            } else {
              methodComboBox->setEditable(false);
            }
          });
  connect(methodComboBox, &QComboBox::currentTextChanged,
          this, &WavefunctionCalculationDialog::updateBasisSetOptions);
  connect(
      basisComboBox, QOverload<int>::of(&QComboBox::activated), [&](int index) {
        if (basisComboBox->itemText(index) == customEntry) {
          basisComboBox->setEditable(true);
          basisComboBox->clearEditText();
          basisComboBox->setFocus();
          basisComboBox->showPopup();
          basisComboBox->setToolTip(tr("Type here to enter a custom value"));
        } else {
          basisComboBox->setEditable(false);
        }
      });
}

void WavefunctionCalculationDialog::updateMethodOptions() {

  ankerl::unordered_dense::map<QString, QStringList> programMethods{
    {"OCC", {"HF", "B3LYP", "wB97m-V"}},
    {"Orca", {"HF", "B3LYP", "wB97m-V"}},
    {"Gaussian", {"HF", "B3LYP", "wB97m-V"}},
    {"XTB", {"GFN0-xTB", "GFN1-xTB", "GFN2-xTB"}},
  };

  methodComboBox->clear();
  QString prog = selectedProgramName();
  QStringList options = programMethods[prog];
  for (const auto &method : options) {
    methodComboBox->addItem(method);
  }
  methodComboBox->addItem(customEntry);

}

void WavefunctionCalculationDialog::updateBasisSetOptions() {

  QStringList basisSets{
      "def2-svp", "def2-tzvp", "6-31G(d,p)", "DGDZVP", "3-21G", "STO-3G",
  };

  basisComboBox->clear();
  if(selectedProgram() != wfn::Program::Xtb) {
    for (const auto &basis : basisSets) {
      basisComboBox->addItem(basis);
    }

    basisComboBox->addItem(customEntry);
  }
}

void WavefunctionCalculationDialog::show() {
  initPrograms(); // wavefunction program availability might have changed so
                  // reinitialise
  QWidget::show();
}

const wfn::Parameters &WavefunctionCalculationDialog::getParameters() const {
  return m_parameters;
}

bool WavefunctionCalculationDialog::isXtbMethod() const {
  return xtb::isXtbMethod(method());
}

bool WavefunctionCalculationDialog::userEditRequested() const {
  return editInputFileCheckBox->isChecked();
}

void WavefunctionCalculationDialog::accept() {
  m_parameters.charge = charge();
  m_parameters.multiplicity = multiplicity();
  m_parameters.program = selectedProgram();
  m_parameters.method = method();
  m_parameters.basis = basis();
  m_parameters.userEditRequested = userEditRequested();

  emit wavefunctionParametersChosen(m_parameters);

  QDialog::accept();
}

wfn::Program WavefunctionCalculationDialog::selectedProgram() const {
  return wfn::programFromName(programComboBox->currentText());
}

QString WavefunctionCalculationDialog::selectedProgramName() const {
  return programComboBox->currentText();
}

QString WavefunctionCalculationDialog::method() const {
  return methodComboBox->currentText();
}

QString WavefunctionCalculationDialog::basis() const {
  return basisComboBox->currentText();
}

void WavefunctionCalculationDialog::setAtomIndices(
    const std::vector<GenericAtomIndex> &idxs) {
  m_parameters.atoms = idxs;
}

const std::vector<GenericAtomIndex> &
WavefunctionCalculationDialog::atomIndices() const {
  return m_parameters.atoms;
}

int WavefunctionCalculationDialog::charge() const {
  return chargeSpinBox->value();
}

void WavefunctionCalculationDialog::setCharge(int charge) {
  chargeSpinBox->setValue(charge);
  m_parameters.charge = charge;
}

int WavefunctionCalculationDialog::multiplicity() const {
  return multiplicitySpinBox->value();
}

void WavefunctionCalculationDialog::setMultiplicity(int mult) {
  multiplicitySpinBox->setValue(mult);
  m_parameters.multiplicity = mult;
}
