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
  bool orcaVisible =
      false && !settings::readSetting(settings::keys::ORCA_EXECUTABLE)
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
          gfnRadioButton->isChecked());
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
  qDebug() << "Handle structure change";
  // TODO check if energy model is symmetric i.e allowInversions should be set
  m_wavefunctions.clear();
  m_requiredWavefunctions.clear();
  if (!m_structure)
    return false;
  updateWavefunctionComboBox();

  auto selectedFragments = m_structure->selectedFragments();
  if (selectedFragments.size() > 2 || selectedFragments.size() < 1)
    return false;

  const auto &fragments = m_structure->getFragments();
  const auto keyFragmentIndex = selectedFragments[0];
  qDebug() << "Key fragment" << keyFragmentIndex;
  FragmentPairSettings pairSettings;
  pairSettings.keyFragment = keyFragmentIndex;
  m_fragmentPairs = m_structure->findFragmentPairs(pairSettings);
  m_fragmentPairsToCalculate.clear();
  const auto &keyFragment = fragments.at(keyFragmentIndex);
  const auto asymIndex = keyFragment.asymmetricFragmentIndex;
  auto asymTransform = keyFragment.asymmetricFragmentTransform;

  FragmentIndexSet wavefunctionsNeeded;
  wavefunctionsNeeded.insert(asymIndex);

  qDebug() << "Unique pairs: " << m_fragmentPairs.uniquePairs.size();

  auto match = [](const Fragment &a, const Fragment &b) {
    return (a.asymmetricFragmentIndex == b.asymmetricFragmentIndex);
  };
  if (selectedFragments.size() == 2) {
    // TODO improve efficiency here
    qDebug() << "Selected fragments: " << selectedFragments[0] << selectedFragments[1];
    const auto &keyFragment2 = fragments.at(selectedFragments[1]);
    qDebug() << "Keyfragment2" << keyFragment2;
    for(const auto &[idx, mpairs]: m_fragmentPairs.pairs) {
      qDebug() << "Key: " << idx;
    }
    for (const auto &[pair, uniqueIndex] :
         m_fragmentPairs.pairs.at(keyFragmentIndex)) {
      if (match(pair.b, keyFragment2)) {
        m_fragmentPairsToCalculate.push_back(
            m_fragmentPairs.uniquePairs[uniqueIndex]);
        wavefunctionsNeeded.insert(pair.a.asymmetricFragmentIndex);
        wavefunctionsNeeded.insert(pair.b.asymmetricFragmentIndex);
      }
    }
  } else {
    for (const auto &pair : m_fragmentPairs.uniquePairs) {
      qDebug() << "Unique pair" << pair.index;
      qDebug() << "Unique pair (asym)" << pair.a.asymmetricFragmentIndex << pair.b.asymmetricFragmentIndex;
      if (match(pair.a, keyFragment) || match(pair.b, keyFragment)) {
        qDebug() << "Will be calculatedpair" << pair.index;
        m_fragmentPairsToCalculate.push_back(pair);
        wavefunctionsNeeded.insert(pair.a.asymmetricFragmentIndex);
        wavefunctionsNeeded.insert(pair.b.asymmetricFragmentIndex);
      }
    }
  }

  const auto &uniqueFragments = m_structure->symmetryUniqueFragments();
  for (const auto &uniqueIndex : wavefunctionsNeeded) {
    const auto &uniqueFrag = uniqueFragments.at(uniqueIndex);
    const auto &state = uniqueFrag.state;
    wfn::Parameters params;
    params.charge = state.charge;
    params.multiplicity = state.multiplicity;
    params.method = m_method;
    params.basis = m_basis;
    params.structure = m_structure;
    params.atoms = uniqueFrag.atomIndices;
    m_requiredWavefunctions.push_back(params);
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
