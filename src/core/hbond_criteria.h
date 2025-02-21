#pragma once
#include <occ/core/linear_algebra.h>
#include <ankerl/unordered_dense.h>
#include <cstdint>
#include <limits>
#include <vector>
#include <QColor>

struct HBondTriple {
  int d{0};
  int h{0};
  int a{0};
};

struct HBondCriteria {
  ankerl::unordered_dense::set<int> donors;
  ankerl::unordered_dense::set<int> acceptors;
  double minAngle{0.0};
  double maxAngle{360.0};
  double minDistance{0.0};
  double maxDistance{std::numeric_limits<double>::max()};
  bool includeIntra{false};

  double vdwOffset{0.0};
  bool vdwCriteria{false};
  QColor color{Qt::black};

  [[nodiscard]] bool isDonor(int atomicNumber) const;
  [[nodiscard]] bool isAcceptor(int atomicNumber) const;

  [[nodiscard]] bool operator==(const HBondCriteria &rhs) const;
  [[nodiscard]] inline bool operator!=(const HBondCriteria &rhs) const {
    return !(*this == rhs);
  }

  [[nodiscard]] std::vector<HBondTriple>
  filter(const occ::Mat3N &positions,
         const occ::IVec &atomicNumbers,
         const std::vector<std::pair<int, int>> &covalentBonds,
         const std::vector<std::pair<int, int>> &candidateBonds) const;
};
