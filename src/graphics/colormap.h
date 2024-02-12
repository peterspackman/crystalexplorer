#pragma once
#include <QString>
#include <QColor>


namespace cx::graphics {

enum class ColorMap{
    Viridis,
    Magma,
};

struct LinearColorMap {
    float minValue{0.0};
    float maxValue{1.0};
    ColorMap colorMap{ColorMap::Viridis};
    QColor operator()(float value) const;
};

}
