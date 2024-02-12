#pragma once
#include <QDialog>

#include "ui_packingdialog.h"

inline const float CELL_FRACTION_MIN_STEP = 0.1f;

class PackingDialog : public QDialog, public Ui::PackingDialog {
  Q_OBJECT

public:
  PackingDialog(QWidget *parent = 0);
  void accept();

signals:
  void packingParametersChosen(QList<float>);

private:
  QList<float> validatedLimits(float, float);
  QList<float> getPackingLimits();
};
