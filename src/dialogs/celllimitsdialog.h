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

  static SlabGenerationOptions
  getSlabGenerationOptions(QWidget *, const QString &, const QString &, bool &ok, Qt::WindowFlags windowFlags = Qt::Dialog);

private:
  void shrink();
};
