#pragma once
#include <QDebug>
#include <ankerl/unordered_dense.h>

struct CrystalFragmentIndex {
  int u{0};
  int h{0};
  int k{0};
  int l{0};

  inline bool operator==(const CrystalFragmentIndex &rhs) const {
    return std::tie(u, h, k, l) == std::tie(rhs.u, rhs.h, rhs.k, rhs.l);
  }

  inline bool operator<(const CrystalFragmentIndex &rhs) const {
    return std::tie(u, h, k, l) < std::tie(rhs.u, rhs.h, rhs.k, rhs.l);
  }

  inline bool operator>(const CrystalFragmentIndex &rhs) const {
    return std::tie(u, h, k, l) > std::tie(rhs.u, rhs.h, rhs.k, rhs.l);
  }
};

struct CrystalFragmentIndexHash {
  using is_avalanching = void;
  [[nodiscard]] auto
  operator()(CrystalFragmentIndex const &idx) const noexcept -> uint64_t {
    static_assert(
        std::has_unique_object_representations_v<CrystalFragmentIndex>);
    return ankerl::unordered_dense::detail::wyhash::hash(&idx, sizeof(idx));
  }
};

struct CrystalFragmentPair {
  CrystalFragmentIndex a;
  CrystalFragmentIndex b;
};

struct CrystalFragmentPairHash {
  using is_avalanching = void;
  [[nodiscard]] auto
  operator()(CrystalFragmentPair const &idx) const noexcept -> uint64_t {
    static_assert(
        std::has_unique_object_representations_v<CrystalFragmentPair>);
    return ankerl::unordered_dense::detail::wyhash::hash(&idx, sizeof(idx));
  }
};

QDebug operator<<(QDebug debug, const CrystalFragmentIndex &);
QDebug operator<<(QDebug debug, const CrystalFragmentPair &);
