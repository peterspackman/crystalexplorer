#pragma once

#include <QDialog>

#include "ui_celllimitsdialog.h"

class CellLimitsDialog : public QDialog, public Ui::CellLimitsDialog {
  Q_OBJECT

public:
  CellLimitsDialog(QWidget *, Qt::WindowFlags);
  QPair<QVector3D, QVector3D> cellLimits();
  void setLabelText(QString);
  void setCellLimitRange(int, int);
  void setCellLimitValues(int, int, int);
  void setCellLimitStep(int);

  static QPair<QVector3D, QVector3D>
  getCellLimits(QWidget *, const QString &, const QString &, int, int, int, int,
                int, int, bool *, Qt::WindowFlags windowFlags = Qt::Dialog);

private:
  void shrink();
};
