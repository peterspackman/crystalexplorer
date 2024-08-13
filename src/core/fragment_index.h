#pragma once
#include <QDebug>
#include <ankerl/unordered_dense.h>

struct FragmentIndex {
  int u{0};
  int h{0};
  int k{0};
  int l{0};

  inline bool operator==(const FragmentIndex &rhs) const {
    return std::tie(u, h, k, l) == std::tie(rhs.u, rhs.h, rhs.k, rhs.l);
  }

  inline bool operator<(const FragmentIndex &rhs) const {
    return std::tie(u, h, k, l) < std::tie(rhs.u, rhs.h, rhs.k, rhs.l);
  }

  inline bool operator>(const FragmentIndex &rhs) const {
    return std::tie(u, h, k, l) > std::tie(rhs.u, rhs.h, rhs.k, rhs.l);
  }

  inline bool operator<=(const FragmentIndex &rhs) const {
    return std::tie(u, h, k, l) <= std::tie(rhs.u, rhs.h, rhs.k, rhs.l);
  }

  inline bool operator>=(const FragmentIndex &rhs) const {
    return std::tie(u, h, k, l) >= std::tie(rhs.u, rhs.h, rhs.k, rhs.l);
  }

  inline bool isValid() const { return u >= 0; }

};

struct FragmentIndexHash {
  using is_avalanching = void;
  [[nodiscard]] auto
  operator()(FragmentIndex const &idx) const noexcept -> uint64_t {
    static_assert(
        std::has_unique_object_representations_v<FragmentIndex>);
    return ankerl::unordered_dense::detail::wyhash::hash(&idx, sizeof(idx));
  }
};

struct FragmentIndexPair {
  FragmentIndex a;
  FragmentIndex b;

  inline bool operator==(const FragmentIndexPair &rhs) const {
    return std::tie(a, b) == std::tie(rhs.a, rhs.b);
  }

  inline bool operator<(const FragmentIndexPair &rhs) const {
    return std::tie(a, b) < std::tie(rhs.a, rhs.b);
  }

  inline bool operator>(const FragmentIndexPair &rhs) const {
    return std::tie(a, b) > std::tie(rhs.a, rhs.b);
  }

  inline bool operator<=(const FragmentIndexPair &rhs) const {
    return std::tie(a, b) <= std::tie(rhs.a, rhs.b);
  }

  inline bool operator>=(const FragmentIndexPair &rhs) const {
    return std::tie(a, b) >= std::tie(rhs.a, rhs.b);
  }

};

struct FragmentIndexPairHash {
  using is_avalanching = void;
  [[nodiscard]] auto
  operator()(FragmentIndexPair const &idx) const noexcept -> uint64_t {
    static_assert(
        std::has_unique_object_representations_v<FragmentIndexPair>);
    return ankerl::unordered_dense::detail::wyhash::hash(&idx, sizeof(idx));
  }
};

using FragmentIndexSet = ankerl::unordered_dense::set<FragmentIndex, FragmentIndexHash>;

QDebug operator<<(QDebug debug, const FragmentIndex &);
QDebug operator<<(QDebug debug, const FragmentIndexPair &);
