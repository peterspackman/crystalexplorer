#pragma once
#include "fragment.h"
#include "generic_atom_index.h"
#include "json.h"
#include "wavefunction_parameters.h"
#include <Eigen/Geometry>
#include <QString>
#include <ankerl/unordered_dense.h>

class MolecularWavefunction;
class ChemicalStructure;

namespace pair_energy {

struct EnergyModelParameters {
  QString model{"ce-1p"};
  std::vector<wfn::Parameters> wavefunctions;
  std::vector<FragmentDimer> pairs;
  bool operator==(const EnergyModelParameters &rhs) const;
  inline bool operator!=(const EnergyModelParameters &rhs) const {
    return !(*this == rhs);
  }
  bool isXtbModel() const;
};

struct Parameters {
  QString model{"ce-1p"};

  std::vector<GenericAtomIndex> atomsA;
  std::vector<GenericAtomIndex> atomsB;
  Eigen::Isometry3d transformA = Eigen::Isometry3d::Identity();
  Eigen::Isometry3d transformB = Eigen::Isometry3d::Identity();

  FragmentDimer fragmentDimer;

  MolecularWavefunction *wfnA{nullptr};
  MolecularWavefunction *wfnB{nullptr};
  ChemicalStructure *structure{nullptr};

  QString deriveName() const;

  int multiplicity() const;
  int charge() const;
  bool operator==(const Parameters &rhs) const;

  inline bool operator!=(const Parameters &rhs) const {
    return !(*this == rhs);
  }

  bool isXtbModel() const;
  bool hasPermutationSymmetry{true};
};

struct Result {
  QString filename;
  QString stdoutContents;
  ankerl::unordered_dense::map<QString, double> energy;
  bool success{false};
};

} // namespace pair_energy

void to_json(nlohmann::json &j, const pair_energy::Parameters &);
void from_json(const nlohmann::json &j, pair_energy::Parameters &);
