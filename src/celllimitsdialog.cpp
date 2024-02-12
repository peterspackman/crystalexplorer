#include "celllimitsdialog.h"
#include <QDebug>
#include <QVector3D>

CellLimitsDialog::CellLimitsDialog(QWidget *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags) {
  setupUi(this);
  // this way, the first spin box text is always selected on opening
  // so the user can just num -> tab -> num tab etc.
  this->aAxisDoubleSpinBox->selectAll();
}

QPair<QVector3D, QVector3D> CellLimitsDialog::cellLimits() {
  return qMakePair(QVector3D(0, 0, 0), QVector3D(aAxisDoubleSpinBox->value(),
                                                 bAxisDoubleSpinBox->value(),
                                                 cAxisDoubleSpinBox->value()));
}

void CellLimitsDialog::setLabelText(QString labelText) {
  if (labelText.isEmpty()) {
    label->setVisible(false);
  } else {
    label->setText(labelText);
  }
}

void CellLimitsDialog::setCellLimitRange(int min, int max) {
  aAxisDoubleSpinBox->setMinimum(min);
  aAxisDoubleSpinBox->setMaximum(max);

  bAxisDoubleSpinBox->setMinimum(min);
  bAxisDoubleSpinBox->setMaximum(max);

  cAxisDoubleSpinBox->setMinimum(min);
  cAxisDoubleSpinBox->setMaximum(max);
}

void CellLimitsDialog::setCellLimitValues(int aVal, int bVal, int cVal) {
  aAxisDoubleSpinBox->setValue(aVal);
  bAxisDoubleSpinBox->setValue(bVal);
  cAxisDoubleSpinBox->setValue(cVal);
}

void CellLimitsDialog::setCellLimitStep(int step) {
  aAxisDoubleSpinBox->setSingleStep(step);
  bAxisDoubleSpinBox->setSingleStep(step);
  cAxisDoubleSpinBox->setSingleStep(step);
}

void CellLimitsDialog::shrink() {
  resize(minimumSize());
  adjustSize();
}

QPair<QVector3D, QVector3D>
CellLimitsDialog::getCellLimits(QWidget *parent, const QString &title,
                                const QString &label, int aVal, int bVal,
                                int cVal, int min, int max, int step, bool *ok,
                                Qt::WindowFlags flags) {
  CellLimitsDialog dialog(parent, flags);
  dialog.setWindowTitle(title);
  dialog.setLabelText(label);
  dialog.setCellLimitRange(min, max);
  dialog.setCellLimitValues(aVal, bVal, cVal);
  dialog.setCellLimitStep(step);
  dialog.shrink();

  int ret = dialog.exec();

  if (ok) {
    *ok = !!ret;
  }

  if (ret) {
    return dialog.cellLimits();
  }
  return qMakePair(QVector3D(0, 0, 0), QVector3D(0, 0, 0));
}
