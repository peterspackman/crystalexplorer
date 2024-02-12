#pragma once
#include "element.h"
#include "ui_elementeditor.h"

class ElementEditor : public QDialog, public Ui::ElementEditor {
  Q_OBJECT

public:
  ElementEditor(QWidget *parent = 0);
  void updateElementComboBox(QStringList sl = QStringList(),
                             QString s = QString());
  void setElement(Element *);

private slots:
  void setElement(QString);
  void getNewElementColor();
  void accept();
  void apply();
  void updateElement();
  void resetCurrentElement();

signals:
  void elementChanged();

private:
  void init();
  void setColorOfColorButton(QColor color);

  QColor _currentColor;
  Element *_element;
};
