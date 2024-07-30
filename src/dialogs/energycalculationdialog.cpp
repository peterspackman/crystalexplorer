#include "energycalculationdialog.h"
#include "settings.h"
#include <ankerl/unordered_dense.h>

EnergyCalculationDialog::EnergyCalculationDialog(QWidget *parent)
    : QDialog(parent) {
  setupUi(this);
  init();
  initConnections();
}

void EnergyCalculationDialog::setChemicalStructure(
    ChemicalStructure *structure) {
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
  connect(this, &EnergyCalculationDialog::accepted, this,
          &EnergyCalculationDialog::validate);
  connect(quantitativeRadioButton, &QRadioButton::toggled, this,
          &EnergyCalculationDialog::handleModelChange);
  connect(qualitativeRadioButton, &QRadioButton::toggled, this,
          &EnergyCalculationDialog::handleModelChange);
  connect(gfnRadioButton, &QRadioButton::toggled, this,
          &EnergyCalculationDialog::handleModelChange);
  connect(gfnComboBox, &QComboBox::currentTextChanged, this,
          &EnergyCalculationDialog::handleModelChange);
  connect(userWavefunctionRadioButton, &QRadioButton::toggled, this,
          &EnergyCalculationDialog::handleModelChange);
}

void EnergyCalculationDialog::showEvent(QShowEvent *) {

  // orca is not visible right now until we implement it
  bool orcaVisible = false &&
                   !settings::readSetting(settings::keys::ORCA_EXECUTABLE)
                        .toString()
                        .isEmpty();
  bool xtbVisible = !settings::readSetting(settings::keys::XTB_EXECUTABLE)
                         .toString()
                         .isEmpty();

  gfnRadioButton->setVisible(xtbVisible);
  gfnComboBox->setVisible(xtbVisible);
  orcaRadioButton->setVisible(orcaVisible);
  orcaLabel->setVisible(orcaVisible);
}

bool EnergyCalculationDialog::methodIsDefined() const {
  return (quantitativeRadioButton->isChecked() ||
          qualitativeRadioButton->isChecked() || orcaRadioButton->isChecked() ||
          gfnRadioButton->isChecked() ||
          (wavefunctionCombobox->currentIndex() == 0));
}

void EnergyCalculationDialog::handleModelChange() {
  m_method = (quantitativeRadioButton->isChecked()) ? "b3lyp" : "hf";
  m_basis = (quantitativeRadioButton->isChecked()) ? "def2-svp" : "3-21g";

  if (gfnRadioButton->isChecked()) {
    m_method = gfnComboBox->currentText();
    m_basis = "";
  }

  wavefunctionCombobox->setEnabled(userWavefunctionRadioButton->isChecked());
  for (auto &wfn : m_requiredWavefunctions) {
    wfn.method = m_method;
    wfn.basis = m_basis;
  }
}

bool EnergyCalculationDialog::handleStructureChange() {
  m_wavefunctions.clear();
  m_requiredWavefunctions.clear();
  if (!m_structure)
    return false;
  updateWavefunctionComboBox();

  auto selectedFragments = m_structure->selectedFragments();
  if (selectedFragments.size() > 2 || selectedFragments.size() < 1)
    return false;

  const auto &fragments = m_structure->getFragments();
  const int keyFragmentIndex = selectedFragments[0];
  m_fragmentPairs = m_structure->findFragmentPairs(keyFragmentIndex);
  m_fragmentPairsToCalculate.clear();
  const auto &keyFragment = fragments[keyFragmentIndex];
  int asymIndex = keyFragment.asymmetricFragmentIndex;
  auto asymTransform = keyFragment.asymmetricFragmentTransform;

  ankerl::unordered_dense::set<int> wavefunctionsNeeded;
  wavefunctionsNeeded.insert(asymIndex);

  auto match = [](const Fragment &a, const Fragment &b) {
    return (a.asymmetricFragmentIndex == b.asymmetricFragmentIndex) &&
           (a.atomIndices == b.atomIndices);
  };
  if (selectedFragments.size() == 2) {
    // TODO improve efficiency here
    const auto &keyFragment2 = fragments[selectedFragments[1]];
    for (const auto &[pair, uniqueIndex] : m_fragmentPairs.pairs[keyFragmentIndex]) {
      if (match(pair.b, keyFragment2)) {
        m_fragmentPairsToCalculate.push_back(m_fragmentPairs.uniquePairs[uniqueIndex]);
        wavefunctionsNeeded.insert(pair.a.asymmetricFragmentIndex);
        wavefunctionsNeeded.insert(pair.b.asymmetricFragmentIndex);
      }
    }
  } else {
    for (const auto &pair : m_fragmentPairs.uniquePairs) {
      if (match(pair.a, keyFragment) || match(pair.b, keyFragment)) {
        m_fragmentPairsToCalculate.push_back(pair);
        wavefunctionsNeeded.insert(pair.a.asymmetricFragmentIndex);
        wavefunctionsNeeded.insert(pair.b.asymmetricFragmentIndex);
      }
    }
  }

  const auto &uniqueFragments = m_structure->symmetryUniqueFragments();
  const auto &uniqueFragmentStates =
      m_structure->symmetryUniqueFragmentStates();
  for (const auto &uniqueIndex : wavefunctionsNeeded) {
    m_requiredWavefunctions.emplace_back(wfn::Parameters{
        uniqueFragmentStates[uniqueIndex].charge,
        uniqueFragmentStates[uniqueIndex].multiplicity, m_method, m_basis,
        m_structure, uniqueFragments[uniqueIndex].atomIndices});
  }
  return false;
}

QString EnergyCalculationDialog::selectedEnergyModel() const {
  // TODO add support for more energy models
  if (gfnRadioButton->isChecked()) {
    return gfnComboBox->currentText();
  }
  if (quantitativeRadioButton->isChecked())
    return "ce-1p";
  if (qualitativeRadioButton->isChecked())
    return "ce-hf";
  // use the CE-1p model with whatever we throw at it
  return "ce-1p";
}

void EnergyCalculationDialog::validate() {

  if (methodIsDefined() || wavefunctionCombobox->currentIndex() != 0) {
    for (auto &wfn : m_requiredWavefunctions) {
      wfn.accepted = true;
    }
  }
  emit energyParametersChosen(pair_energy::EnergyModelParameters{
      selectedEnergyModel(), m_requiredWavefunctions,
      m_fragmentPairsToCalculate});
}

void EnergyCalculationDialog::updateWavefunctionComboBox() {
  wavefunctionCombobox->clear();
  QStringList items{"Generate New Wavefunction"};

  if (m_structure) {
    for (const auto *child : m_structure->children()) {
      const MolecularWavefunction *wfn =
          qobject_cast<const MolecularWavefunction *>(child);
      if (wfn) {
        items.append(wfn->description());
      }
    }
  }
  wavefunctionCombobox->addItems(items);
}
