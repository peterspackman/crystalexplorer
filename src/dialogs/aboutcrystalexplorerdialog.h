#pragma once

#include "ui_aboutcrystalexplorerdialog.h"
#include <QDialog>

class AboutCrystalExplorerDialog : public QDialog,
                                   public Ui::AboutCrystalExplorerDialog {
  Q_OBJECT

public:
  explicit AboutCrystalExplorerDialog(QWidget *parent = nullptr);
};
