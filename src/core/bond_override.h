#pragma once
#include "generic_atom_index.h"

enum class BondMethod {
  Bond,     // Force bond to exist
  DontBond, // Force bond to not exist
  Detect    // Use normal detection logic, default
};

struct BondIndexPair {
  GenericAtomIndex a;
  GenericAtomIndex b;

  inline bool operator==(const BondIndexPair &rhs) const {
    return std::tie(a, b) == std::tie(rhs.a, rhs.b);
  }
};

struct BondIndexPairHash {
  using is_avalanching = void;
  [[nodiscard]] auto operator()(BondIndexPair const &idx) const noexcept
      -> uint64_t {
    static_assert(std::has_unique_object_representations_v<BondIndexPair>);
    return ankerl::unordered_dense::detail::wyhash::hash(&idx, sizeof(idx));
  }
};

struct BondOverride {
  GenericAtomIndex a;
  GenericAtomIndex b;
  BondMethod bond{BondMethod::Detect};
};

inline BondIndexPair makeBondPair(GenericAtomIndex a,
                                  GenericAtomIndex b) {
  return (a < b) ? BondIndexPair{a, b} : BondIndexPair{b, a};
}
