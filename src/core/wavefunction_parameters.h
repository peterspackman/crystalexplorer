#pragma once
#include <QString>
#include <ankerl/unordered_dense.h>
#include "generic_atom_index.h"

class ChemicalStructure;

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
    bool accepted{false};

    inline bool hasEquivalentMethodTo(const Parameters &rhs) const {
	return (method.toLower() == rhs.method.toLower()) && (basis.toLower() == rhs.basis.toLower());
    }

    inline bool operator==(const Parameters &rhs) const {
	if(structure != rhs.structure) return false;
	if(charge != rhs.charge) return false;
	if(multiplicity != rhs.multiplicity) return false;
	if(method != rhs.method) return false;
	if(basis != rhs.basis) return false;
	if(atoms != rhs.atoms) return false;
	return true;
    }
};

struct Result {
    QString filename;
    QString stdoutContents;
    ankerl::unordered_dense::map<QString, double> energy;
    bool success{false};
};


QString fileFormatString(FileFormat);
QString fileFormatSuffix(FileFormat);
FileFormat fileFormatFromFilename(const QString &filename);

}
