#include <QDataStream>
#include <QString>
#include <QVector>
#include <QtDebug>
#include <limits>

#include "colorschemer.h"
#include "math.h"
#include "settings.h"
#include "surfaceproperty.h"

SurfaceProperty::SurfaceProperty() {}

SurfaceProperty::SurfaceProperty(QString propertyString,
                                 QVector<float> propertyValues) {
  _type = IsosurfacePropertyDetails::typeFromTontoName(propertyString);

  Q_ASSERT_X(_type != IsosurfacePropertyDetails::Type::Unknown, Q_FUNC_INFO,
             "Unknown surface property");

  Q_ASSERT(propertyValues.size() > 0);
  _values = propertyValues;

  updateMinMaxMean();

  switch (_type) {
  case IsosurfacePropertyDetails::Type::None:
    resetNonePropertyColor();
    break;
  default:
    updateColors();
  }
}

void SurfaceProperty::updateMinMaxMean() {
  _min = std::numeric_limits<float>::max();
  _max = std::numeric_limits<float>::min();
  _mean = 0.0;
  for (const auto &v : std::as_const(_values)) {
    _min = qMin(_min, v);
    _max = qMax(_max, v);
    _mean += v;
  }
  _mean /= _values.size();
}

QMap<PropertyStatisticsType, double> SurfaceProperty::getStatistics() const {
  QMap<PropertyStatisticsType, double> result;

  // Deviations
  int numPos = 0;
  int numNeg = 0;
  double meanPos = 0.0;
  double meanNeg = 0.0;
  double meanDev = 0.0;
  for (const auto &v : _values) {
    if (v < 0.0f) {
      meanNeg += v;
      numNeg++;
    } else {
      meanPos += v;
      numPos++;
    }
    meanDev += fabs(v - _mean);
  }

  result[MeanPlus] = meanPos / numPos;
  result[MeanMinus] = meanNeg / numNeg;
  result[PiStat] = meanDev / (numPos + numNeg);

  // Variances
  double varPos = 0.0;
  double varNeg = 0.0;
  for (const auto &v : _values) {
    if (v < 0.0f) {
      varNeg += (v - meanNeg) * (v - meanNeg);
    } else {
      varPos += (v - meanPos) * (v - meanPos);
    }
  }
  double varTot = varPos + varNeg;

  result[SigmaPlus] = varPos / numPos;
  result[SigmaMinus] = varNeg / numNeg;
  result[SigmaT] = varTot;
  result[NuStat] = (varPos * varNeg) / (varTot * varTot);

  return result;
}

void SurfaceProperty::resetNonePropertyColor() {
  Q_ASSERT(_type == IsosurfacePropertyDetails::Type::None);

  _nonePropColor = ColorSchemer::getNoneColor();
  updateColors();
}

void SurfaceProperty::setNonePropertyColor(QColor color) {
  Q_ASSERT(_type == IsosurfacePropertyDetails::Type::None);

  _nonePropColor = color;
  updateColors();
}

void SurfaceProperty::updateColors() { updateColors(_min, _max); }

void SurfaceProperty::updateColors(float minValue, float maxValue) {
  _rescaledMin = minValue;
  _rescaledMax = maxValue;

  _colors.clear();
  const auto &surfacePropertyAttributes =
      IsosurfacePropertyDetails::getAttributes(_type);
  if (_type == IsosurfacePropertyDetails::Type::None) {
    for (int i = 0; i < _values.size(); ++i) {
      _colors.append(_nonePropColor);
    }
  } else {
    for (int i = 0; i < _values.size(); ++i) {
      _colors.append(ColorSchemer::color(surfacePropertyAttributes.colorScheme,
                                         _values[i], minValue, maxValue));
    }
  }
}

float SurfaceProperty::valueAtVertex(int i) const {
  Q_ASSERT(i >= 0 && i < _values.size());
  return _values[i];
}

QColor SurfaceProperty::colorAtVertex(int i) const {
  Q_ASSERT(i >= 0 && i < _values.size());
  return _colors[i];
}

QString SurfaceProperty::units() const {
  const auto &surfacePropertyAttributes =
      IsosurfacePropertyDetails::getAttributes(_type);
  return surfacePropertyAttributes.unit;
}

void SurfaceProperty::mergeValues(int i, int j) {
  _values[i] = (_values[i] + _values[j]) / 2.0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &ds, const SurfaceProperty &property) {
  ds << property._type;
  ds << property._values;
  ds << property._nonePropColor;

  return ds;
}

QDataStream &operator>>(QDataStream &ds, SurfaceProperty &property) {
  int propertyType;
  ds >> propertyType;
  property._type = static_cast<IsosurfacePropertyDetails::Type>(propertyType);

  ds >> property._values;
  ds >> property._nonePropColor;

  property.updateMinMaxMean();
  property.updateColors();

  return ds;
}
