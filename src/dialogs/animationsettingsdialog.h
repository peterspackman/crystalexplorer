#pragma once
#include <QDialog>

#include "ui_animationsettingsdialog.h"

class AnimationSettingsDialog : public QDialog,
                                public Ui::AnimationSettingsDialog {
  Q_OBJECT

public:
  AnimationSettingsDialog();
  void reset();

signals:
  void animationSettingsChanged(double, double, double, double, double, double,
                                double, double);
  void animationToggled(bool);

private slots:
  void cancel();
  void startStop(bool);
  void userSetSpeed();
  void userSetValues(double = 0.0); // ignored, but necessary for overload
  void detailedSettingsToggled(bool);
  void majorAxisSettingsToggled(bool);
};
