#include "periodictabledialog.h"
#include "elementdata.h"
#include "ui_periodictabledialog.h"
#include <QColorDialog>
#include <QDebug>

bool isColorDark(const QColor &color) {
  double luminence =
      0.299 * color.redF() + 0.587 * color.greenF() + 0.114 * color.blueF();
  return luminence < 0.5;
}

void updateButtonColors(QPushButton *b, const QColor &color) {
  QString style = "QPushButton { background-color: %1; color: %2; }";

  QColor fg = isColorDark(color) ? Qt::white : Qt::black;

  b->setStyleSheet(style.arg(color.name()).arg(fg.name()));
  b->update();
}

PeriodicTableDialog::PeriodicTableDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::PeriodicTableDialog) {
  ui->setupUi(this);

  connect(ui->colorButton, &QPushButton::clicked, this,
          &PeriodicTableDialog::getNewElementColor);
  connect(ui->applyButton, &QPushButton::clicked, this,
          &PeriodicTableDialog::apply);
  connect(ui->resetButton, &QPushButton::clicked, this,
          &PeriodicTableDialog::resetCurrentElement);
  reset();
  connect(ui->elementButtons,
          QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this,
          &PeriodicTableDialog::elementButtonClicked);
}

void PeriodicTableDialog::resetElements() { reset(); }

void PeriodicTableDialog::reset() {
  setElement("H");
  for (auto *button : ui->elementButtons->buttons()) {
    QPushButton *b = qobject_cast<QPushButton *>(button);
    QString elementSymbol = b->text();
    m_buttons.insert(elementSymbol, b);
    Element *element = ElementData::elementFromSymbol(elementSymbol);
    if (element != nullptr) {
      updateButtonColors(b, element->color());
    }
  }
}

void PeriodicTableDialog::elementButtonClicked(QAbstractButton *button) {
  setElement(button->text());
  button->setChecked(true);
}

void PeriodicTableDialog::updateSelectedElement(const QString &elementSymbol) {
  ui->elementLabel->setText(elementSymbol);
  setElement(ui->elementLabel->text());
}

void PeriodicTableDialog::setElement(const QString &elementSymbol) {
  if (!elementSymbol.isEmpty()) {
    setElement(ElementData::elementFromSymbol(elementSymbol));
  }
}

void PeriodicTableDialog::setElement(Element *element) {
  if (element == nullptr)
    return;
  m_element = element;

  ui->covRadiusSpinBox->setValue(element->covRadius());
  ui->vdwRadiusSpinBox->setValue(element->vdwRadius());

  ui->elementLabel->setText(m_element->capitalizedSymbol());
  ui->elementNameLabel->setText(m_element->name());
  m_currentColor = m_element->color();
  setColorOfColorButton(m_currentColor);
}

void PeriodicTableDialog::getNewElementColor() {
  QColor color = QColorDialog::getColor(m_currentColor);
  if (color.isValid()) {
    setColorOfColorButton(color);
  }
}

void PeriodicTableDialog::setColorOfColorButton(const QColor &color) {
  m_currentColor = color;
  QPixmap pixmap = QPixmap(ui->colorButton->iconSize());
  pixmap.fill(color);
  ui->colorButton->setIcon(QIcon(pixmap));
  QString elementSymbol = m_element->capitalizedSymbol();
  auto kv = m_buttons.find(elementSymbol);
  if (kv != m_buttons.end()) {
    QPushButton *b = kv.value();
    updateButtonColors(b, color);
  }
}

void PeriodicTableDialog::resetCurrentElement() {
  if (m_element == nullptr)
    return;
  ElementData::resetElement(m_element->symbol());
  setElement(m_element);
  emit elementChanged();
}

void PeriodicTableDialog::accept() {
  updateElement();
  emit elementChanged();
  QDialog::accept();
}

void PeriodicTableDialog::apply() {
  updateElement();
  emit elementChanged();
}

void PeriodicTableDialog::updateElement() {
  m_element->setCovRadius(ui->covRadiusSpinBox->value());
  m_element->setVdwRadius(ui->vdwRadiusSpinBox->value());
  m_element->setColor(m_currentColor);
}

PeriodicTableDialog::~PeriodicTableDialog() { delete ui; }
