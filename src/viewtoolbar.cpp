#include "viewtoolbar.h"

#include <QDebug>
#include <QShortcut>

ViewToolbar::ViewToolbar(QWidget *parent) : QToolBar(parent) {
  setWindowTitle(tr("View Toolbar"));
  createWidgets();
  addWidgetsToToolbar();
  setupConnections();
  showAnimationSpeedControl(false);
}

void ViewToolbar::createWidgets() {

  horizRotLabel = new QLabel(this);
  horizRotLabel->setPixmap(QPixmap(":/images/rotateX.png")
                               .scaledToWidth(22, Qt::SmoothTransformation));
  vertRotLabel = new QLabel(this);
  vertRotLabel->setPixmap(QPixmap(":/images/rotateY.png")
                              .scaledToWidth(22, Qt::SmoothTransformation));
  zRotLabel = new QLabel(this);
  zRotLabel->setPixmap(QPixmap(":/images/rotateZ.png")
                           .scaledToWidth(22, Qt::SmoothTransformation));
  xSpinBox = new QSpinBox(this);
  xSpinBox->setRange(MIN_VALUE, MAX_VALUE);
  ySpinBox = new QSpinBox(this);
  ySpinBox->setRange(MIN_VALUE, MAX_VALUE);
  zSpinBox = new QSpinBox(this);
  zSpinBox->setRange(MIN_VALUE, MAX_VALUE);

  scaleLabel = new QLabel(this);
  scaleLabel->setText(tr("Scale"));
  scaleSpinBox = new QSpinBox(this);
  scaleSpinBox->setRange(SCALE_MIN_VALUE, SCALE_MAX_VALUE);

  viewLabel = new QLabel(this);
  viewLabel->setText(tr("View Direction"));
  // The leading/trailing space in the QPushButton labels *is* critical to
  // working round a Qt Bug.
  // Otherwise the shape of the button (Square or rounded) changes depending
  // on what single character is passed. 'a' and 'c' = square button, 'b' =
  // rounded.
  viewDownAButton = new QPushButton(this);
  viewDownAButton->setIcon(QPixmap(":/images/a-axis.png")
                               .scaledToWidth(22, Qt::SmoothTransformation));
  // viewDownAButton->setText(tr("View down &A-axis"));
  QShortcut *hotkeyA = new QShortcut(QKeySequence("Alt+A"), this);
  QObject::connect(hotkeyA, &QShortcut::activated, viewDownAButton,
                   &QPushButton::click);

  viewDownBButton = new QPushButton(this);
  viewDownBButton->setIcon(QPixmap(":/images/b-axis.png")
                               .scaledToWidth(22, Qt::SmoothTransformation));
  // viewDownBButton->setText(tr("View down &B-axis"));
  QShortcut *hotkeyB = new QShortcut(QKeySequence("Alt+B"), this);
  QObject::connect(hotkeyB, &QShortcut::activated, viewDownBButton,
                   &QPushButton::click);

  viewDownCButton = new QPushButton(this);
  viewDownCButton->setIcon(QPixmap(":/images/c-axis.png")
                               .scaledToWidth(22, Qt::SmoothTransformation));
  // viewDownCButton->setText(tr("View down &C-axis"));
  QShortcut *hotkeyC = new QShortcut(QKeySequence("Alt+C"), this);
  QObject::connect(hotkeyC, &QShortcut::activated, viewDownCButton,
                   &QPushButton::click);

  hSpinBox = new QDoubleSpinBox(this);
  hSpinBox->setRange(-100, 100);
  kSpinBox = new QDoubleSpinBox(this);
  kSpinBox->setRange(-100, 100);
  lSpinBox = new QDoubleSpinBox(this);
  lSpinBox->setRange(-100, 100);

  recenterButton = new QPushButton(this);
  recenterButton->setText(tr("&Recenter"));

  animMinLabel = new QLabel(this);
  animMinLabel->setText(tr("Animation Speed: min"));
  animSpeedSlider = new QSlider(this);
  animSpeedSlider->setRange(ANIM_SLIDER_MIN_VALUE, ANIM_SLIDER_MAX_VALUE);
  animSpeedSlider->setOrientation(Qt::Horizontal);
  animSpeedSlider->setValue((ANIM_SLIDER_MAX_VALUE - ANIM_SLIDER_MIN_VALUE) /
                            2);
  animMaxLabel = new QLabel(this);
  animMaxLabel->setText(tr("max"));

  // Animated currently running label
  calculationRunningLabel = new QLabel(this);
  calculationRunningMovie = new QMovie(":/images/loading.gif");
  calculationRunningMovie->setScaledSize(QSize(22, 22));

  xSpinBox->setFocusPolicy(Qt::StrongFocus);
  ySpinBox->setFocusPolicy(Qt::StrongFocus);
  zSpinBox->setFocusPolicy(Qt::StrongFocus);
  scaleSpinBox->setFocusPolicy(Qt::StrongFocus);
  hSpinBox->setFocusPolicy(Qt::StrongFocus);
  kSpinBox->setFocusPolicy(Qt::StrongFocus);
  lSpinBox->setFocusPolicy(Qt::StrongFocus);
}

void ViewToolbar::addWidgetsToToolbar() {
  addWidget(horizRotLabel);
  addWidget(xSpinBox);
  addWidget(vertRotLabel);
  addWidget(ySpinBox);
  addWidget(zRotLabel);
  addWidget(zSpinBox);
  addSeparator();
  addWidget(scaleLabel);
  addWidget(scaleSpinBox);
  addSeparator();
  addWidget(recenterButton);
  addSeparator();
  addWidget(viewLabel);
  addWidget(hSpinBox);
  addWidget(viewDownAButton);
  addWidget(kSpinBox);
  addWidget(viewDownBButton);
  addWidget(lSpinBox);
  addWidget(viewDownCButton);
  addSeparator();
  addWidget(calculationRunningLabel);
  animMinLabelAction = addWidget(animMinLabel);
  animSpeedSliderAction = addWidget(animSpeedSlider);
  animMaxLabelAction = addWidget(animMaxLabel);
}

void ViewToolbar::setupConnections() {
  connect(xSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &ViewToolbar::rotateAboutX);
  connect(ySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &ViewToolbar::rotateAboutY);
  connect(zSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &ViewToolbar::rotateAboutZ);
  connect(scaleSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &ViewToolbar::scaleSpinBoxChanged);
  connect(viewDownAButton, &QAbstractButton::clicked, this,
          &ViewToolbar::aButtonClicked);
  connect(viewDownBButton, &QAbstractButton::clicked, this,
          &ViewToolbar::bButtonClicked);
  connect(viewDownCButton, &QAbstractButton::clicked, this,
          &ViewToolbar::cButtonClicked);
  connect(recenterButton, &QAbstractButton::clicked, this,
          &ViewToolbar::recenterScene);
  connect(animSpeedSlider, &QAbstractSlider::valueChanged, this,
          &ViewToolbar::animSpeedSliderChanged);
  connect(hSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &ViewToolbar::hChanged);
  connect(kSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &ViewToolbar::kChanged);
  connect(lSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &ViewToolbar::lChanged);
}

void ViewToolbar::setRotations(float x, float y, float z) {
  QObject::blockSignals(true);
  xSpinBox->setValue(x);
  ySpinBox->setValue(y);
  zSpinBox->setValue(z);
  QObject::blockSignals(false);
}

void ViewToolbar::setMillerViewDirection(float h, float k, float l) {
  QObject::blockSignals(true);
  hSpinBox->setValue(h);
  kSpinBox->setValue(k);
  lSpinBox->setValue(l);
  QObject::blockSignals(false);
}

void ViewToolbar::showAnimationSpeedControl(bool show) {
  // A widget added to a QToolBar becomes/has an associated QAction
  // When this happens QWidget::setVisible not longer works and visibility
  // must be set through the QAction e.g...
  animMinLabelAction->setVisible(show);
  animSpeedSliderAction->setVisible(show);
  animMaxLabelAction->setVisible(show);
}

void ViewToolbar::setScale(float realscale) {
  int scale = (int)(realscale * SCALING_FACTOR);
  QObject::blockSignals(true);
  scaleSpinBox->setValue(scale);
  QObject::blockSignals(false);
}

void ViewToolbar::scaleSpinBoxChanged(int scale) {
  float realscale = scale / SCALING_FACTOR;
  emit scaleChanged(realscale);
}

void ViewToolbar::resetAll() {
  xSpinBox->setValue(0);
  ySpinBox->setValue(0);
  zSpinBox->setValue(0);
}

void ViewToolbar::aButtonClicked() {
  setMillerViewDirection(1, 0, 0);
  emit viewDirectionChanged(1, 0, 0);
}

void ViewToolbar::bButtonClicked() {
  setMillerViewDirection(0, 1, 0);
  emit viewDirectionChanged(0, 1, 0);
}

void ViewToolbar::cButtonClicked() {
  setMillerViewDirection(0, 0, 1);
  emit viewDirectionChanged(0, 0, 1);
}

void ViewToolbar::animSpeedSliderChanged(int value) {
  emit animSpeedChanged(
      value / ((float)ANIM_SLIDER_MAX_VALUE -
               ANIM_SLIDER_MIN_VALUE)); // speed is between 0.0 and 1.0
}

void ViewToolbar::showCalculationRunning(bool running) {
  calculationRunningLabel->setVisible(running);
  if (running) {
    calculationRunningLabel->setMovie(calculationRunningMovie);
    calculationRunningMovie->start();
  } else {
    calculationRunningMovie->stop();
    calculationRunningLabel->clear();
  }
}

void ViewToolbar::hChanged(double h) {
  emit viewDirectionChanged(h, kSpinBox->value(), lSpinBox->value());
}

void ViewToolbar::kChanged(double k) {
  emit viewDirectionChanged(hSpinBox->value(), k, lSpinBox->value());
}

void ViewToolbar::lChanged(double l) {
  emit viewDirectionChanged(hSpinBox->value(), kSpinBox->value(), l);
}
