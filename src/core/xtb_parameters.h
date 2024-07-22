#pragma once
#include "generic_atom_index.h"
#include <QString>
#include <ankerl/unordered_dense.h>

class ChemicalStructure;

namespace xtb {

enum class Method {
  GFN0_xTB,
  GFN1_xTB,
  GFN2_xTB,
  GFN_FF
};

struct Parameters {
  int charge{0};
  int multiplicity{1};
  Method method{Method::GFN2_xTB};
  ChemicalStructure *structure{nullptr};
  std::vector<GenericAtomIndex> atoms;
  bool accepted{false};

  inline bool hasEquivalentMethodTo(const Parameters &rhs) const {
    return (method == rhs.method);
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
    if (atoms != rhs.atoms)
      return false;
    return true;
  }
};

struct Result {
  QString filename;
  QString jsonContents;
  ankerl::unordered_dense::map<QString, double> energy;
  bool success{false};
};

inline QString methodToString(Method) { return "GFN2-xTB"; }

} // namespace wfn
