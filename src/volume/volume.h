#pragma once
#include <QMatrix4x4>
#include <vector>
#include <array>

namespace volume {

template<typename T>

struct Volume {
    QMatrix4x4 transform;
    std::array<int, 3> dimensions;
    std::vector<T> data;
};

}
