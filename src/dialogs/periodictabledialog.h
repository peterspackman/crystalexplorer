#pragma once

#include "element.h"
#include <QDialog>
#include <QMap>
#include <QToolButton>

namespace Ui {
class PeriodicTableDialog;
}

class PeriodicTableDialog : public QDialog {
  Q_OBJECT

public slots:
  void resetElements();
private slots:
  void setElement(const QString &);
  void getNewElementColor();
  void accept();
  void apply();
  void updateElement();
  void resetCurrentElement();
  void elementButtonClicked(QAbstractButton *);
  void reset();

signals:
  void elementChanged();

public:
  explicit PeriodicTableDialog(QWidget *parent = nullptr);
  ~PeriodicTableDialog();
  void updateSelectedElement(const QString & = "H");
  void setElement(Element *);

private:
  void setColorOfColorButton(const QColor &color);

  Ui::PeriodicTableDialog *ui;
  QMap<QString, QPushButton *> m_buttons;
  QColor m_currentColor;
  Element *m_element{nullptr};
};
