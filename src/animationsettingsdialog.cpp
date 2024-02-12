/*
 *  animationsettingsdialog.cpp
 *  CrystalExplorer
 *
 *  Created by Mike Turner on 19/07/11.
 *  Copyright 2011 BBCS. All rights reserved.
 *
 */

#include "animationsettingsdialog.h"

#include <QDebug>

AnimationSettingsDialog::AnimationSettingsDialog() {
  setupUi(this);

  connect(minorXSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &AnimationSettingsDialog::userSetValues);
  connect(minorYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &AnimationSettingsDialog::userSetValues);
  connect(minorZSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &AnimationSettingsDialog::userSetValues);
  connect(majorXSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &AnimationSettingsDialog::userSetValues);
  connect(majorYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &AnimationSettingsDialog::userSetValues);
  connect(majorZSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &AnimationSettingsDialog::userSetValues);

  connect(overallSpeedSlider, &QAbstractSlider::valueChanged, this,
          &AnimationSettingsDialog::userSetSpeed);
  connect(minorSpeedSlider, &QAbstractSlider::valueChanged, this,
          &AnimationSettingsDialog::userSetSpeed);
  connect(majorSpeedSlider, &QAbstractSlider::valueChanged, this,
          &AnimationSettingsDialog::userSetSpeed);

  connect(cancelButton, &QAbstractButton::clicked, this,
          &AnimationSettingsDialog::cancel);
  connect(startStopButton, &QAbstractButton::clicked, this,
          &AnimationSettingsDialog::startStop);
  connect(detailedSettingsCheckBox, &QAbstractButton::toggled, this,
          &AnimationSettingsDialog::detailedSettingsToggled);
  connect(majorAxisCheckBox, &QAbstractButton::toggled, this,
          &AnimationSettingsDialog::majorAxisSettingsToggled);

  detailedSettingsCheckBox->setChecked(false);
  majorAxisCheckBox->setChecked(false);
  majorAxisCheckBox->hide();
  minorAxisGroupBox->hide();
  majorAxisGroupBox->hide();

  startStopButton->setStyleSheet("color: rgb(0,150,0)");

  resize(minimumSize());
  adjustSize();

  reset();
}

void AnimationSettingsDialog::detailedSettingsToggled(bool checked) {
  if (checked) {
    minorAxisGroupBox->show();
    majorAxisCheckBox->show();
    if (majorAxisCheckBox->isChecked()) {
      majorAxisGroupBox->show();
    }
  } else {
    minorAxisGroupBox->hide();
    majorAxisCheckBox->hide();
    majorAxisGroupBox->hide();
    resize(minimumSize());
    adjustSize();
  }
}

void AnimationSettingsDialog::majorAxisSettingsToggled(bool checked) {
  if (checked) {
    majorAxisGroupBox->show();
  } else {
    majorAxisGroupBox->hide();
    resize(minimumSize());
    adjustSize();
  }
}

void AnimationSettingsDialog::userSetSpeed() { userSetValues(); }

void AnimationSettingsDialog::userSetValues(double) {
  float minorX = minorXSpinBox->value();
  float minorY = minorYSpinBox->value();
  float minorZ = minorZSpinBox->value();
  float majorX = majorXSpinBox->value();
  float majorY = majorYSpinBox->value();
  float majorZ = majorZSpinBox->value();

  float overallSpeed = overallSpeedSlider->value() / 30.0;
  float minorSpeed = minorSpeedSlider->value() / 10.0 * overallSpeed;
  float majorSpeed = majorSpeedSlider->value() / 10.0 * overallSpeed;

  emit animationSettingsChanged(minorX, minorY, minorZ, minorSpeed, majorX,
                                majorY, majorZ, majorSpeed);
}

void AnimationSettingsDialog::reset() {
  minorXSpinBox->setValue(1.0);
  minorYSpinBox->setValue(0);
  minorZSpinBox->setValue(0);
  majorXSpinBox->setValue(0);
  majorYSpinBox->setValue(0);
  majorZSpinBox->setValue(1.0);
  minorSpeedSlider->setValue(30.0);
  majorSpeedSlider->setValue(10.0);
}

void AnimationSettingsDialog::startStop(bool start) {
  userSetValues();
  emit animationToggled(start);
  if (start) {
    startStopButton->setStyleSheet("color: rgb(255,0,0)");
    startStopButton->setText("Stop");
  } else {
    startStopButton->setStyleSheet("color: rgb(0,150,0)");
    startStopButton->setText("Start");
  }
}
void AnimationSettingsDialog::cancel() {
  if (startStopButton->isChecked()) {
    startStopButton->click();
  } else {
    emit animationToggled(false);
  }
  QDialog::reject();
}
