#include "energycalculationdialog.h"
#include <ankerl/unordered_dense.h>
#include "settings.h"

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
  connect(userWavefunctionRadioButton, &QRadioButton::toggled, this,
          &EnergyCalculationDialog::handleModelChange);
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
                       */
  bool xtbVisible = !settings::readSetting(settings::keys::XTB_EXECUTABLE)
                         .toString()
                         .isEmpty();

  bool orcaVisible = false;
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

  qDebug() << "Found" << m_fragmentPairs.uniquePairs.size() << "unique pairs";

  m_fragmentPairsToCalculate.clear();
  const auto &keyFragment = fragments[keyFragmentIndex];
  int asymIndex = keyFragment.asymmetricFragmentIndex;
  auto asymTransform = keyFragment.asymmetricFragmentTransform;

  ankerl::unordered_dense::set<int> wavefunctionsNeeded;
  wavefunctionsNeeded.insert(asymIndex);

  for (const auto &pair : m_fragmentPairs.uniquePairs) {
    const bool lhs = ((pair.a.asymmetricFragmentIndex == asymIndex) &&
                      (pair.a.atomIndices == keyFragment.atomIndices));
    const bool rhs = ((pair.b.asymmetricFragmentIndex == asymIndex) &&
                      (pair.b.atomIndices == keyFragment.atomIndices));
    if (lhs || rhs) {
      m_fragmentPairsToCalculate.push_back(pair);
      qDebug() << pair.a.asymmetricFragmentIndex
               << pair.b.asymmetricFragmentIndex << pair.centroidDistance << pair.symmetry;
      wavefunctionsNeeded.insert(pair.a.asymmetricFragmentIndex);
      wavefunctionsNeeded.insert(pair.b.asymmetricFragmentIndex);
    }
  }

  qDebug() << "Need to calculate" << wavefunctionsNeeded.size()
           << "wavefunctions";
  qDebug() << "Need to calculate" << m_fragmentPairsToCalculate.size()
           << "pairs";

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
  if(quantitativeRadioButton->isChecked()) return "ce-1p";
  if(qualitativeRadioButton->isChecked()) return "ce-hf";
  return "ce-1p";
}

void EnergyCalculationDialog::validate() {

  if (methodIsDefined() || wavefunctionCombobox->currentIndex() != 0) {
    for (auto &wfn : m_requiredWavefunctions) {
      wfn.accepted = true;
    }
  }
  emit energyParametersChosen(pair_energy::EnergyModelParameters{
      selectedEnergyModel(), m_requiredWavefunctions, m_fragmentPairsToCalculate});
}

void EnergyCalculationDialog::updateWavefunctionComboBox() {
  wavefunctionCombobox->clear();
  QStringList items{"Generate New Wavefunction"};

  if (m_structure) {
    qDebug() << "Have m_structure: " << m_structure;
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
