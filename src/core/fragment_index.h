#pragma once
#include <QDebug>
#include <nlohmann/json.hpp>
#include <ankerl/unordered_dense.h>
#include <occ/crystal/dimer_mapping_table.h>

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

  inline occ::crystal::SiteIndex toSiteIndex() const {
    return occ::crystal::SiteIndex{u, occ::crystal::HKL{h, k, l}};
  }

  static inline FragmentIndex fromSiteIndex(const occ::crystal::SiteIndex &idx) {
    return FragmentIndex{idx.offset, idx.hkl.h, idx.hkl.k, idx.hkl.l};
  }
};

struct FragmentIndexHash {
  using is_avalanching = void;
  [[nodiscard]] auto
  operator()(FragmentIndex const &idx) const noexcept -> uint64_t {
    static_assert(std::has_unique_object_representations_v<FragmentIndex>);
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

  inline bool equivalent(const FragmentIndexPair &rhs,
                         bool symmetric = true) const {
    // Check if the unique molecule indices match
    if (a.u != rhs.a.u || b.u != rhs.b.u) {
      return false;
    }

    // Calculate the difference in cell offsets
    int dh = b.h - a.h;
    int dk = b.k - a.k;
    int dl = b.l - a.l;

    // Check if the difference in cell offsets is the same
    bool offsetsEqual = (dh == rhs.b.h - rhs.a.h) &&
                        (dk == rhs.b.k - rhs.a.k) && (dl == rhs.b.l - rhs.a.l);

    if (offsetsEqual) {
      return true;
    }

    if (symmetric) {
      // Check if the reversed pair matches
      if (a.u != rhs.b.u || b.u != rhs.a.u) {
        return false;
      }

      // Check the reverse direction
      dh = -dh;
      dk = -dk;
      dl = -dl;

      offsetsEqual = (dh == rhs.b.h - rhs.a.h) && (dk == rhs.b.k - rhs.a.k) &&
                     (dl == rhs.b.l - rhs.a.l);

      return offsetsEqual;
    }

    return false;
  }

  inline occ::crystal::DimerIndex toDimerIndex() const {
    return occ::crystal::DimerIndex{a.toSiteIndex(), b.toSiteIndex()};
  }

  static inline FragmentIndexPair fromDimerIndex(const occ::crystal::DimerIndex &idx) {
    return FragmentIndexPair{
      FragmentIndex::fromSiteIndex(idx.a),
      FragmentIndex::fromSiteIndex(idx.b)
    };
  }
};

struct FragmentIndexPairHash {
  using is_avalanching = void;
  [[nodiscard]] auto
  operator()(FragmentIndexPair const &idx) const noexcept -> uint64_t {
    static_assert(std::has_unique_object_representations_v<FragmentIndexPair>);
    return ankerl::unordered_dense::detail::wyhash::hash(&idx, sizeof(idx));
  }
};

using FragmentIndexSet =
    ankerl::unordered_dense::set<FragmentIndex, FragmentIndexHash>;

QDebug operator<<(QDebug debug, const FragmentIndex &);
QDebug operator<<(QDebug debug, const FragmentIndexPair &);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FragmentIndex, u, h, k, l)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FragmentIndexPair, a, b);
