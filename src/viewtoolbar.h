#pragma once

#include <QAction>
#include <QLabel>
#include <QMovie>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QToolBar>

const float MIN_VALUE = 0.0;
const float MAX_VALUE = 360.0;
const float SCALE_MIN_VALUE = 1.0;
const float SCALE_MAX_VALUE = 9999.0;
const float SCALING_FACTOR = 150.0;
const int ANIM_SLIDER_MIN_VALUE = 0;
const int ANIM_SLIDER_MAX_VALUE = 20;

enum AxisID { aAxis, bAxis, cAxis, aStarAxis, bStarAxis, cStarAxis };

class ViewToolbar : public QToolBar {
  Q_OBJECT

public:
  ViewToolbar(QWidget *parent = 0);

public slots:
  void setRotations(float, float, float);
  void setMillerViewDirection(float, float, float);
  void setScale(float);
  void resetAll();
  void showAnimationSpeedControl(bool);
  void showCalculationRunning(bool);

protected slots:
  void scaleSpinBoxChanged(int);
  void aButtonClicked();
  void bButtonClicked();
  void cButtonClicked();
  void hChanged(double);
  void kChanged(double);
  void lChanged(double);
  void animSpeedSliderChanged(int);

signals:
  void rotateAboutX(int);
  void rotateAboutY(int);
  void rotateAboutZ(int);
  void scaleChanged(float);
  void viewDirectionChanged(float, float, float);
  void axisButtonClicked(AxisID);
  void animSpeedChanged(float);
  void recenterScene();

private:
  void createWidgets();
  void addWidgetsToToolbar();
  void setupConnections();
  QLabel *horizRotLabel;
  QLabel *vertRotLabel;
  QLabel *zRotLabel;
  QSpinBox *xSpinBox;
  QSpinBox *ySpinBox;
  QSpinBox *zSpinBox;
  QLabel *scaleLabel;
  QSpinBox *scaleSpinBox;
  QLabel *viewLabel;
  QLabel *calculationRunningLabel;
  QMovie *calculationRunningMovie;
  QPushButton *viewDownAButton;
  QPushButton *viewDownBButton;
  QPushButton *viewDownCButton;
  QPushButton *recenterButton;
  QDoubleSpinBox *hSpinBox;
  QDoubleSpinBox *kSpinBox;
  QDoubleSpinBox *lSpinBox;
  float m_xRot{0.0f}, m_yRot{0.0f}, m_zRot{0.0f};
  double m_h{1.0}, m_k{0.0}, m_l{0.0};

  QLabel *animMinLabel;
  QSlider *animSpeedSlider;
  QLabel *animMaxLabel;
  QAction *animMinLabelAction;
  QAction *animSpeedSliderAction;
  QAction *animMaxLabelAction;
};
