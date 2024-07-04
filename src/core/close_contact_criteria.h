#pragma once
#include <occ/core/linear_algebra.h>
#include <ankerl/unordered_dense.h>
#include <cstdint>
#include <limits>
#include <vector>
#include <QColor>

struct CloseContactPair {
  int d{0};
  int a{0};
};

struct CloseContactCriteria {
  ankerl::unordered_dense::set<int> donors;
  ankerl::unordered_dense::set<int> acceptors;
  double minDistance{0.0};
  double maxDistance{std::numeric_limits<double>::max()};
  bool includeIntra{false};
  bool show{false};
  QColor color{Qt::black};

  [[nodiscard]] bool isDonor(int atomicNumber) const;
  [[nodiscard]] bool isAcceptor(int atomicNumber) const;

  [[nodiscard]] std::vector<CloseContactPair>
  filter(const occ::Mat3N &positions,
         const occ::IVec &atomicNumbers,
         const std::vector<std::pair<int, int>> &covalentBonds,
         const std::vector<std::pair<int, int>> &candidateBonds) const;
};
