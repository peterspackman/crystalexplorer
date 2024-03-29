#include "energycalculationdialog.h"
#include "energydescription.h"
#include "globals.h" // only NEW_WAVEFUNCTION_ITEM
#include "settings.h"

EnergyCalculationDialog::EnergyCalculationDialog(QWidget *parent)
    : QDialog(parent) {
  setupUi(this);
  init();
  initConnections();
}

void EnergyCalculationDialog::setCrystal(DeprecatedCrystal *crystal) {
  m_crystal = crystal;
  qDebug() << "Calling setCrystal";
  updateWavefunctionComboBox();
}

void EnergyCalculationDialog::init() {
  setModal(true);
  adjustSize();
  editTontoInputFileCheckbox->setChecked(false);
  editWavefunctionInputFileCheckbox->setChecked(false);
  _waitingOnWavefunction = false;
  qDebug() << "Initialization";
  updateWavefunctionComboBox();
  _numberOfCalculations = 0;
  _currentCalculationIndex = 1;

  quantitativeRadioButton->setText("Accurate");
  quantitativeLabel->setText(QString("[%1]").arg(
      EnergyDescription::quantitativeEnergyModelDescription()));
  qualitativeRadioButton->setText("Fast");
  qualitativeLabel->setText(QString("[%1]").arg(
      EnergyDescription::qualitativeEnergyModelDescription()));

  gfnComboBox->addItem("GFN0-xTB");
  gfnComboBox->addItem("GFN1-xTB");
  gfnComboBox->addItem("GFN2-xTB");
  gfnComboBox->setCurrentIndex(2);

  _chargeA = 0;
  _chargeB = 0;
  _multiplicityA = 1;
  _multiplicityB = 1;
}

void EnergyCalculationDialog::initConnections() {
  connect(this, &EnergyCalculationDialog::accepted,
          this, &EnergyCalculationDialog::validate);
  connect(quantitativeRadioButton, SIGNAL(toggled(bool)), this,
          SLOT(modelChemistryChanged()));
  connect(qualitativeRadioButton, SIGNAL(toggled(bool)), this,
          SLOT(modelChemistryChanged()));
  connect(userWavefunctionRadioButton, SIGNAL(toggled(bool)), this,
          SLOT(modelChemistryChanged()));
}

void EnergyCalculationDialog::showEvent(QShowEvent *) {

  bool show_experimental =
      settings::readSetting(
          settings::keys::ENABLE_EXPERIMENTAL_INTERACTION_ENERGIES)
          .toBool();
  qDebug() << "Show event" << show_experimental;
  bool orcaVisible = show_experimental &&
                     !settings::readSetting(settings::keys::ORCA_EXECUTABLE)
                          .toString()
                          .isEmpty();
  bool xtbVisible = show_experimental &&
                    !settings::readSetting(settings::keys::XTB_EXECUTABLE)
                         .toString()
                         .isEmpty();
  gfnRadioButton->setVisible(xtbVisible);
  gfnComboBox->setVisible(xtbVisible);
  orcaRadioButton->setVisible(orcaVisible);
  orcaLabel->setVisible(orcaVisible);
}

QVector<AtomId> EnergyCalculationDialog::atomsForFragmentA() {
  return _atomsForCalculation.mid(0, _atomGroups[0]);
}

QVector<AtomId> EnergyCalculationDialog::atomsForFragmentB() {
  return _atomsForCalculation.mid(_atomGroups[0]);
}

bool EnergyCalculationDialog::needWavefunctionCalculationDialog() {
  return !(quantitativeRadioButton->isChecked() ||
           qualitativeRadioButton->isChecked() ||
           orcaRadioButton->isChecked() || gfnRadioButton->isChecked());
}

bool EnergyCalculationDialog::findMatchingWavefunctions() {
  Q_ASSERT(m_crystal != nullptr);
  m_waveFunctions.clear();
  auto wfns_a =
      m_crystal->transformableWavefunctionsForAtoms(atomsForFragmentA());
  auto wfns_b =
      m_crystal->transformableWavefunctionsForAtoms(atomsForFragmentB());
  m_foundA = false;
  for (const auto &tw : wfns_a) {
    if (tw.first.jobParameters().theory == m_method &&
        tw.first.jobParameters().basisset == m_basis) {
      qDebug() << "Found matching wavefunction for A, atoms = "
               << _atomGroups[0];
      m_waveFunctions.push_back(tw);
      m_foundA = true;
    }
  }
  m_foundB = false;
  for (const auto &tw : wfns_b) {
    if (tw.first.jobParameters().theory == m_method &&
        tw.first.jobParameters().basisset == m_basis) {
      qDebug() << "Found matching wavefunction for B" << _atomGroups[1];
      m_waveFunctions.push_back(tw);
      m_foundB = true;
    }
  }
  return m_foundA && m_foundB;
}

bool EnergyCalculationDialog::findMatchingMonomerEnergies() {
  Q_ASSERT(m_crystal != nullptr);
  m_monomerEnergies.clear();
  JobParameters ja = createMonomerEnergyCalculationJobParameters(
      atomsForFragmentA(), _chargeA, _multiplicityA);
  JobParameters jb = createMonomerEnergyCalculationJobParameters(
      atomsForFragmentB(), _chargeB, _multiplicityB);
  auto m_a = m_crystal->monomerEnergyMatchingParameters(ja);
  auto m_b = m_crystal->monomerEnergyMatchingParameters(jb);
  m_foundA = false;
  if (m_a) {
    m_foundA = true;
    m_monomerEnergies.push_back(*m_a);
  }

  m_foundB = false;
  if (m_b) {
    m_foundB = true;
    m_monomerEnergies.push_back(*m_b);
  }
  return m_foundA && m_foundB;
}

// to chain the calculations on our part.
void EnergyCalculationDialog::validate() {
  Q_ASSERT(m_crystal);
  if (!needWavefunctionCalculationDialog()) {
    if (orcaRadioButton->isChecked()) {
      m_method = Method::DLPNO;
      m_basis = BasisSet::CC_PVDZ;
    } else if (gfnRadioButton->isChecked()) {
      const QVector<Method> gfnMethods{Method::GFN0xTB, Method::GFN1xTB,
                                       Method::GFN2xTB};
      m_method = gfnMethods[gfnComboBox->currentIndex()];
    } else {
      m_method = (quantitativeRadioButton->isChecked())
                     ? EnergyDescription::quantitativeEnergyModelTheory()
                     : EnergyDescription::qualitativeEnergyModelTheory();
      m_basis = (quantitativeRadioButton->isChecked())
                    ? EnergyDescription::quantitativeEnergyModelBasisset()
                    : EnergyDescription::qualitativeEnergyModelBasisset();
    }
  } else {
    if (wavefunctionCombobox->currentIndex() == 0) {
      // need to sort out this logic.
      _waitingOnWavefunction = true;
      emit requireWavefunction(atomsForFragmentA(), _chargeA, _multiplicityA);
      return;
    } else {
      QString description = wavefunctionCombobox->currentText();
      for (const auto &wfn : m_crystal->wavefunctions()) {
        if (wfn.description() == description) {
          qDebug() << "Found matching wavefunction for description";
          m_method = wfn.jobParameters().theory;
          m_basis = wfn.jobParameters().basisset;
          break;
        }
      }
    }
  }
  calculate();
}

void EnergyCalculationDialog::setMethodAndBasis(Method method, BasisSet basis) {
  m_method = method;
  m_basis = basis;
}

void EnergyCalculationDialog::setAtomsForCalculation(
    const QVector<AtomId> &atomsForFragmentA,
    const QVector<AtomId> &atomsForFragmentB) {
  _atomsForCalculation.clear();
  _atomsForCalculation.append(atomsForFragmentA);
  _atomsForCalculation.append(atomsForFragmentB);

  _atomGroups.clear();
  _atomGroups.append(atomsForFragmentA.size());
  _atomGroups.append(atomsForFragmentB.size());
}

void EnergyCalculationDialog::updateWavefunctionComboBox() {
  wavefunctionCombobox->clear();
  qDebug() << "In update wavefunctionCombobox, index: " << wavefunctionCombobox->currentIndex();
  QStringList items{"Generate New Wavefunction"};

  if (m_crystal) {
    qDebug() << "Have m_crystal: " << m_crystal;
    for (const auto &wfn : m_crystal->wavefunctions()) {
      items.append(wfn.description());
    }
  }
  wavefunctionCombobox->addItems(items);
}

bool EnergyCalculationDialog::calculatedEnergiesForAllPairs() {
  return _atomsForRemainingFragments.size() == 0;
}

void EnergyCalculationDialog::setAtomsForRemainingFragments(
    const QVector<QVector<AtomId>> &fragments) {
  _atomsForRemainingFragments = fragments;
  _numberOfCalculations =
      fragments.size() +
      1; // since fragments is remaining we have to +1 for the first fragment
  _currentCalculationIndex = 0;
}

QVector<AtomId> EnergyCalculationDialog::nextFragmentAtoms() {
  return _atomsForRemainingFragments.takeFirst();
}

void EnergyCalculationDialog::calculateDLPNO() {
  JobParameters jobParams;
  // have Wavefunctions!
  jobParams.jobType = JobType::pairEnergy;
  jobParams.editInputFile =
      (editTontoInputFileCheckbox->checkState() == Qt::Checked);
  jobParams.atoms = _atomsForCalculation;
  jobParams.atomGroups = _atomGroups;
  jobParams.theory = m_method;
  jobParams.basisset = m_basis;

  jobParams.maxStep = _numberOfCalculations;
  _currentCalculationIndex++;
  jobParams.step = _currentCalculationIndex;
  emit energyParametersChosen(jobParams, {}); // along with wavefunction
}

void EnergyCalculationDialog::calculateGFN() {
  _waitingOnWavefunction = false;
  if (!findMatchingMonomerEnergies()) {
    JobParameters jobParams;
    if (!m_foundA) {
      qDebug() << "Calculating new monomer energy for A";
      jobParams = createMonomerEnergyCalculationJobParameters(
          atomsForFragmentA(), _chargeA, _multiplicityA);
    } else {
      qDebug() << "Calculating new monomer energy for B";
      jobParams = createMonomerEnergyCalculationJobParameters(
          atomsForFragmentB(), _chargeB, _multiplicityB);
    }
    _waitingOnWavefunction = true;
    emit requireMonomerEnergy(jobParams);
    return;
  }

  JobParameters jobParams;
  // have Wavefunctions!
  jobParams.jobType = JobType::pairEnergy;
  jobParams.editInputFile =
      (editTontoInputFileCheckbox->checkState() == Qt::Checked);
  jobParams.atoms = _atomsForCalculation;
  jobParams.atomGroups = _atomGroups;
  // Not required for the energy calculation but used when producing a results
  // table of interaction energies
  jobParams.theory = m_method;
  jobParams.basisset = m_basis;

  jobParams.maxStep = _numberOfCalculations;
  _currentCalculationIndex++;
  jobParams.step = _currentCalculationIndex;
  for (const auto &m : m_monomerEnergies) {
    for (auto kv = m.energies.constKeyValueBegin();
         kv != m.energies.constKeyValueEnd(); kv++) {
      if (!jobParams.monomerEnergySum.contains(kv->first)) {
        jobParams.monomerEnergySum[kv->first] = kv->second;
      } else {
        jobParams.monomerEnergySum[kv->first] += kv->second;
      }
    }
  }

  emit energyParametersChosen(jobParams, {}); // along with wavefunction
}

void EnergyCalculationDialog::calculate() {
  qDebug() << "In calculate";
  if (m_method == Method::DLPNO) {
    calculateDLPNO();
    return;
  } else if (m_method == Method::GFN0xTB) {
    m_basis = BasisSet::TightBinding;
    calculateGFN();
    return;
  } else if (m_method == Method::GFN1xTB) {
    m_basis = BasisSet::TightBinding;
    calculateGFN();
    return;
  } else if (m_method == Method::GFN2xTB) {
    m_basis = BasisSet::TightBinding;
    calculateGFN();
    return;
  }

  _waitingOnWavefunction = false;
  if (!findMatchingWavefunctions()) {
    JobParameters jobParams;
    if (!m_foundA) {
      qDebug() << "Calculating new wavefunction for A";
      jobParams = createWavefunctionCalculationJobParameters(
          atomsForFragmentA(), _chargeA, _multiplicityA);
    } else {
      qDebug() << "Calculating new wavefunction for B";
      jobParams = createWavefunctionCalculationJobParameters(
          atomsForFragmentB(), _chargeB, _multiplicityB);
    }
    _waitingOnWavefunction = true;
    emit requireSpecifiedWavefunction(jobParams);
    return;
  }

  JobParameters jobParams;
  // have Wavefunctions!
  jobParams.jobType = JobType::pairEnergy;
  jobParams.editInputFile =
      (editTontoInputFileCheckbox->checkState() == Qt::Checked);
  jobParams.atoms = _atomsForCalculation;
  jobParams.atomGroups = _atomGroups;
  // Not required for the energy calculation but used when producing a results
  // table of interaction energies
  jobParams.theory = m_method;
  jobParams.basisset = m_basis;

  jobParams.wavefunctionTransforms = {m_waveFunctions[0].second,
                                      m_waveFunctions[1].second};

  jobParams.maxStep = _numberOfCalculations;
  _currentCalculationIndex++;
  jobParams.step = _currentCalculationIndex;

  QVector<Wavefunction> wavefunctions{m_waveFunctions[0].first,
                                      m_waveFunctions[1].first};
  qDebug() << (wavefunctions[0].wavefunctionFile() ==
               wavefunctions[1].wavefunctionFile());
  emit energyParametersChosen(jobParams,
                              wavefunctions); // along with wavefunction
}

void EnergyCalculationDialog::modelChemistryChanged() {
  wavefunctionCombobox->setEnabled(userWavefunctionRadioButton->isChecked());
}

JobParameters
EnergyCalculationDialog::createWavefunctionCalculationJobParameters(
    const QVector<AtomId> &atoms, int charge, int multiplicity) const {
  JobParameters jobParams;
  auto wf_source = JobParameters::prefferedWavefunctionSource();
  jobParams.jobType = JobType::wavefunction;
  jobParams.program = wf_source;
  jobParams.editInputFile =
      (editWavefunctionInputFileCheckbox->checkState() == Qt::Checked);
  jobParams.theory = m_method;
  jobParams.basisset = m_basis;
  jobParams.charge = charge;
  jobParams.multiplicity = multiplicity;
  jobParams.atoms = atoms;

  return jobParams;
}

JobParameters
EnergyCalculationDialog::createMonomerEnergyCalculationJobParameters(
    const QVector<AtomId> &atoms, int charge, int multiplicity) const {
  JobParameters jobParams;
  jobParams.jobType = JobType::monomerEnergy;
  jobParams.program = ExternalProgram::XTB;
  jobParams.editInputFile =
      (editWavefunctionInputFileCheckbox->checkState() == Qt::Checked);
  jobParams.theory = m_method;
  jobParams.basisset = m_basis;
  jobParams.charge = charge;
  jobParams.multiplicity = multiplicity;
  jobParams.atoms = atoms;

  return jobParams;
}
