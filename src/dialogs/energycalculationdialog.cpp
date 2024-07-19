#include "energycalculationdialog.h"
#include <ankerl/unordered_dense.h>

EnergyCalculationDialog::EnergyCalculationDialog(QWidget *parent)
    : QDialog(parent) {
  setupUi(this);
  init();
  initConnections();
}

void EnergyCalculationDialog::setChemicalStructure(ChemicalStructure *structure) {
  m_structure = structure;
  handleStructureChange();
}

void EnergyCalculationDialog::init() {
  setModal(true);
  adjustSize();
  editTontoInputFileCheckbox->setChecked(false);
  editWavefunctionInputFileCheckbox->setChecked(false);

  handleStructureChange();

  quantitativeRadioButton->setText("Accurate");
  quantitativeLabel->setText(QString("[CE-1p]"));
  qualitativeRadioButton->setText("Fast");
  qualitativeLabel->setText(QString("[CE-HF]"));

  gfnComboBox->addItem("GFN0-xTB");
  gfnComboBox->addItem("GFN1-xTB");
  gfnComboBox->addItem("GFN2-xTB");
  gfnComboBox->setCurrentIndex(2);

}

void EnergyCalculationDialog::initConnections() {
  connect(this, &EnergyCalculationDialog::accepted,
          this, &EnergyCalculationDialog::validate);
  connect(quantitativeRadioButton, &QRadioButton::toggled,
	  this, &EnergyCalculationDialog::handleModelChange);
  connect(qualitativeRadioButton, &QRadioButton::toggled,
	  this, &EnergyCalculationDialog::handleModelChange);
  connect(userWavefunctionRadioButton, &QRadioButton::toggled,
	  this, &EnergyCalculationDialog::handleModelChange);
}

void EnergyCalculationDialog::showEvent(QShowEvent *) {

    /*
  bool show_experimental = false;
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
			 */

  bool orcaVisible = false;
  bool xtbVisible = false;
  gfnRadioButton->setVisible(xtbVisible);
  gfnComboBox->setVisible(xtbVisible);
  orcaRadioButton->setVisible(orcaVisible);
  orcaLabel->setVisible(orcaVisible);
}

bool EnergyCalculationDialog::methodIsDefined() const {
    return (
	quantitativeRadioButton->isChecked() ||
	qualitativeRadioButton->isChecked() ||
	orcaRadioButton->isChecked() ||
	gfnRadioButton->isChecked()  ||
	(wavefunctionCombobox->currentIndex() == 0)
    );
}

void EnergyCalculationDialog::handleModelChange() {
  m_method = (quantitativeRadioButton->isChecked()) ? "b3lyp" : "hf";
  m_basis = (quantitativeRadioButton->isChecked()) ? "def2-svp" : "3-21g";
  wavefunctionCombobox->setEnabled(userWavefunctionRadioButton->isChecked());
  for(auto &wfn: m_requiredWavefunctions) {
      wfn.method = m_method;
      wfn.basis = m_basis;
  }
}

bool EnergyCalculationDialog::handleStructureChange() {
  m_wavefunctions.clear();
  m_requiredWavefunctions.clear();
  if(!m_structure) return false;
  updateWavefunctionComboBox();


  auto selectedFragments = m_structure->selectedFragments();
  if(selectedFragments.size() > 2) return false;
  const auto &fragments = m_structure->getFragments();
  m_fragmentPairs =  m_structure->findFragmentPairs();

  qDebug() << "Found" << m_fragmentPairs.uniquePairs.size() << "unique pairs";

  m_fragmentPairsToCalculate.clear();
  int asymIndex = fragments[selectedFragments[0]].asymmetricFragmentIndex;
  auto asymTransform = fragments[selectedFragments[0]].asymmetricFragmentTransform;

  ankerl::unordered_dense::set<int> wavefunctionsNeeded;
  wavefunctionsNeeded.insert(asymIndex);

  for(const auto &pair: m_fragmentPairs.uniquePairs) {
      if(pair.a.asymmetricFragmentIndex == asymIndex ||
	 pair.b.asymmetricFragmentIndex == asymIndex) {
	  m_fragmentPairsToCalculate.push_back(pair);
	  qDebug() << pair.a.asymmetricFragmentIndex << pair.b.asymmetricFragmentIndex;
	  wavefunctionsNeeded.insert(pair.a.asymmetricFragmentIndex);
	  wavefunctionsNeeded.insert(pair.b.asymmetricFragmentIndex);
      }
  }

  qDebug() << "Need to calculate" << wavefunctionsNeeded.size() << "wavefunctions";
  qDebug() << "Need to calculate" << m_fragmentPairsToCalculate.size() << "pairs";

  const auto &uniqueFragments = m_structure->symmetryUniqueFragments();
  const auto &uniqueFragmentStates = m_structure->symmetryUniqueFragmentStates();
  for(const auto &uniqueIndex: wavefunctionsNeeded) {
      m_requiredWavefunctions.emplace_back(
          wfn::Parameters{
	      uniqueFragmentStates[uniqueIndex].charge,
	      uniqueFragmentStates[uniqueIndex].multiplicity,
	      m_method,
	      m_basis,
	      m_structure,
	      uniqueFragments[uniqueIndex].atomIndices
	 }
      );
  }
  return false;
}

void EnergyCalculationDialog::validate() {

    if(methodIsDefined() || wavefunctionCombobox->currentIndex() != 0) {
	for(auto &wfn: m_requiredWavefunctions) {
	    wfn.accepted = true;
	}
    } 

    /*
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
  */
  emit energyParametersChosen(pair_energy::EnergyModelParameters{
    "ce-hf",
    m_requiredWavefunctions,
    m_fragmentPairsToCalculate
  });
}

void EnergyCalculationDialog::updateWavefunctionComboBox() {
  wavefunctionCombobox->clear();
  QStringList items{"Generate New Wavefunction"};

  if (m_structure) {
    qDebug() << "Have m_structure: " << m_structure;
    for(const auto *child: m_structure->children()) {
	const MolecularWavefunction * wfn = qobject_cast<const MolecularWavefunction *>(child);
	if(wfn) {
	    items.append(wfn->description());
	}
    }
  }
  wavefunctionCombobox->addItems(items);
}

/*

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
*/
