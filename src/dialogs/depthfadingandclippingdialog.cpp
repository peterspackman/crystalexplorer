#include "depthfadingandclippingdialog.h"
#include "globals.h"
#include "settings.h"

DepthFadingAndClippingDialog::DepthFadingAndClippingDialog(QWidget *parent)
    : QDialog(parent) {
  setupUi(this);
  init();
  initConnections();
}

void DepthFadingAndClippingDialog::init() {
  enableDepthFogCheckBox->setChecked(
      settings::readSetting(settings::keys::DEPTH_FOG_ENABLED).toBool());
  fogDensitySlider->setValue(static_cast<int>(
      settings::readSetting(settings::keys::DEPTH_FOG_DENSITY).toFloat() *
      FADING_SCALE_FACTOR));
  fogOffsetSlider->setValue(static_cast<int>(
      settings::readSetting(settings::keys::DEPTH_FOG_OFFSET).toFloat() *
      FOG_OFFSET_SCALE_FACTOR));
  initClippingSlider();
}

void DepthFadingAndClippingDialog::initConnections() {
  connect(buttonBox, &QDialogButtonBox::accepted, this,
          &DepthFadingAndClippingDialog::accept);

  // Widgets on 'depth fading' Tab
  connect(enableDepthFogCheckBox, &QCheckBox::toggled, this,
          &DepthFadingAndClippingDialog::reportDepthFadingSettings);
  connect(fogDensitySlider, &QSlider::sliderMoved, this,
          &DepthFadingAndClippingDialog::reportDepthFadingSettings);
  connect(fogOffsetSlider, &QSlider::sliderMoved, this,
          &DepthFadingAndClippingDialog::reportDepthFadingSettings);
  // Widgets on 'clipping' Tab
  connect(frontClippingSlider, &QSlider::valueChanged, this,
          &DepthFadingAndClippingDialog::reportClippingSettings);
  connect(resetClippingButton, &QPushButton::clicked, this,
          &DepthFadingAndClippingDialog::resetClipping);
}

/// By using CLIPPING_SCALE we can increase the granularity of the slider
void DepthFadingAndClippingDialog::initClippingSlider() {
  frontClippingSlider->setSingleStep(CLIPPING_STEP);
  frontClippingSlider->setTickInterval(CLIPPING_INTERVAL);
  frontClippingSlider->setMinimum(CLIPPING_SCALE * cx::globals::frontClippingPlane);
  frontClippingSlider->setMaximum(CLIPPING_SCALE * CLIPPING_MAXIMUM);
}

void DepthFadingAndClippingDialog::showDialogWithDepthFadingTab() {
  tabWidget->setCurrentIndex(DEPTHFADING_TAB);
  show();
}

void DepthFadingAndClippingDialog::showDialogWithClippingTab() {
  tabWidget->setCurrentIndex(CLIPPING_TAB);
  show();
}

void DepthFadingAndClippingDialog::reportDepthFadingSettings() {
  bool state = enableDepthFogCheckBox->checkState();
  enableFadeWidgets(state);
  settings::writeSetting(settings::keys::DEPTH_FOG_ENABLED, state);
  settings::writeSetting(settings::keys::DEPTH_FOG_DENSITY,
                         getFogDensity(fogDensitySlider));
  settings::writeSetting(settings::keys::DEPTH_FOG_OFFSET,
                         getFogOffset(fogOffsetSlider));
  emit depthFadingSettingsChanged();
}

void DepthFadingAndClippingDialog::enableFadeWidgets(bool enable) {
  fogDensitySlider->setEnabled(enable);
  fogOffsetSlider->setEnabled(enable);
}

float DepthFadingAndClippingDialog::getFogDensity(QSlider *slider) {
  return slider->value() / FADING_SCALE_FACTOR;
}

float DepthFadingAndClippingDialog::getFogOffset(QSlider *slider) {
  return slider->value() / FOG_OFFSET_SCALE_FACTOR;
}

void DepthFadingAndClippingDialog::reportClippingSettings(int value) {
  emit frontClippingPlaneChanged(value / CLIPPING_SCALE);
}

void DepthFadingAndClippingDialog::resetClipping() {
  frontClippingSlider->setValue(frontClippingSlider->minimum());
}
