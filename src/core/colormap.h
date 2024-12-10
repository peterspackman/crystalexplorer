#pragma once
#include <QColor>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <occ/core/linear_algebra.h>

enum class ColorMapMethod {
  Linear,
  QuantizedLinear,
  TriColor,
  HueRange,
  SingleColor
};

struct ColorMapDescription {
  QString name{"Unknown"};
  std::vector<QColor> colors{Qt::white};
  ColorMapMethod method{ColorMapMethod::Linear};
};

bool loadColorMapConfiguration(QMap<QString, ColorMapDescription> &colorMaps);

QColor linearColorMap(double x, const ColorMapDescription &);
QColor quantizedLinearColorMap(double x, unsigned int num_levels,
                               const ColorMapDescription &);

struct ColorMap {
  explicit ColorMap(const QString &, double minValue = 0.0,
                    double maxValue = 0.0);
  QColor operator()(double x) const;
  QString name;
  double lower{0.0};
  double upper{1.0};
  bool reverse{false};
  int quantizationLevels{4};
  QColor noneColor{Qt::white};
  ColorMapDescription description;
};

ColorMapDescription getColorMapDescription(const QString &name);

QString colorMapMethodToString(ColorMapMethod cm);
ColorMapMethod colorMapMethodFromString(const QString &);

QStringList availableColorMaps();
