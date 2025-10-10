#pragma once
#include "generic_atom_index.h"
#include <QString>
#include <QMetaType>
#include <ankerl/unordered_dense.h>

class ChemicalStructure;

namespace wfn {

enum class FileFormat { OccWavefunction, Fchk, Molden, XtbJson };
enum class Program { Occ, Orca, Gaussian, NWChem, Psi4, Xtb };

struct Parameters {
  int charge{0};
  int multiplicity{1};
  Program program{Program::Occ};
  QString method{"b3lyp"};
  QString basis{"def2-svp"};
  ChemicalStructure *structure{nullptr};
  std::vector<GenericAtomIndex> atoms;
  bool accepted{false};
  bool userEditRequested{false};
  QString name{"wavefunction"};
  QString userInputContents;

  inline bool hasEquivalentMethodTo(const Parameters &rhs) const {
    return (method.toLower() == rhs.method.toLower()) &&
           (basis.toLower() == rhs.basis.toLower());
  }

  inline bool operator==(const Parameters &rhs) const {
    if (structure != rhs.structure)
      return false;
    if (charge != rhs.charge)
      return false;
    if (multiplicity != rhs.multiplicity)
      return false;
    if (method != rhs.method)
      return false;
    if (basis != rhs.basis)
      return false;
    if (atoms != rhs.atoms)
      return false;
    return true;
  }

  inline bool operator!=(const Parameters &rhs) const {
    return !(*this == rhs);
  }

  bool isXtbMethod() const;
};

struct Result {
  QString filename;
  QString stdoutContents;
  ankerl::unordered_dense::map<QString, double> energy;
  bool success{false};
};

QString fileFormatString(FileFormat);
QString fileFormatSuffix(FileFormat);
FileFormat fileFormatFromString(const QString &);
FileFormat fileFormatFromFilename(const QString &filename);

QString programName(Program);
Program programFromName(const QString &programString);

} // namespace wfn

Q_DECLARE_METATYPE(wfn::Parameters)
