#pragma once
#include <QDialog>

#include "ui_depthfadingandclippingdialog.h"

const int DEPTHFADING_TAB = 0;
const int CLIPPING_TAB = 1;

const float FADING_SCALE_FACTOR = 1.0;
const float FOG_OFFSET_SCALE_FACTOR = 100.0;

const int CLIPPING_STEP = 1;
const float CLIPPING_SCALE = 10.0;
const int CLIPPING_INTERVAL = CLIPPING_SCALE;
const int CLIPPING_MAXIMUM = 70;

class DepthFadingAndClippingDialog : public QDialog,
                                     public Ui::DepthFadingAndClippingDialog {
  Q_OBJECT

public:
  DepthFadingAndClippingDialog(QWidget *parent = 0);

public slots:
  void showDialogWithDepthFadingTab();
  void showDialogWithClippingTab();

signals:
  void depthFadingSettingsChanged();
  void frontClippingPlaneChanged(float);

private slots:
  void reportDepthFadingSettings();
  void reportClippingSettings(int);
  void resetClipping();

private:
  void init();
  void initConnections();
  void initClippingSlider();
  void enableFadeWidgets(bool);
  float getFogDensity(QSlider *);
  float getFogOffset(QSlider *);
};
