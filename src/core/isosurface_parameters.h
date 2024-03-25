#pragma once
#include <QString>
#include "chemicalstructure.h"

namespace isosurface {

enum class Kind {
    Promolecule,
    Hirshfeld,
    Void,
    ESP,
    ElectronDensity,
    DeformationDensity,
    Unknown
};


struct Parameters {
    Kind kind;
    float isovalue{0.0};
    float separation{0.2};
    ChemicalStructure *structure{nullptr};
};

struct Result {
    bool success{false};
};


QString kindToString(Kind);
Kind stringToKind(const QString &);

}
