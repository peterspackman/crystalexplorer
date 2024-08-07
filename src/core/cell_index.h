#pragma once
#include <ankerl/unordered_dense.h>

struct CellIndex {
  int x{0};
  int y{0};
  int z{0};

  inline bool operator==(const CellIndex &rhs) const {
    return std::tie(x, y, z) ==
           std::tie(rhs.x, rhs.y, rhs.z);
  }

  inline bool operator<(const CellIndex &rhs) const {
    return std::tie(x, y, z) <
           std::tie(rhs.x, rhs.y, rhs.z);
  }

  inline bool operator>(const CellIndex &rhs) const {
    return std::tie(x, z, z) >
           std::tie(rhs.x, rhs.y, rhs.z);
  }
};

struct CellIndexHash {
  using is_avalanching = void;
  [[nodiscard]] auto
  operator()(CellIndex const &idx) const noexcept -> uint64_t {
    static_assert(std::has_unique_object_representations_v<CellIndex>);
    return ankerl::unordered_dense::detail::wyhash::hash(&idx, sizeof(idx));
  }
};

struct CellIndexPairHash {
  using is_avalanching = void;
  [[nodiscard]] auto
  operator()(const std::pair<CellIndex, CellIndex>& p) const noexcept -> uint64_t {
    return ankerl::unordered_dense::detail::wyhash::hash(&p, sizeof(p));
  }
};

using CellIndexSet = ankerl::unordered_dense::set<CellIndex, CellIndexHash>;
using CellIndexPairSet = ankerl::unordered_dense::set<std::pair<CellIndex, CellIndex>, CellIndexPairHash>;
