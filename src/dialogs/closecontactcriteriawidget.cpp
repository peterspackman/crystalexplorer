#include "closecontactcriteriawidget.h"
#include "elementdata.h"
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>

/*
void CloseContactCriteriaWidget::updateContactDistanceCriteria(
    QComboBox *donorComboBox, QComboBox *acceptorComboBox,
    QDoubleSpinBox *distanceCriteriaSpinBox) {
  QString donor = donorComboBox->currentText();
  QString acceptor = acceptorComboBox->currentText();

  if (donor == "" || acceptor == "") {
    return;
  }

  float donorVDWRadius;
  float acceptorVDWRadius;

  if (donor == cx::globals::anyItemLabel) {
    donorVDWRadius = largestVdwRadiusForAllElements();
  } else {
    donorVDWRadius = ElementData::elementFromSymbol(donor)->vdwRadius();
  }

  if (acceptor == cx::globals::anyItemLabel) {
    acceptorVDWRadius = largestVdwRadiusForAllElements();
  } else {
    acceptorVDWRadius = ElementData::elementFromSymbol(acceptor)->vdwRadius();
  }

  double vdwDistanceCriteria = donorVDWRadius + acceptorVDWRadius;
  distanceCriteriaSpinBox->setValue(vdwDistanceCriteria);

  double maxDistanceCriteria = vdwDistanceCriteria * CLOSECONTACT_FACTOR;
  distanceCriteriaSpinBox->setMaximum(maxDistanceCriteria);
}
*/

double CloseContactCriteriaWidget::largestVdwRadiusForAllElements() const {
  double vdwRadius = 0.0;

  for (int i = 0; i < m_elements.size(); ++i) {
    QString elementSymbol = m_elements[i];
    if (elementSymbol != cx::globals::anyItemLabel) {
      vdwRadius = std::max(
          vdwRadius,
          static_cast<double>(
              ElementData::elementFromSymbol(elementSymbol)->vdwRadius()));
    }
  }
  return vdwRadius;
}

void CloseContactCriteriaWidget::setButtonColor(QToolButton *colorButton,
                                                QColor color) {
  QPixmap pixmap = QPixmap(colorButton->iconSize());
  pixmap.fill(color);
  colorButton->setIcon(QIcon(pixmap));
}

QColor CloseContactCriteriaWidget::getButtonColor(QToolButton *colorButton) {
  return colorButton->icon().pixmap(1, 1).toImage().pixel(0, 0);
}

CloseContactCriteriaWidget::CloseContactCriteriaWidget(QWidget *parent)
    : QWidget(parent), m_colorMap(ColorMapName::Turbo, 0.0, 10.0) {
  m_layout = new QGridLayout(this);

  addHeader();
}

void CloseContactCriteriaWidget::addHeader() {

  QLabel *enabledLabel = new QLabel(QString("Enabled"), this);
  m_layout->addWidget(enabledLabel, 0, 0);

  QLabel *xLabel = new QLabel(QString("X"), this);
  m_layout->addWidget(xLabel, 0, 1);

  QLabel *yLabel = new QLabel(QString("Y"), this);
  m_layout->addWidget(yLabel, 0, 2);

  QLabel *distanceLabel = new QLabel(QString("X•••Y distance"), this);
  m_layout->addWidget(distanceLabel, 0, 3);

  QLabel *colorLabel = new QLabel(QString("Color"), this);
  m_layout->addWidget(colorLabel, 0, 4);
}

void CloseContactCriteriaWidget::updateElements(
    const QList<QString> &elements) {
  m_elements = elements;
  m_vdwMax = largestVdwRadiusForAllElements();
  // CLEAR COMBOBOXES,
  // ADD ELEMENT ITEMS (POSSIBLE PREPEND cx::globals::anyItemLabel)
}

CloseContactCriteria CloseContactCriteriaWidget::getCriteria(int row) {
  CloseContactCriteria criteria;

  if (row < 0 || row >= m_layout->rowCount()) {
    return criteria;
  }

  QLayoutItem *donorItem = m_layout->itemAtPosition(row, 1);
  QLayoutItem *acceptorItem = m_layout->itemAtPosition(row, 2);

  if (donorItem && donorItem->widget()) {
    QComboBox *donorBox = qobject_cast<QComboBox *>(donorItem->widget());
    QString donor = donorBox ? donorBox->currentText() : QString();
    if (donor != cx::globals::anyItemLabel) {
      auto *element = ElementData::elementFromSymbol(donor);
      if (element) {
        criteria.donors.insert(element->number());
      }
    }
  }

  if (acceptorItem && acceptorItem->widget()) {
    QComboBox *acceptorBox = qobject_cast<QComboBox *>(acceptorItem->widget());
    QString acceptor = acceptorBox ? acceptorBox->currentText() : QString();
    if (acceptor != cx::globals::anyItemLabel) {
      auto *element = ElementData::elementFromSymbol(acceptor);
      if (element) {
        criteria.acceptors.insert(element->number());
      }
    }
  }

  QLayoutItem *checkboxItem = m_layout->itemAtPosition(row, 0);
  if (checkboxItem && checkboxItem->widget()) {
    QCheckBox *checkbox = qobject_cast<QCheckBox *>(checkboxItem->widget());
    criteria.show = checkbox ? checkbox->isChecked() : false;
  }

  QLayoutItem *distanceItem = m_layout->itemAtPosition(row, 3);
  if (distanceItem && distanceItem->widget()) {
    QDoubleSpinBox *distanceBox =
        qobject_cast<QDoubleSpinBox *>(distanceItem->widget());
    criteria.maxDistance = distanceBox ? distanceBox->value() : 0.0;
  }

  QLayoutItem *colorItem = m_layout->itemAtPosition(row, 4);
  if (colorItem && colorItem->widget()) {
    QToolButton *colorButton = qobject_cast<QToolButton *>(colorItem->widget());
    if (colorButton) {
      criteria.color = getButtonColor(colorButton);
    }
  }

  return criteria;
}

int CloseContactCriteriaWidget::count() const {
  return m_layout->rowCount() - 1;
}


void CloseContactCriteriaWidget::criteriaChanged(int row) {
  emit closeContactsSettingsChanged(row, getCriteria(row));
}

void CloseContactCriteriaWidget::addRow() {
  int row = m_layout->rowCount();

  QCheckBox *checkbox = new QCheckBox(this);
  checkbox->setChecked(true);
  QComboBox *donorDropdown = new QComboBox(this);
  QComboBox *acceptorDropdown = new QComboBox(this);
  QDoubleSpinBox *distanceCriteria = new QDoubleSpinBox(this);
  QToolButton *colorButton = new QToolButton(this);
  distanceCriteria->setValue(m_vdwMax * 2);
  distanceCriteria->setSingleStep(0.05);

  setButtonColor(colorButton, m_colorMap(row - 1.0));

  // Setup your dropdowns, etc. here
  donorDropdown->addItems(m_elements);
  acceptorDropdown->addItems(m_elements);

  connect(checkbox, &QCheckBox::stateChanged,
          [this, row]() { criteriaChanged(row); });

  connect(donorDropdown, &QComboBox::currentTextChanged,
          [this, row]() { criteriaChanged(row); });

  connect(acceptorDropdown, &QComboBox::currentTextChanged,
          [this, row]() { criteriaChanged(row); });

  connect(distanceCriteria, &QDoubleSpinBox::valueChanged,
          [this, row]() { criteriaChanged(row); });

  connect(colorButton, &QToolButton::clicked, this, [this, colorButton, row]() {
    QColor color = QColorDialog::getColor(Qt::white, this, "Choose Color");
    if (color.isValid()) {
      setButtonColor(colorButton, color);
      emit closeContactsSettingsChanged(row, getCriteria(row));
    }
  });

  m_layout->addWidget(checkbox, row, 0);
  m_layout->addWidget(donorDropdown, row, 1);
  m_layout->addWidget(acceptorDropdown, row, 2);
  m_layout->addWidget(distanceCriteria, row, 3);
  m_layout->addWidget(colorButton, row, 4);

  criteriaChanged(row);
}
