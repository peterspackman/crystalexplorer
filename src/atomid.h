#pragma once
#include <QDataStream>
#include <tuple>

struct Shift {
  int h{0};
  int k{0};
  int l{0};

  constexpr inline bool operator==(const Shift &rhs) const {
    return h == rhs.h && k == rhs.k && l == rhs.l;
  }

  constexpr inline bool operator!=(const Shift &rhs) const {
    return h != rhs.h || k != rhs.k || l != rhs.l;
  }

  constexpr inline bool operator<(const Shift &rhs) const {
    return std::tie(h, k, l) < std::tie(rhs.h, rhs.k, rhs.l);
  }
};

struct AtomId {
  int unitCellIndex{0};
  Shift shift;

  constexpr inline bool operator==(const AtomId &rhs) const {
    return unitCellIndex == rhs.unitCellIndex && shift == rhs.shift;
  }
  constexpr inline bool operator!=(const AtomId &rhs) const {
    return unitCellIndex != rhs.unitCellIndex || shift != rhs.shift;
  }

  constexpr inline bool operator<(const AtomId &rhs) const {
    return std::tie(unitCellIndex, shift) <
           std::tie(rhs.unitCellIndex, rhs.shift);
  }
};

inline QDataStream &operator<<(QDataStream &ds, const Shift &shift) {
  ds << shift.h << shift.k << shift.l;
  return ds;
}

inline QDataStream &operator>>(QDataStream &ds, Shift &shift) {
  ds >> shift.h;
  ds >> shift.k;
  ds >> shift.l;
  return ds;
}

inline QDataStream &operator<<(QDataStream &ds, const AtomId &atomId) {
  ds << atomId.unitCellIndex << atomId.shift;
  return ds;
}

inline QDataStream &operator>>(QDataStream &ds, AtomId &atomId) {
  ds >> atomId.unitCellIndex;
  ds >> atomId.shift;
  return ds;
}
