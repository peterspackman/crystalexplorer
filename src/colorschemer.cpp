#include "colorschemer.h"
#include "globals.h"
#include "mathconstants.h"
#include "settings.h"
#include <QColor>
#include <QtDebug>
#include <cmath>

// cosine based palette, 4 vec3 params
QVector3D cosine_palette(float t, const QVector3D &a, const QVector3D &b,
                         const QVector3D &c, const QVector3D &d) {
  return a + b * QVector3D(std::cos(PI * (c.x() * t + d.x())),
                           std::cos(PI * (c.y() * t + d.y())),
                           std::cos(PI * (c.z() * t + d.z())));
}

QColor color_from_cosine_palette(float x, float minValue, float maxValue) {
  float norm = (std::max(std::min(x, maxValue), minValue) - minValue) /
               (maxValue - minValue);
  auto c = cosine_palette(
      norm, QVector3D(0.5f, 0.5f, 0.5f), QVector3D(0.5f, 0.5f, 0.5f),
      QVector3D(1.0f, 1.0f, 1.0f), QVector3D(0.00f, 0.10f, 0.20f));
  return QColor::fromRgbF(c.x(), c.y(), c.z());
}

QColor ColorSchemer::color(ColorScheme scheme, float value, float minValue,
                           float maxValue, bool reverse) {
  QColor color;
  switch (scheme) {
  case ColorScheme::noneColor:
    color = ColorSchemer::getNoneColor();
    break;
  case ColorScheme::RedGreenBlue:
    color = ColorSchemer::colorMappedFromHueRange(value, minValue, maxValue,
                                                  reverse, REDGREENBLUE_MINHUE,
                                                  REDGREENBLUE_MAXHUE);
    break;
  case ColorScheme::RedWhiteBlue:
    color = ColorSchemer::colorMappedFromColorRange(
        value, minValue, maxValue, COLORSCHEME_RED, COLORSCHEME_WHITE,
        COLORSCHEME_BLUE);
    break;
  case ColorScheme::OrangeWhitePurple:
    color = ColorSchemer::colorMappedFromColorRange(
        value, minValue, maxValue, COLORSCHEME_ORANGE, COLORSCHEME_WHITE,
        COLORSCHEME_PURPLE);
    break;
  case ColorScheme::Qualitative14Dark:
    color = ColorSchemer::colorAsQualitative14Dark(value, minValue, maxValue);
    break;
  case ColorScheme::Qualitative14Light:
    color = ColorSchemer::colorAsQualitative14Light(value, minValue, maxValue);
    break;
  case ColorScheme::SpectralRainbow:
    color = ColorSchemer::colorAsSpectralRainbow(value, minValue, maxValue);
    break;
  case ColorScheme::Viridis:
    color = ColorSchemer::colorAsViridis(value, minValue, maxValue);
    break;
  case ColorScheme::Magma:
    color = ColorSchemer::colorAsMagma(value, minValue, maxValue);
    break;
  case ColorScheme::MaterialDesign:
    color = ColorSchemer::colorAsMaterialDesign20(value, minValue, maxValue);
    break;
  case ColorScheme::Rainbow:
    // This prevents the value=minValue or a value-maxValue both being set to
    // red
    // Since hue(0.0) is red and hue(359.0) is also red
    float newMaxValue = maxValue + 1;
    color = ColorSchemer::colorMappedFromHueRange(
        value, minValue, newMaxValue, reverse, RAINBOW_MINHUE, RAINBOW_MAXHUE);
    break;
  }
  return color;
}

QColor ColorSchemer::colorMappedFromHueRange(float value, float minValue,
                                             float maxValue, bool reverse,
                                             float minHue, float maxHue) {
  QColor color;

  float newValue;
  newValue = qMin(value, maxValue);
  newValue = qMax(value, newValue);

  float range = maxValue - minValue;
  float rangeRatio;
  if (range > EPSILON) {
    rangeRatio = (maxHue - minHue) / range;
  } else {
    rangeRatio = 0.0;
  }

  float h;
  if (reverse) {
    h = qMax(minHue, (maxHue - rangeRatio * (newValue - minValue)));
    h = qMin(h, maxHue);
  } else {
    h = qMin(maxHue, (minHue + rangeRatio * (newValue - minValue)));
    h = qMax(h, minHue);
  }
  color.setHsv((int)h, 255, 255);

  return color;
}

QColor ColorSchemer::colorMappedFromColorRange(float value, float minValue,
                                               float maxValue,
                                               QColor startColor,
                                               QColor midColor,
                                               QColor endColor) {
  // Because the midColor is tied to 0 when using a "color mapped from color
  // range"
  // we can't allow minValue to become positive nor the maxValue to become
  // negative
  const float LIMIT = 0.0001f;
  minValue = (minValue > 0.0) ? -LIMIT : minValue;
  maxValue = (maxValue < 0.0) ? LIMIT : maxValue;

  float factor;
  QColor color;
  if (value < 0.0) { // Mapping to color range: [startColor,midColor]
    factor = 1.0 - value / minValue;
    color = startColor;
  } else { // Mapping to color range: [midColor,endColor]
    factor = 1.0 - value / maxValue;
    color = endColor;
  }

  QColor finalColor = QColor(color);

  if (factor > 0.0) {
    // A small error might be introduced here since a real is cast to an int
    finalColor.setRed(color.red() + (midColor.red() - color.red()) * factor);
    finalColor.setGreen(color.green() +
                        (midColor.green() - color.green()) * factor);
    finalColor.setBlue(color.blue() +
                       (midColor.blue() - color.blue()) * factor);
  }

  return finalColor;
}

QColor ColorSchemer::colorAsQualitative14Dark(float value, float minValue,
                                              float maxValue) {
  Q_ASSERT(value <= maxValue);
  Q_ASSERT(value >= minValue);

  int inputStart = int(minValue);
  int input = int(value);

  return QUALITATIVE_DARK_14[(input - inputStart) % QUALITATIVE14_SIZE];
}

QColor ColorSchemer::colorAsMaterialDesign20(float value, float minValue,
                                             float maxValue) {
  Q_ASSERT(value <= maxValue);
  Q_ASSERT(value >= minValue);

  int inputStart = int(minValue);
  int input = int(value);

  return MATERIAL_DESIGN_PALETTE_20[(input - inputStart) %
                                    MATERIAL_DESIGN_SIZE];
}

QColor ColorSchemer::colorAsQualitative14Light(float value, float minValue,
                                               float maxValue) {
  Q_ASSERT(value <= maxValue);
  Q_ASSERT(value >= minValue);

  int inputStart = int(minValue);
  int input = int(value);

  return QUALITATIVE_LIGHT_14[(input - inputStart) % QUALITATIVE14_SIZE];
}

QColor ColorSchemer::colorAsViridis(float value, float minValue,
                                    float maxValue) {
  Q_ASSERT(value <= maxValue);
  Q_ASSERT(value >= minValue);
  if (minValue == maxValue)
    return VIRIDIS[0];
  int position =
      int((value - minValue) * (VIRIDIS_SIZE - 1) / (maxValue - minValue));
  return VIRIDIS[position];
}

QColor ColorSchemer::colorAsMagma(float value, float minValue, float maxValue) {
  Q_ASSERT(value <= maxValue);
  Q_ASSERT(value >= minValue);
  if (minValue == maxValue)
    return MAGMA[0];
  int position =
      int((value - minValue) * (MAGMA_SIZE - 1) / (maxValue - minValue));
  return MAGMA[position];
}

QColor ColorSchemer::colorAsSpectralRainbow(float value, float minValue,
                                            float maxValue) {
  int inputStart = int(minValue);
  int inputEnd = int(maxValue);
  int input = int(value);

  int inputRange = inputEnd - inputStart + 1;

  QColor result;
  switch (inputRange) {
  case 3:
    result = SPECTRALRAINBOW_3[input];
    break;
  case 4:
    result = SPECTRALRAINBOW_4[input];
    break;
  case 5:
    result = SPECTRALRAINBOW_5[input];
    break;
  case 6:
    result = SPECTRALRAINBOW_6[input];
    break;
  case 7:
    result = SPECTRALRAINBOW_7[input];
    break;
  case 8:
    result = SPECTRALRAINBOW_8[input];
    break;
  case 9:
    result = SPECTRALRAINBOW_9[input];
    break;
  case 10:
    result = SPECTRALRAINBOW_10[input];
    break;
  case 11:
    result = SPECTRALRAINBOW_11[input];
    break;
  default:
    if (input > 10) {
      result = SPECTRALRAINBOW_11[10];
    } else {
      result = SPECTRALRAINBOW_11[input];
    }
    // Can but shouldn't get here.";
    // Q_ASSERT(false);
  }

  return result;
}

QColor ColorSchemer::getNoneColor() {
  return QColor(
      settings::readSetting(settings::keys::NONE_PROPERTY_COLOR).toString());
}
