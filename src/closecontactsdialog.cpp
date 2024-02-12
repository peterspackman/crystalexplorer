#include "closecontactsdialog.h"
#include "elementdata.h"
#include "globals.h"
#include "settings.h"

#include <QColorDialog>
#include <QDebug>
#include <QtGlobal>

CloseContactDialog::CloseContactDialog(QWidget *parent) : QDialog(parent) {
  setupUi(this);
  init();
  initConnections();
}

void CloseContactDialog::init() {
  Qt::WindowFlags flags = windowFlags();
  setWindowFlags(flags | Qt::WindowStaysOnTopHint);

  setEnabledCloseContact1(false);
  setEnabledCloseContact2(false);
  setEnabledCloseContact3(false);

  setButtonColor(
      hbondColorButton,
      QColor(settings::readSetting(settings::keys::HBOND_COLOR).toString()));
  setButtonColor(
      cc1ColorButton,
      QColor(settings::readSetting(settings::keys::CONTACT1_COLOR).toString()));
  setButtonColor(
      cc2ColorButton,
      QColor(settings::readSetting(settings::keys::CONTACT2_COLOR).toString()));
  setButtonColor(
      cc3ColorButton,
      QColor(settings::readSetting(settings::keys::CONTACT3_COLOR).toString()));
}

void CloseContactDialog::initConnections() {
  connect(buttonBox, &QDialogButtonBox::accepted, this,
          &QDialog::accept); // allows ok button to close dialog

  auto reportSettings = [&](auto) { reportHBondSettingsChanges(); };
  // Connections for widgets on "Hydrogen Bonds" tab
  connect(showHBondsCheckBox, &QAbstractButton::toggled, this,
          &CloseContactDialog::hbondsToggled);
  connect(hbondDistanceCriteriaSpinBox,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          reportSettings);
  connect(hbondDonorComboBox,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          reportSettings);
  connect(hbondAcceptorComboBox,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          reportSettings);
  connect(includeIntraHBondsCheckBox, &QAbstractButton::toggled, this,
          &CloseContactDialog::reportHBondSettingsChanges);
  connect(hbondColorButton, &QAbstractButton::clicked, this,
          &CloseContactDialog::reportHBondColorChange);

  // Connections for widgets on "X•••Y Close Contacts" tab
  connect(cc1EnableCheckBox, &QAbstractButton::toggled, this,
          &CloseContactDialog::updateCloseContact1);
  connect(cc2EnableCheckBox, &QAbstractButton::toggled, this,
          &CloseContactDialog::updateCloseContact2);
  connect(cc3EnableCheckBox, &QAbstractButton::toggled, this,
          &CloseContactDialog::updateCloseContact3);

  // Connections for first close contact
  connect(cc1DonorComboBox, &QComboBox::currentIndexChanged,
          this, &CloseContactDialog::reportCC1SettingsChanges);
  connect(cc1DonorComboBox, &QComboBox::currentIndexChanged,
          this, &CloseContactDialog::updateContact1DistanceCriteria);

  connect(cc1AcceptorComboBox, &QComboBox::currentIndexChanged,
          this, &CloseContactDialog::reportCC1SettingsChanges);
  connect(cc1AcceptorComboBox, &QComboBox::currentIndexChanged,
          this, &CloseContactDialog::updateContact1DistanceCriteria);

  connect(cc1DistanceCriteriaSpinBox, &QDoubleSpinBox::valueChanged,
          this, &CloseContactDialog::reportCC1SettingsChanges);
  connect(cc1ColorButton, &QToolButton::clicked,
          this, &CloseContactDialog::updateCloseContact1Color);

  // Connections for 2nd close contact
  connect(cc2DonorComboBox, &QComboBox::currentIndexChanged,
          this, &CloseContactDialog::reportCC2SettingsChanges);
  connect(cc2DonorComboBox, &QComboBox::currentIndexChanged,
          this, &CloseContactDialog::updateContact2DistanceCriteria);

  connect(cc2AcceptorComboBox, &QComboBox::currentIndexChanged,
          this, &CloseContactDialog::reportCC2SettingsChanges);
  connect(cc2AcceptorComboBox, &QComboBox::currentIndexChanged,
          this, &CloseContactDialog::updateContact2DistanceCriteria);

  connect(cc2DistanceCriteriaSpinBox, &QDoubleSpinBox::valueChanged,
          this, &CloseContactDialog::reportCC2SettingsChanges);
  connect(cc2ColorButton, &QToolButton::clicked,
          this, &CloseContactDialog::updateCloseContact2Color);


  // Connections for 3rd close contact
  connect(cc3DonorComboBox, &QComboBox::currentIndexChanged,
          this, &CloseContactDialog::reportCC3SettingsChanges);
  connect(cc3DonorComboBox, &QComboBox::currentIndexChanged,
          this, &CloseContactDialog::updateContact3DistanceCriteria);

  connect(cc3AcceptorComboBox, &QComboBox::currentIndexChanged,
          this, &CloseContactDialog::reportCC3SettingsChanges);
  connect(cc3AcceptorComboBox, &QComboBox::currentIndexChanged,
          this, &CloseContactDialog::updateContact3DistanceCriteria);

  connect(cc3DistanceCriteriaSpinBox, &QDoubleSpinBox::valueChanged,
          this, &CloseContactDialog::reportCC3SettingsChanges);
  connect(cc3ColorButton, &QToolButton::clicked,
          this, &CloseContactDialog::updateCloseContact3Color);
}

void CloseContactDialog::updateDonorsAndAcceptors(QStringList elements,
                                                  QStringList hydrogenDonors) {
  elements.prepend(ANY_ITEM);
  hydrogenDonors.prepend(ANY_ITEM);

  QStringList hydrogenAcceptors = elements;
  hydrogenAcceptors.removeAll("H");
  updateComboBox(hbondDonorComboBox, hydrogenDonors);
  updateComboBox(hbondAcceptorComboBox, hydrogenAcceptors);
  updateComboBox(cc1DonorComboBox, elements);
  updateComboBox(cc1AcceptorComboBox, elements);
  updateComboBox(cc2DonorComboBox, elements);
  updateComboBox(cc2AcceptorComboBox, elements);
  updateComboBox(cc3DonorComboBox, elements);
  updateComboBox(cc3AcceptorComboBox, elements);
}

void CloseContactDialog::updateComboBox(QComboBox *comboBox,
                                        QStringList items) {
  comboBox->clear();
  comboBox->addItems(items);
}

void CloseContactDialog::setEnabledCloseContact1(bool enable) {
  cc1DonorComboBox->setEnabled(enable);
  cc1AcceptorComboBox->setEnabled(enable);
  cc1DistanceCriteriaSpinBox->setEnabled(enable);
  cc1ColorButton->setEnabled(enable);
  updateContact1DistanceCriteria();
}

void CloseContactDialog::setEnabledCloseContact2(bool enable) {
  cc2DonorComboBox->setEnabled(enable);
  cc2AcceptorComboBox->setEnabled(enable);
  cc2DistanceCriteriaSpinBox->setEnabled(enable);
  cc2ColorButton->setEnabled(enable);
  updateContact2DistanceCriteria();
}

void CloseContactDialog::setEnabledCloseContact3(bool enable) {
  cc3DonorComboBox->setEnabled(enable);
  cc3AcceptorComboBox->setEnabled(enable);
  cc3DistanceCriteriaSpinBox->setEnabled(enable);
  cc3ColorButton->setEnabled(enable);
  updateContact1DistanceCriteria();
}

void CloseContactDialog::updateCloseContact1(bool state) {
  setEnabledCloseContact1(state);
  emit cc1Toggled(state);
}

void CloseContactDialog::updateCloseContact2(bool state) {
  setEnabledCloseContact2(state);
  emit cc2Toggled(state);
}

void CloseContactDialog::updateCloseContact3(bool state) {
  setEnabledCloseContact3(state);
  emit cc3Toggled(state);
}

void CloseContactDialog::updateContact1DistanceCriteria() {
  updateContactDistanceCriteria(cc1DonorComboBox, cc1AcceptorComboBox,
                                cc1DistanceCriteriaSpinBox);
}

void CloseContactDialog::updateContact2DistanceCriteria() {
  updateContactDistanceCriteria(cc2DonorComboBox, cc2AcceptorComboBox,
                                cc2DistanceCriteriaSpinBox);
}

void CloseContactDialog::updateContact3DistanceCriteria() {
  updateContactDistanceCriteria(cc3DonorComboBox, cc3AcceptorComboBox,
                                cc3DistanceCriteriaSpinBox);
}

void CloseContactDialog::updateContactDistanceCriteria(
    QComboBox *donorComboBox, QComboBox *acceptorComboBox,
    QDoubleSpinBox *distanceCriteriaSpinBox) {
  QString donor = donorComboBox->currentText();
  QString acceptor = acceptorComboBox->currentText();

  if (donor == "" || acceptor == "") {
    return;
  }

  float donorVDWRadius;
  float acceptorVDWRadius;

  if (donor == ANY_ITEM) {
    donorVDWRadius = largestVdwRadiusForAllElements();
  } else {
    donorVDWRadius = ElementData::elementFromSymbol(donor)->vdwRadius();
  }

  if (acceptor == ANY_ITEM) {
    acceptorVDWRadius = largestVdwRadiusForAllElements();
  } else {
    acceptorVDWRadius = ElementData::elementFromSymbol(acceptor)->vdwRadius();
  }

  double vdwDistanceCriteria = donorVDWRadius + acceptorVDWRadius;
  distanceCriteriaSpinBox->setValue(vdwDistanceCriteria);

  double maxDistanceCriteria = vdwDistanceCriteria * CLOSECONTACT_FACTOR;
  distanceCriteriaSpinBox->setMaximum(maxDistanceCriteria);
}

double CloseContactDialog::largestVdwRadiusForAllElements() {
  double vdwRadius = 0.0;

  for (int i = 0; i < cc1DonorComboBox->count(); ++i) {
    QString elementSymbol = cc1DonorComboBox->itemText(i);
    if (elementSymbol != ANY_ITEM) {
      vdwRadius = qMax(
          vdwRadius,
          (double)ElementData::elementFromSymbol(elementSymbol)->vdwRadius());
    }
  }
  return vdwRadius;
}

void CloseContactDialog::reportCC1SettingsChanges() {
  QString x = cc1DonorComboBox->currentText();
  QString y = cc1AcceptorComboBox->currentText();
  double distanceCriteria = cc1DistanceCriteriaSpinBox->value();
  emit closeContactsSettingsChanged(CC1_INDEX, x, y, distanceCriteria);
}

void CloseContactDialog::reportCC2SettingsChanges() {
  QString x = cc2DonorComboBox->currentText();
  QString y = cc2AcceptorComboBox->currentText();
  double distanceCriteria = cc2DistanceCriteriaSpinBox->value();
  emit closeContactsSettingsChanged(CC2_INDEX, x, y, distanceCriteria);
}

void CloseContactDialog::reportCC3SettingsChanges() {
  QString x = cc3DonorComboBox->currentText();
  QString y = cc3AcceptorComboBox->currentText();
  double distanceCriteria = cc3DistanceCriteriaSpinBox->value();
  emit closeContactsSettingsChanged(CC3_INDEX, x, y, distanceCriteria);
}

void CloseContactDialog::showDialogWithHydrogenBondTab() {
  tabWidget->setCurrentIndex(HBOND_TAB);
  show();
}

void CloseContactDialog::showDialogWithCloseContactsTab() {
  tabWidget->setCurrentIndex(CLOSE_CONTACTS_TAB);
  show();
}

void CloseContactDialog::setButtonColor(QToolButton *colorButton,
                                        QColor color) {
  QPixmap pixmap = QPixmap(colorButton->iconSize());
  pixmap.fill(color);
  colorButton->setIcon(QIcon(pixmap));
}

QColor CloseContactDialog::getButtonColor(QToolButton *colorButton) {
  return colorButton->icon().pixmap(1, 1).toImage().pixel(0, 0);
}

void CloseContactDialog::reportHBondColorChange() {
  QColor color = QColorDialog::getColor(getButtonColor(hbondColorButton), this);
  if (color.isValid()) {
    setButtonColor(hbondColorButton, color);
    settings::writeSetting(settings::keys::HBOND_COLOR, color.name());
    emit hbondColorChanged();
  }
}

void CloseContactDialog::reportHBondSettingsChanges() {

  QString donor = hbondDonorComboBox->currentText();
  QString acceptor = hbondAcceptorComboBox->currentText();
  double distanceCriteria = hbondDistanceCriteriaSpinBox->value();
  bool includeIntraContacts = includeIntraHBondsCheckBox->checkState();
  emit hbondSettingsChanged(donor, acceptor, distanceCriteria,
                            includeIntraContacts);
}

void CloseContactDialog::updateCloseContactColor(QToolButton *colorButton,
                                                 QString settingsKey) {
  QColor color = QColorDialog::getColor(getButtonColor(colorButton), this);
  if (color.isValid()) {
    setButtonColor(colorButton, color);
    settings::writeSetting(settingsKey, color.name());
    emit closeContactsColorChanged();
  }
}

void CloseContactDialog::updateCloseContact1Color() {
  updateCloseContactColor(cc1ColorButton, settings::keys::CONTACT1_COLOR);
}

void CloseContactDialog::updateCloseContact2Color() {
  updateCloseContactColor(cc2ColorButton, settings::keys::CONTACT2_COLOR);
}

void CloseContactDialog::updateCloseContact3Color() {
  updateCloseContactColor(cc3ColorButton, settings::keys::CONTACT3_COLOR);
}
