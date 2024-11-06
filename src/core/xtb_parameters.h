#pragma once
#include "generic_atom_index.h"
#include "wavefunction_parameters.h"
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
  double reference_energy{0.0};
  bool accepted{false};
  QString name{"XtbCalculation"};
  bool write_molden{false};
  bool userEditRequested{false};
  QString userInputContents;

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
  QString name;
  QString filename;
  QByteArray stdoutContents;
  QByteArray jsonContents;
  QByteArray propertiesContents;
  QByteArray moldenContents;
  ankerl::unordered_dense::map<QString, double> energy;
  bool success{false};
};

inline QString methodToString(Method method) {
    switch(method) {
    default: return "GFN2-xTB";
    case Method::GFN0_xTB: return "GFN0-xTB";
    case Method::GFN1_xTB: return "GFN1-xTB";
    case Method::GFN_FF: return "GFN-FF";
    }
}

inline Method stringToMethod(const QString &s) {
    if (s.compare("GFN0-xTB", Qt::CaseInsensitive) == 0) return Method::GFN0_xTB;
    if (s.compare("GFN1-xTB", Qt::CaseInsensitive) == 0) return Method::GFN1_xTB;
    if (s.compare("GFN-FF", Qt::CaseInsensitive) == 0) return Method::GFN_FF;
    return Method::GFN2_xTB; // Default case
}

inline bool isXtbMethod(const QString &s) {
    if (s.compare("GFN0-xTB", Qt::CaseInsensitive) == 0) return true;
    if (s.compare("GFN1-xTB", Qt::CaseInsensitive) == 0) return true;
    if (s.compare("GFN2-xTB", Qt::CaseInsensitive) == 0) return true;
    if (s.compare("GFN-FF", Qt::CaseInsensitive) == 0) return true;
    return false; // Default case
}

} // namespace wfn
