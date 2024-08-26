#include "close_contact_criteria.h"
#include "elementdata.h"
#include <occ/core/element.h>

bool CloseContactCriteria::isDonor(int atomicNumber) const {
  return donors.empty() || donors.contains(atomicNumber);
}

bool CloseContactCriteria::isAcceptor(int atomicNumber) const {
  return acceptors.empty() || acceptors.contains(atomicNumber);
}

std::vector<CloseContactPair> CloseContactCriteria::filter(
    const occ::Mat3N &positions, const occ::IVec &atomicNumbers,
    const std::vector<std::pair<int, int>> &covalentBonds,
    const std::vector<std::pair<int, int>> &candidateBonds) const {

  std::vector<CloseContactPair> filteredBonds;

  for (const auto &bond : candidateBonds) {
    CloseContactPair pair{bond.first, bond.second};

    if (!isDonor(atomicNumbers(pair.d)) ||
        !isAcceptor(atomicNumbers(pair.a))) {
      continue;
    }

    double distance = (positions.col(pair.d) - positions.col(pair.a)).norm();

    if (distance < minDistance || distance > maxDistance) {
      continue;
    }

    filteredBonds.push_back(pair);
  }

  return filteredBonds;
}
