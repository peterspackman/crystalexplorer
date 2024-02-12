#pragma once
#include <QColor>
#include <QMap>
#include <QString>
#include <QTextStream>
#include <QVector>

#include "surfacedescription.h"

class SurfaceProperty {
  friend QDataStream &operator<<(QDataStream &, const SurfaceProperty &);
  friend QDataStream &operator>>(QDataStream &, SurfaceProperty &);

public:
  SurfaceProperty();
  SurfaceProperty(QString, QVector<float>);
  IsosurfacePropertyDetails::Type type() const { return _type; }
  QString propertyName() const {
    return IsosurfacePropertyDetails::getAttributes(_type).name;
  }
  float min() const { return _min; }
  float max() const { return _max; }
  float mean() const { return _mean; }
  QMap<PropertyStatisticsType, double> getStatistics() const;
  float valueAtVertex(int) const;
  QColor colorAtVertex(int) const;
  void updateColors();
  void updateColors(float, float);
  void setNonePropertyColor(QColor color);
  void resetNonePropertyColor();
  QString units() const;
  float rescaledMin() const { return _rescaledMin; }
  float rescaledMax() const { return _rescaledMax; }
  void mergeValues(int, int);

private:
  void updateMinMaxMean();

  IsosurfacePropertyDetails::Type _type;
  QVector<float> _values;
  QVector<QColor> _colors;
  float _min;
  float _max;
  float _mean;
  QColor _nonePropColor;

  float _rescaledMin;
  float _rescaledMax;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &, const SurfaceProperty &);
QDataStream &operator>>(QDataStream &, SurfaceProperty &);
