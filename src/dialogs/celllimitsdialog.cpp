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
  updateControlsForPeriodicity();
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

void CellLimitsDialog::setPeriodicityMode(int periodicDimensions) {
  m_periodicDimensions = periodicDimensions;
  updateControlsForPeriodicity();
}

void CellLimitsDialog::updateControlsForPeriodicity() {
  bool showCAxis = (m_periodicDimensions >= 3);
  
  // Show/hide c-axis controls based on periodicity
  cAxisLabel->setVisible(showCAxis);
  cAxisLowerBound->setVisible(showCAxis);
  cAxisUpperBound->setVisible(showCAxis);
  
  // Update window title and label text for different dimensionalities
  if (m_periodicDimensions == 2) {
    setWindowTitle("Show Multiple Surface Cells");
    // Set default values for hidden c-axis (no expansion in z direction)
    cAxisLowerBound->setValue(0.0);
    cAxisUpperBound->setValue(0.0);
  } else if (m_periodicDimensions == 1) {
    setWindowTitle("Show Multiple Wire Cells");
    bAxisLabel->setVisible(false);
    bAxisLowerBound->setVisible(false);
    bAxisUpperBound->setVisible(false);
    // Set defaults for hidden axes
    bAxisLowerBound->setValue(0.0);
    bAxisUpperBound->setValue(0.0);
    cAxisLowerBound->setValue(0.0);
    cAxisUpperBound->setValue(0.0);
  } else if (m_periodicDimensions == 0) {
    setWindowTitle("Show Cluster");
    // Hide all periodic controls for 0D clusters
    aAxisLabel->setVisible(false);
    aAxisLowerBound->setVisible(false);
    aAxisUpperBound->setVisible(false);
    bAxisLabel->setVisible(false);
    bAxisLowerBound->setVisible(false);
    bAxisUpperBound->setVisible(false);
    // Set all to zero for no expansion
    aAxisLowerBound->setValue(0.0);
    aAxisUpperBound->setValue(0.0);
    bAxisLowerBound->setValue(0.0);
    bAxisUpperBound->setValue(0.0);
    cAxisLowerBound->setValue(0.0);
    cAxisUpperBound->setValue(0.0);
  }
}

SlabGenerationOptions CellLimitsDialog::getSlabGenerationOptions(
    QWidget *parent, const QString &title, const QString &label, bool &ok,
    Qt::WindowFlags flags) {
  return getSlabGenerationOptions(parent, title, label, 3, ok, flags);
}

SlabGenerationOptions CellLimitsDialog::getSlabGenerationOptions(
    QWidget *parent, const QString &title, const QString &label, int periodicDimensions, bool &ok,
    Qt::WindowFlags flags) {
  CellLimitsDialog dialog(parent, flags);
  dialog.setPeriodicityMode(periodicDimensions);
  dialog.setWindowTitle(title);
  dialog.setLabelText(label);
  dialog.shrink();
  ok = (dialog.exec() == QDialog::Accepted);
  return dialog.currentSettings();
}
