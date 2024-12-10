#include "colormap.h"
#include "globalconfiguration.h"
#include "settings.h"
#include <QDebug>
#include <QFile>
#include <nlohmann/json.hpp>

// Array of all Program values
inline constexpr std::array<ColorMapMethod, 5> availableColorMapMethods = {
    ColorMapMethod::Linear, ColorMapMethod::QuantizedLinear,
    ColorMapMethod::TriColor, ColorMapMethod::HueRange,
    ColorMapMethod::SingleColor};

QString colorMapMethodToString(ColorMapMethod cm) {
  switch (cm) {
  default: // case ColorMapMethod::Linear:
    return "linear";
  case ColorMapMethod::QuantizedLinear:
    return "quantized";
  case ColorMapMethod::TriColor:
    return "tricolor";
  case ColorMapMethod::HueRange:
    return "hue range";
  case ColorMapMethod::SingleColor:
    return "single";
  }
}

ColorMapMethod colorMapMethodFromString(const QString &name) {
  QString lowercaseName = name.toLower();
  for (const auto &method : availableColorMapMethods) {
    QString candidate = colorMapMethodToString(method).toLower();
    if (lowercaseName == candidate)
      return method;
  }
  return ColorMapMethod::Linear;
}

void from_json(const nlohmann::json &j, ColorMapDescription &cm) {
  cm.colors.clear();
  auto qs = [](const std::string &s) { return QString::fromStdString(s); };
  for (const auto &color : j.at("colors")) {
    if (color.is_array() && color.size() == 3) {
      float r = color[0];
      float g = color[1];
      float b = color[2];
      cm.colors.push_back(QColor::fromRgbF(r, g, b));
    } else if (color.is_string()) {
      // Keep existing string handling for backwards compatibility
      cm.colors.push_back(QColor(qs(color)));
    }
  }
  if (j.contains("method")) {
    cm.method = colorMapMethodFromString(qs(j.at("method").get<std::string>()));
  }
}

namespace impl {

template <typename T>
inline constexpr T clamp_range(T x, T lower = 0, T upper = 1) noexcept {
  return (x < lower) ? lower : (x > upper) ? upper : x;
}

inline float linear(float a, float b, float t) noexcept {
  return a * (1 - t) + b * t;
}

inline QColor linearColorMap(double x, const std::vector<QColor> &data) {
  const double a = clamp_range(x) * (data.size() - 1);
  const double i = std::floor(a);
  const double t = a - i;
  const QColor &colorA = data[static_cast<std::size_t>(i)];
  const QColor &colorB = data[static_cast<std::size_t>(std::ceil(a))];

  float h1, s1, v1;
  float h2, s2, v2;
  colorA.getRgbF(&h1, &s1, &v1);
  colorB.getRgbF(&h2, &s2, &v2);

  return QColor::fromRgbF(linear(h1, h2, t), linear(s1, s2, t),
                          linear(v1, v2, t));
}

inline double quantize(double x, uint_fast8_t levels) {
  levels = std::max(uint_fast8_t{1}, std::min(levels, uint_fast8_t{255}));

  const double interval = 255.0 / levels;

  constexpr double eps = 0.0005;
  const unsigned int index =
      static_cast<uint_fast8_t>((x * 255.0 - eps) / interval);

  const unsigned int upper =
      static_cast<uint_fast8_t>(index * interval + interval);
  const unsigned int lower = static_cast<uint_fast8_t>(upper - interval);

  const double middle = static_cast<double>(upper + lower) * 0.5 / 255.0;
  return middle;
}

} // namespace impl

QColor triColorMap(double x, double minValue, double maxValue,
                   const QColor &startColor, const QColor &midColor,
                   const QColor &endColor) {

  // Because the midColor is tied to 0 when using a "color mapped from color
  // range"
  // we can't allow minValue to become positive nor the maxValue to become
  // negative
  constexpr double LIMIT = 0.0001;
  minValue = (minValue > 0.0) ? -LIMIT : minValue;
  maxValue = (maxValue < 0.0) ? LIMIT : maxValue;

  QColor color = (x < 0.0) ? startColor : endColor;
  double denom = (x < 0.0) ? minValue : maxValue;
  double factor = 1.0 - x / denom;

  if (factor > 0.0) {
    return QColor::fromRgbF(
        (color.redF() + (midColor.redF() - color.redF()) * factor),
        (color.greenF() + (midColor.greenF() - color.greenF()) * factor),
        (color.blueF() + (midColor.blueF() - color.blueF()) * factor));
  }
  return color;
}

QColor colorMappedFromHueRange(double value, double minValue, double maxValue,
                               bool reverse, double minHue, double maxHue) {

  double newValue = std::max(std::min(value, maxValue), value);
  double range = maxValue - minValue;
  double rangeRatio = (range > 1e-6) ? (maxHue - minHue) / range : 0.0;

  double h;
  if (reverse) {
    h = std::max(minHue, (maxHue - rangeRatio * (newValue - minValue)));
    h = std::min(h, maxHue);
  } else {
    h = std::min(maxHue, (minHue + rangeRatio * (newValue - minValue)));
    h = std::max(h, minHue);
  }
  return QColor::fromHsvF(h, 1.0, 1.0).toRgb();
}

QColor linearColorMap(double x, const ColorMapDescription &cm) {
  return impl::linearColorMap(x, cm.colors);
}

QColor quantizedLinearColorMap(double x, unsigned int num_levels,
                               const ColorMapDescription &cm) {
  return impl::linearColorMap(impl::quantize(x, num_levels), cm.colors);
}

ColorMap::ColorMap(const QString &n, double minValue, double maxValue)
    : name(n), lower(minValue), upper(maxValue) {

  noneColor = QColor(
      settings::readSetting(settings::keys::NONE_PROPERTY_COLOR).toString());
  description = getColorMapDescription(n);
}

QColor ColorMap::operator()(double x) const {
  const auto &c = description.colors;
  switch (description.method) {
  case ColorMapMethod::SingleColor: {
    return c[0];
    break;
  }
  case ColorMapMethod::TriColor: {
    return triColorMap(x, lower, upper, c[0], c[1], c[2]);
    break;
  }
  case ColorMapMethod::HueRange: {
    const double minHue = c[0].hueF();
    const double maxHue = c[1].hueF();
    return colorMappedFromHueRange(x, lower, upper, reverse, minHue, maxHue);
    break;
  }
  case ColorMapMethod::QuantizedLinear: {
    return quantizedLinearColorMap((x - lower) / (upper - lower),
                                   quantizationLevels, description);
    break;
  }
  default: {
    return linearColorMap((x - lower) / (upper - lower), description);
    break;
  }
  }
}

QMap<QString, ColorMapDescription> loadColorMaps(const nlohmann::json &json) {
  QMap<QString, ColorMapDescription> cmaps;
  auto qs = [](const std::string &str) { return QString::fromStdString(str); };
  qDebug() << "Load colormap data from JSON";

  for (const auto &item : json.items()) {
    try {
      ColorMapDescription cmap;
      from_json(item.value(), cmap);
      cmaps.insert(qs(item.key()), cmap);
    } catch (nlohmann::json::exception &e) {
      qWarning() << "Failed to parse color map" << qs(item.key()) << ":"
                 << e.what();
    }
  }
  return cmaps;
}

bool loadColorMapConfiguration(QMap<QString, ColorMapDescription> &cmaps) {
  QFile file(":/resources/colormaps.json");
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning("Couldn't open config file.");
    return false;
  }
  QByteArray data = file.readAll();

  nlohmann::json doc;
  try {
    doc = nlohmann::json::parse(data.constData());
  } catch (nlohmann::json::parse_error &e) {
    qWarning() << "JSON parse error:" << e.what();
    return false;
  }

  cmaps = loadColorMaps(doc);
  return true;
}

ColorMapDescription getColorMapDescription(const QString &name) {
  const auto &descriptions =
      GlobalConfiguration::getInstance()->getColorMapDescriptions();
  auto loc = descriptions.find(name);
  if (loc != descriptions.end()) {
    return *loc;
  }
  QColor noneColor(
      settings::readSetting(settings::keys::NONE_PROPERTY_COLOR).toString());
  return ColorMapDescription{"None", {noneColor}, ColorMapMethod::SingleColor};
}

QStringList availableColorMaps() {
  const auto &descriptions =
      GlobalConfiguration::getInstance()->getColorMapDescriptions();
  return descriptions.keys();
}
