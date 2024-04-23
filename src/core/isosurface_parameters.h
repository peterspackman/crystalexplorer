#pragma once
#include <QString>
#include "chemicalstructure.h"
#include "molecular_wavefunction.h"

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
    MolecularWavefunction *wfn{nullptr};
};

struct Result {
    bool success{false};
};


QString kindToString(Kind);
Kind stringToKind(const QString &);


struct SurfacePropertyDescription {
    QString cmap;
    QString occName;
    QString displayName;
    QString units;
    bool needsWavefunction{false};
    bool needsIsovalue{false};
    bool needsOrbital{false};
    QString description;
};

struct SurfaceDescription {
    QString displayName{"Unknown"};
    QString occName{"unknown"};
    double defaultIsovalue{0.0};
    bool needsIsovalue{false};
    bool needsWavefunction{false};
    bool needsOrbital{false};
    bool needsCluster{false};
    bool periodic{false};
    QString units{""};
    QString description{"Unknown"};
    QStringList requestableProperties{"none"};
};

SurfaceDescription getSurfaceDescription(Kind);

bool loadSurfaceDescriptionConfiguration(
    QMap<QString, SurfacePropertyDescription> &,
    QMap<QString, SurfaceDescription> &,
    QMap<QString, double> &
);

}
