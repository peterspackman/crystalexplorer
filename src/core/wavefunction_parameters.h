#pragma once
#include <QString>
#include <ankerl/unordered_dense.h>
#include "chemicalstructure.h"
#include "generic_atom_index.h"

namespace wfn {

enum class FileFormat {
    OccWavefunction,
    Fchk,
    Molden
};

struct Parameters {
    int charge{0};
    int multiplicity{1};
    QString method{"b3lyp"};
    QString basis{"def2-qzvp"};
    ChemicalStructure *structure{nullptr};
    std::vector<GenericAtomIndex> atoms;
};

struct Result {
    QString filename;
    QString stdoutContents;
    ankerl::unordered_dense::map<QString, double> energy;
    bool success{false};
};


QString fileFormatString(FileFormat);

}
