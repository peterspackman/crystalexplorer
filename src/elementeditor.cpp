#include <QColor>
#include <QColorDialog>
#include <QIcon>
#include <QPixmap>
#include <QtDebug>

#include "elementdata.h"
#include "elementeditor.h"

ElementEditor::ElementEditor(QWidget *parent) : QDialog(parent) { init(); }

void ElementEditor::init() {
  setupUi(this);

  updateElementComboBox();

  connect(elementComboBox, SIGNAL(currentIndexChanged(QString)), this,
          SLOT(setElement(QString)));
  connect(colorButton, SIGNAL(clicked(bool)), this, SLOT(getNewElementColor()));
  connect(applyButton, SIGNAL(clicked()), this, SLOT(apply()));
  connect(resetButton, SIGNAL(clicked()), this, SLOT(resetCurrentElement()));
}

void ElementEditor::updateElementComboBox(QStringList elementSymbols,
                                          QString currentElementSymbol) {
  elementComboBox->clear();

  if (elementSymbols.isEmpty()) {
    elementComboBox->addItems(ElementData::elementSymbols());
  } else {
    elementComboBox->addItems(elementSymbols);
  }

  Q_ASSERT(elementComboBox->count() != 0);

  if (!currentElementSymbol.isNull()) {
    elementComboBox->setCurrentIndex(
        elementSymbols.indexOf(currentElementSymbol));
  } else {
    setElement(elementComboBox->currentText());
  }
}

void ElementEditor::setElement(QString elementSymbol) {
  if (!elementSymbol.isEmpty()) {
    setElement(ElementData::elementFromSymbol(elementSymbol));
  }
}

void ElementEditor::getNewElementColor() {
  QColor color = QColorDialog::getColor(_currentColor);
  if (color.isValid()) {
    setColorOfColorButton(color);
  }
}

void ElementEditor::setElement(Element *element) {
  _element = element;

  covRadiusSpinBox->setValue(element->covRadius());
  vdwRadiusSpinBox->setValue(element->vdwRadius());

  _currentColor = _element->color();
  setColorOfColorButton(_currentColor);
}

void ElementEditor::setColorOfColorButton(QColor color) {
  _currentColor = color;
  QPixmap pixmap = QPixmap(colorButton->iconSize());
  pixmap.fill(color);
  colorButton->setIcon(QIcon(pixmap));
}

void ElementEditor::resetCurrentElement() {
  ElementData::resetElement(_element->symbol());
  setElement(_element);
  emit elementChanged();
}

void ElementEditor::accept() {
  updateElement();
  emit elementChanged();
  QDialog::accept();
}

void ElementEditor::apply() {
  updateElement();
  emit elementChanged();
}

void ElementEditor::updateElement() {
  _element->setCovRadius(covRadiusSpinBox->value());
  _element->setVdwRadius(vdwRadiusSpinBox->value());
  _element->setColor(_currentColor);
}
