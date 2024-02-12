#include <QtDebug>

#include "packingdialog.h"

PackingDialog::PackingDialog(QWidget *parent) : QDialog(parent) {
  setupUi(this);
}

void PackingDialog::accept() {
  emit packingParametersChosen(getPackingLimits());
  QDialog::accept();
}

QList<float> PackingDialog::validatedLimits(float minLimit, float maxLimit) {
  if (minLimit >= maxLimit) {
    maxLimit += CELL_FRACTION_MIN_STEP;
  }
  return QList<float>() << minLimit << maxLimit;
}

QList<float> PackingDialog::getPackingLimits() {
  QList<float> limits;

  limits << validatedLimits(aAxisMinSpinBox->value(), aAxisMaxSpinBox->value());
  limits << validatedLimits(bAxisMinSpinBox->value(), bAxisMaxSpinBox->value());
  limits << validatedLimits(cAxisMinSpinBox->value(), cAxisMaxSpinBox->value());

  return limits;
}