#pragma once
#include "json.h"
#include <QDebug>
#include <ankerl/unordered_dense.h>

struct GenericAtomIndex {
  int unique{0};
  int x{0};
  int y{0};
  int z{0};

  [[nodiscard]] inline bool operator==(const GenericAtomIndex &rhs) const {
    return std::tie(unique, x, y, z) ==
           std::tie(rhs.unique, rhs.x, rhs.y, rhs.z);
  }

  [[nodiscard]] inline bool operator!=(const GenericAtomIndex &rhs) const {
    return !(*this == rhs);
  }

  [[nodiscard]] inline bool operator<(const GenericAtomIndex &rhs) const {
    return std::tie(unique, x, y, z) <
           std::tie(rhs.unique, rhs.x, rhs.y, rhs.z);
  }

  [[nodiscard]] inline bool operator>(const GenericAtomIndex &rhs) const {
    return std::tie(unique, x, z, z) >
           std::tie(rhs.unique, rhs.x, rhs.y, rhs.z);
  }
};

struct GenericAtomIndexHash {
  using is_avalanching = void;
  [[nodiscard]] auto operator()(GenericAtomIndex const &idx) const noexcept
      -> uint64_t {
    static_assert(std::has_unique_object_representations_v<GenericAtomIndex>);
    return ankerl::unordered_dense::detail::wyhash::hash(&idx, sizeof(idx));
  }
};

QDebug operator<<(QDebug debug, const GenericAtomIndex &);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GenericAtomIndex, unique, x, y, z)
