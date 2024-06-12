#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <QColor>
#include <occ/core/linear_algebra.h>

enum class ColorMapName
{
    Parula, Heat, Jet, Turbo, Hot, Gray, Magma, Inferno, Plasma, Viridis, Cividis, Github, Cubehelix, HSV,
    CE_bwr, CE_rgb, CE_None
};

QColor linearColorMap(double x, ColorMapName name);
QColor quantizedLinearColorMap(double x, unsigned int num_levels, ColorMapName name);


std::vector<ColorMapName> availableColorMaps();
ColorMapName colorMapFromString(const QString &);
const char * colorMapToString(ColorMapName);

struct ColorMapFunc {

    ColorMapFunc(ColorMapName);
    ColorMapFunc(ColorMapName, double minValue, double maxValue);

    QColor operator()(double x) const;

    ColorMapName name;
    double lower{0.0};
    double upper{1.0};
    bool reverse{false};
};
