#pragma once

#include <QDialog>

#include "ui_celllimitsdialog.h"
#include "slab_options.h"

class CellLimitsDialog : public QDialog, public Ui::CellLimitsDialog {
  Q_OBJECT

public:
  CellLimitsDialog(QWidget *, Qt::WindowFlags);
  SlabGenerationOptions currentSettings();
  void setLabelText(QString);
  void setPeriodicityMode(int periodicDimensions);

  static SlabGenerationOptions
  getSlabGenerationOptions(QWidget *, const QString &, const QString &, bool &ok, Qt::WindowFlags windowFlags = Qt::Dialog);
  
  static SlabGenerationOptions
  getSlabGenerationOptions(QWidget *, const QString &, const QString &, int periodicDimensions, bool &ok, Qt::WindowFlags windowFlags = Qt::Dialog);

private:
  void shrink();
  void updateControlsForPeriodicity();
  int m_periodicDimensions{3}; // Default to 3D
};
