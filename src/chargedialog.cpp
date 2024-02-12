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
  if (hasChargesAndMultiplicities() && !chargeIsBalanced()) {
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
    const std::vector<ChargeMultiplicityPair> &fragmentCM) {
  QVBoxLayout *boxLayout = new QVBoxLayout();

  for (int i = 0; i < fragmentString.size(); ++i) {

    QSpinBox *chargeSpinBox = new QSpinBox();
    chargeSpinBox->setRange(-MAXMINCHARGE, MAXMINCHARGE);
    chargeSpinBox->setSingleStep(1);
    chargeSpinBox->setValue(fragmentCM[i].charge);
    chargeSpinBox->setToolTip("Fragment charge");
    _chargeSpinBoxes.push_back(chargeSpinBox);

    QSpinBox *multiplicitySpinBox = new QSpinBox();
    multiplicitySpinBox->setRange(1, 12); // Need to factor this out
    multiplicitySpinBox->setSingleStep(1);
    multiplicitySpinBox->setValue(fragmentCM[i].multiplicity);
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

void ChargeDialog::setChargeMultiplicityInfo(
    const QStringList &fragmentString,
    const std::vector<ChargeMultiplicityPair> &fragmentCM,
    bool hasChargedFragments) {
  Q_ASSERT((fragmentString.size() == fragmentCM.size()));

  cleanupWidgets();
  createWidgets(fragmentString, fragmentCM);

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

bool ChargeDialog::hasChargesAndMultiplicities() {
  return ui->yesRadioButton->isChecked();
}

std::vector<ChargeMultiplicityPair>
ChargeDialog::getChargesAndMultiplicities() {
  std::vector<ChargeMultiplicityPair> result;
  for (int i = 0;
       i < std::min(_chargeSpinBoxes.size(), _multiplicitySpinBoxes.size());
       i++) {
    result.push_back(
        {_chargeSpinBoxes[i]->value(), _multiplicitySpinBoxes[i]->value()});
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
