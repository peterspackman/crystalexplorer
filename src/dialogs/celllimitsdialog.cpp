#include "celllimitsdialog.h"
#include <QDebug>
#include <QVector3D>

CellLimitsDialog::CellLimitsDialog(QWidget *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags) {
  setupUi(this);
  // this way, the first spin box text is always selected on opening
  // so the user can just num -> tab -> num tab etc.
  aAxisLowerBound->selectAll();
  modeComboBox->insertItems(0, availableSlabGenerationModeOptions());
}

SlabGenerationOptions CellLimitsDialog::currentSettings() {
  SlabGenerationOptions result;
  result.lowerBound =
      QVector3D(aAxisLowerBound->value(), bAxisLowerBound->value(),
                cAxisLowerBound->value());
  result.upperBound =
      QVector3D(aAxisUpperBound->value(), bAxisUpperBound->value(),
                cAxisUpperBound->value());
  result.mode = slabGenerationModeFromString(modeComboBox->currentText());
  return result;
}

void CellLimitsDialog::setLabelText(QString labelText) {
  if (labelText.isEmpty()) {
    label->setVisible(false);
  } else {
    label->setText(labelText);
  }
}
void CellLimitsDialog::shrink() {
  resize(minimumSize());
  adjustSize();
}

SlabGenerationOptions CellLimitsDialog::getSlabGenerationOptions(
    QWidget *parent, const QString &title, const QString &label, bool &ok,
    Qt::WindowFlags flags) {
  CellLimitsDialog dialog(parent, flags);
  dialog.setWindowTitle(title);
  dialog.setLabelText(label);
  dialog.shrink();
  ok = (dialog.exec() == QDialog::Accepted);
  return dialog.currentSettings();
}
