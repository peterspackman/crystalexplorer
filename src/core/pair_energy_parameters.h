#pragma once
#include <QString>
#include <ankerl/unordered_dense.h>
#include <Eigen/Geometry>
#include "generic_atom_index.h"
#include "wavefunction_parameters.h"
#include "fragment.h"

class MolecularWavefunction;
class ChemicalStructure;

namespace pair_energy {

struct EnergyModelParameters {
    QString model{"ce-1p"};
    std::vector<wfn::Parameters> wavefunctions;
    std::vector<FragmentDimer> pairs;
    bool operator==(const EnergyModelParameters& rhs) const;
};


struct Parameters {
    QString model{"ce-1p"};
    std::vector<GenericAtomIndex> atomsA;
    std::vector<GenericAtomIndex> atomsB;
    Eigen::Isometry3d transformA = Eigen::Isometry3d::Identity();
    Eigen::Isometry3d transformB = Eigen::Isometry3d::Identity();

    MolecularWavefunction * wfnA{nullptr};
    MolecularWavefunction * wfnB{nullptr};
    ChemicalStructure * structure{nullptr};

    QString deriveName() const;

    bool operator==(const Parameters& rhs) const;
};

struct Result {
    QString filename;
    QString stdoutContents;
    ankerl::unordered_dense::map<QString, double> energy;
    bool success{false};
};

}
