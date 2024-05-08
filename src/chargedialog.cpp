#include "chargedialog.h"
#include "ui_chargedialog.h"

#include <QDebug>
#include <QMessageBox>

ChargeDialog::ChargeDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ChargeDialog) {
  ui->setupUi(this);
  init();
  initConnections();
}

ChargeDialog::~ChargeDialog() { delete ui; }

void ChargeDialog::init() {}

void ChargeDialog::initConnections() {
  connect(ui->yesRadioButton, &QAbstractButton::toggled, this,
          &ChargeDialog::yesRadioButtonToggled);
}

void ChargeDialog::accept() {
  if (hasFragmentStates() && !chargeIsBalanced()) {
    QString question =
        "Charges are not balanced.\n\nDo you want to continue anyway?";
    QMessageBox::StandardButton reply =
        QMessageBox::question(this, "Setting Fragment Charges", question,
                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) {
      return; // don't accept dialog
    }
  }

  QDialog::accept();
}

void ChargeDialog::cleanupWidgets() {
  for (auto spinBox : _chargeSpinBoxes) {
    delete spinBox;
  }
  _chargeSpinBoxes.clear();

  for (auto spinBox : _multiplicitySpinBoxes) {
    delete spinBox;
  }
  _multiplicitySpinBoxes.clear();

  for (auto label : _labels) {
    delete label;
  }
  _labels.clear();

  for (auto layout : _layouts) {
    delete layout;
  }
  _layouts.clear();

  QLayout *layoutToDelete = ui->chargesGroupBox->layout();
  if (layoutToDelete != nullptr) {
    delete layoutToDelete;
  }
}

void ChargeDialog::createWidgets(
    const QStringList &fragmentString,
    const std::vector<ChemicalStructure::FragmentState> &fragmentStates) {
  QVBoxLayout *boxLayout = new QVBoxLayout();

  for (int i = 0; i < fragmentString.size(); ++i) {

    QSpinBox *chargeSpinBox = new QSpinBox();
    chargeSpinBox->setRange(-MAXMINCHARGE, MAXMINCHARGE);
    chargeSpinBox->setSingleStep(1);
    chargeSpinBox->setValue(fragmentStates[i].charge);
    chargeSpinBox->setToolTip("Fragment charge");
    _chargeSpinBoxes.push_back(chargeSpinBox);

    QSpinBox *multiplicitySpinBox = new QSpinBox();
    multiplicitySpinBox->setRange(1, 12); // Need to factor this out
    multiplicitySpinBox->setSingleStep(1);
    multiplicitySpinBox->setValue(fragmentStates[i].multiplicity);
    multiplicitySpinBox->setToolTip("Fragment multiplicity");
    _multiplicitySpinBoxes.push_back(multiplicitySpinBox);

    QLabel *label = new QLabel(fragmentString[i]);
    _labels.push_back(label);

    QHBoxLayout *layout = new QHBoxLayout();
    _layouts.push_back(layout);

    layout->addWidget(label);
    layout->addWidget(chargeSpinBox);
    layout->addWidget(multiplicitySpinBox);

    boxLayout->addLayout(layout);
  }

  QString info = "If you choose the wrong charges, they can be changed using "
                 "the menu option: <i>Actions → Fragment Charges</i>";
  QLabel *label = new QLabel(info);
  label->setTextFormat(Qt::RichText);
  label->setWordWrap(true);
  QFont font = label->font();
  font.setPointSize(11);
  label->setFont(font);
  boxLayout->addWidget(label);
  _labels.push_back(label); // Keep track of label so it can be cleaned up in
                            // ChargDialog::cleanupChargeWidgets

  ui->chargesGroupBox->setLayout(boxLayout);

  registerConnectionsForSpinBoxes();
}

void ChargeDialog::registerConnectionsForSpinBoxes() {
  for (const auto spinBox : std::as_const(_chargeSpinBoxes)) {
    connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &ChargeDialog::chargeSpinBoxChanged);
  }
}

void ChargeDialog::populate(ChemicalStructure *structure) {
    if(!structure) return;

    const auto fragments = structure->symmetryUniqueFragments();
    bool statesFromUser{false};
    const auto states = structure->symmetryUniqueFragmentStates();
    bool hasChargedFragments = std::any_of(states.begin(), states.end(),
	    [](const ChemicalStructure::FragmentState &state) { return state.charge != 0; });


    QStringList fragmentStrings;
    for(const auto &frag: fragments) {
	fragmentStrings << structure->formulaSumForAtoms(frag, true);
    }

    setFragmentInformation(fragmentStrings, states, hasChargedFragments);

}
void ChargeDialog::setFragmentInformation(
    const QStringList &fragmentString,
    const std::vector<ChemicalStructure::FragmentState> &fragmentStates,
    bool hasChargedFragments) {
    qDebug() << fragmentString << fragmentStates.size();
  Q_ASSERT((fragmentString.size() == fragmentStates.size()));

  cleanupWidgets();
  createWidgets(fragmentString, fragmentStates);

  // Take into account the size of the above added widgets
  adjustSize();

  if (hasChargedFragments) { // Force toggling to show charges section
    ui->noRadioButton->setChecked(true);
    ui->yesRadioButton->setChecked(true);
  } else { // Force toggling to hide the charges section
    ui->yesRadioButton->setChecked(true);
    ui->noRadioButton->setChecked(true);
  }
}

void ChargeDialog::yesRadioButtonToggled(bool state) {
  ui->chargesGroupBox->setVisible(state);
  adjustSize();
}

bool ChargeDialog::hasFragmentStates() {
  return ui->yesRadioButton->isChecked();
}

std::vector<ChemicalStructure::FragmentState>
ChargeDialog::getFragmentStates() {
  std::vector<ChemicalStructure::FragmentState> result;
  for (int i = 0;
       i < std::min(_chargeSpinBoxes.size(), _multiplicitySpinBoxes.size());
       i++) {
    result.push_back(
        ChemicalStructure::FragmentState{_chargeSpinBoxes[i]->value(), _multiplicitySpinBoxes[i]->value()});
  }
  return result;
}

void ChargeDialog::chargeSpinBoxChanged(int value) {
  // If there are only two spinboxes then there must be only two fragments
  // If the spinbox is set to value for one fragment then the other should be
  // set to -value to keep the charge balanced.
  if (CONSTRAIN_CHARGES && _chargeSpinBoxes.size() == 2) {
    int otherSpinBoxIndex = (_chargeSpinBoxes[0] == QObject::sender()) ? 1 : 0;

    _chargeSpinBoxes[otherSpinBoxIndex]->blockSignals(
        true); // Prevent
               // chargeSpinBoxChanged
               // being called again
               // when we call setValue
    _chargeSpinBoxes[otherSpinBoxIndex]->setValue(-value);
    _chargeSpinBoxes[otherSpinBoxIndex]->blockSignals(false);
  }
}

int ChargeDialog::totalCharge() {
  Q_ASSERT(_chargeSpinBoxes.size() >
           0); // Shouldn't be calling this if there's no charge to sum up

  int total = 0;
  for (const QSpinBox *spinBox : std::as_const(_chargeSpinBoxes)) {
    total += spinBox->value();
  }
  return total;
}

bool ChargeDialog::chargeIsBalanced() { return totalCharge() == 0; }
