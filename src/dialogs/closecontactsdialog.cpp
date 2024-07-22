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

  setButtonColor(
      hbondColorButton,
      QColor(settings::readSetting(settings::keys::HBOND_COLOR).toString()));

  useVdwBasedCriteria(vdwCriteriaCheckBox->checkState());
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

  connect(vdwCriteriaCheckBox, &QCheckBox::stateChanged, this,
          &CloseContactDialog::useVdwBasedCriteria);

  connect(hbondDonorComboBox,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          reportSettings);
  connect(hbondAcceptorComboBox,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          reportSettings);

  connect(distanceMinSpinBox,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          reportSettings);

  connect(distanceMaxSpinBox,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          reportSettings);

  connect(angleMaxSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, reportSettings);

  connect(angleMinSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, reportSettings);

  connect(includeIntraHBondsCheckBox, &QAbstractButton::toggled, this,
          &CloseContactDialog::reportHBondSettingsChanges);
  connect(hbondColorButton, &QAbstractButton::clicked, this,
          &CloseContactDialog::reportHBondColorChange);

  connect(addContactPushButton, &QPushButton::clicked, closeContactsWidget, &CloseContactCriteriaWidget::addRow);
  connect(closeContactsWidget, &CloseContactCriteriaWidget::closeContactsSettingsChanged,
          this, &CloseContactDialog::closeContactsSettingsChanged);
}

void CloseContactDialog::updateDonorsAndAcceptors(QStringList elements,
                                                  QStringList hydrogenDonors) {
  elements.prepend(ANY_ITEM);
  hydrogenDonors.prepend(ANY_ITEM);

  QStringList hydrogenAcceptors = elements;
  hydrogenAcceptors.removeAll("H");
  updateComboBox(hbondDonorComboBox, hydrogenDonors);
  updateComboBox(hbondAcceptorComboBox, hydrogenAcceptors);
  closeContactsWidget->updateElements(elements);
}

void CloseContactDialog::updateComboBox(QComboBox *comboBox,
                                        QStringList items) {
  comboBox->clear();
  comboBox->addItems(items);
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
    emit hbondCriteriaChanged(currentHBondCriteria());
  }
}

void CloseContactDialog::reportHBondSettingsChanges() {

  emit hbondCriteriaChanged(currentHBondCriteria());
}

void setLayoutHidden(QHBoxLayout *layout, bool enabled) {
  for (int i = 0; i < layout->count(); ++i) {
    QWidget *widget = layout->itemAt(i)->widget();
    if (widget) {
      widget->setVisible(enabled);
    }
  }
}

void CloseContactDialog::useVdwBasedCriteria(bool vdw) {
  setLayoutHidden(vdwCriteriaLayout, vdw);
  setLayoutHidden(distanceCriteriaLayout, !vdw);
  reportHBondSettingsChanges();
}

HBondCriteria CloseContactDialog::currentHBondCriteria() {
  HBondCriteria criteria;

  QString donor = hbondDonorComboBox->currentText();
  QString acceptor = hbondAcceptorComboBox->currentText();
  if (donor != ANY_ITEM) {
    auto *element = ElementData::elementFromSymbol(donor);
    if (element) {
      criteria.donors.insert(element->number());
    }
  }

  if (acceptor != ANY_ITEM) {
    auto *element = ElementData::elementFromSymbol(acceptor);
    if (element) {
      criteria.acceptors.insert(element->number());
    }
  }

  criteria.color = getButtonColor(hbondColorButton);
  criteria.minAngle = angleMinSpinBox->value();
  criteria.maxAngle = angleMaxSpinBox->value();
  criteria.minDistance = distanceMinSpinBox->value();
  criteria.maxDistance = distanceMaxSpinBox->value();
  criteria.includeIntra = includeIntraHBondsCheckBox->checkState();
  criteria.vdwOffset = hbondDistanceCriteriaSpinBox->value();
  criteria.vdwCriteria = vdwCriteriaCheckBox->checkState();

  return criteria;
}
