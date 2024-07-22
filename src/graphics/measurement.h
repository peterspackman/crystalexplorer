#pragma once
#include "circlerenderer.h"
#include "linerenderer.h"
#include <QColor>
#include <QVector3D>

enum class MeasurementType {
  Distance,
  Angle,
  Dihedral,
  OutOfPlaneBend,
  InPlaneBend
};

static const int ANGLE_TEXT_PRECISION = 2;
static const int DISTANCE_TEXT_PRECISION = 3;

class Measurement {

public:
  Measurement();
  Measurement(MeasurementType);
  void addPosition(QVector3D);
  void setColor(QColor);

  inline const QColor &color() const { return m_color; }

  void draw(LineRenderer *lines, CircleRenderer *circles) const;

  inline const QString &label() const { return _label; }
  inline const QVector3D labelPosition() const { return _labelPosition; }
  void removeLastPosition() {
    if (_positions.size() > 0)
      _positions.removeLast();
  }
  void clearPositions() { _positions.clear(); }

  inline MeasurementType type() const { return _type; }
  inline static int totalPositions(MeasurementType type) {
    switch (type) {
    case MeasurementType::Distance:
      return 2;
    case MeasurementType::Angle:
      return 3;
    case MeasurementType::Dihedral:
      return 4;
    case MeasurementType::OutOfPlaneBend:
      return 4;
    case MeasurementType::InPlaneBend:
      return 4;
    }
    return -1;
  }

private:
  float lineRadius() const;
  void calculateMeasurement();
  void calculateDistance();
  //	void calculateMinimumDistance();
  void calculateAngle();
  void calculateDihedral();
  void calculateOutOfPlaneBend();
  void calculateInPlaneBend();

  // drawing code
  void drawDistance(LineRenderer *lines, CircleRenderer *circles) const;
  void drawAngle(LineRenderer *lines, CircleRenderer *circles) const;
  void drawDihedral(LineRenderer *lines, CircleRenderer *circles) const;
  void drawOutOfPlaneBend(LineRenderer *lines, CircleRenderer *circles) const;
  void drawInPlaneBend(LineRenderer *lines, CircleRenderer *circles) const;

  MeasurementType _type{MeasurementType::Distance};
  QList<QVector3D> _positions;
  double _value;
  QString _label;
  QVector3D _labelPosition;
  QColor m_color;
};
