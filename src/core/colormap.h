#pragma once
#include <QColor>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <occ/core/linear_algebra.h>

enum class ColorMapName {
  Parula,
  Heat,
  Jet,
  Turbo,
  Hot,
  Gray,
  Magma,
  Inferno,
  Plasma,
  Viridis,
  Cividis,
  Github,
  Cubehelix,
  HSV,
  CE_bwr,
  CE_rgb,
  CE_None,
  Archambault,
  Austria,
  Benedictus,
  Cassatt1,
  Cassatt2,
  Cross,
  Degas,
  Demuth,
  Derain,
  Egypt,
  Gauguin,
  Greek,
  Hiroshige,
  Hokusai1,
  Hokusai2,
  Hokusai3,
  Homer1,
  Homer2,
  Ingres,
  Isfahan1,
  Isfahan2,
  Java,
  Johnson,
  Juarez,
  Kandinsky,
  Klimt,
  Lakota,
  Manet,
  Monet,
  Moreau,
  Morgenstern,
  Nattier,
  Navajo,
  NewKingdom,
  Nizami,
  OKeeffe1,
  OKeeffe2,
  Paquin,
  Peru1,
  Peru2,
  Pillement,
  Pissaro,
  Redon,
  Renoir,
  Robert,
  Signac,
  Stevens,
  Tam,
  Tara,
  Thomas,
  Tiepolo,
  Troy,
  Tsimshian,
  VanGogh1,
  VanGogh2,
  VanGogh3,
  Veronese,
  Wissing,
};

QColor linearColorMap(double x, ColorMapName name);
QColor quantizedLinearColorMap(double x, unsigned int num_levels,
                               ColorMapName name);

std::vector<ColorMapName> availableColorMaps();
ColorMapName colorMapFromString(const QString &);
const char *colorMapToString(ColorMapName);

struct ColorMapFunc {

  ColorMapFunc(ColorMapName);
  ColorMapFunc(ColorMapName, double minValue, double maxValue);
  ColorMapFunc(QString, double minValue = 0.0, double maxValue = 0.0);

  QColor operator()(double x) const;

  ColorMapName name;
  double lower{0.0};
  double upper{1.0};
  bool reverse{false};
  QColor noneColor{Qt::white};
};
