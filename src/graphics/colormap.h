#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <QColor>
#include <occ/core/linear_algebra.h>

enum class ColorMapName
{
    Parula, Heat, Jet, Turbo, Hot, Gray, Magma, Inferno, Plasma, Viridis, Cividis, Github, Cubehelix, HSV
};

QColor linearColorMap(double x, ColorMapName name);
QColor quantizedLinearColorMap(double x, unsigned int num_levels, ColorMapName name);
