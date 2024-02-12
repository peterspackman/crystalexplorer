#pragma once
#include <QTableWidget>
#include <QTextDocument>
#include <QWidget>

#include "surface.h"
#include "ui_surfacecontroller.h"

// The following strings *must* match those in the propertyFromString map (see
// surfacedescription.h)
const QStringList clampedProperties = QStringList() << "shape_index"
                                                    << "curvedness"
                                                    << "none";
const QVector<float> clampedMinimumScaleValues = QVector<float>()
                                                 << -1.0f << -4.0f << 0.0f;
const QVector<float> clampedMaximumScaleValues = QVector<float>()
                                                 << +1.0f << +0.4f << 0.0f;

class SurfaceController : public QWidget, public Ui::SurfaceController {
  Q_OBJECT

public:
  SurfaceController(QWidget *parent = 0);
  void setSurfaceInfo(float, float, float, float);
  void enableFingerprintButton(bool);
  void currentSurfaceVisibilityChanged(bool);
  void selectLastSurfaceProperty();

public slots:
  void resetSurface();
  void setCurrentSurface(Surface *);
  void setPropertyInfo(const SurfaceProperty *);
  void setSelectedPropertyValue(float);

protected slots:
  void surfacePropertySelected(int);
  void minPropertyChanged();
  void maxPropertyChanged();
  void resetScale();
  void exportButtonClicked();

signals:
  void updateSurfaceTransparency(bool);
  void surfacePropertyChosen(int);
  void showFingerprint();
  void surfacePropertyRangeChanged(float, float);
  void exportCurrentSurface();

private:
  void setup();
  void populatePropertyComboBox(const QStringList &, int);
  void setScale(float, float);
  void clampScale(float, float,
                  bool emitUpdateSurfacePropertyRangeSignal = false);
  void setMinAndMaxSpinBoxes(float, float);
  void setUnitLabels(QString);
  void updateSurfacePropertyRange();
  void clearPropertyInfo();
  QString convertToNaturalPropertyName(QString);
  void enableSurfaceControls(bool);

  enum { OPTIONS_PAGE, SURFACEINFO_PAGE, PROPERTYINFO_PAGE };

  int _currentPropertyIndex;

  bool _updateSurfacePropertyRange;

  const SurfaceProperty *_currentSurfaceProperty;
};
