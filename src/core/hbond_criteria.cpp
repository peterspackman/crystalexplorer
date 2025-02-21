#include "hbond_criteria.h"
#include "elementdata.h"
#include <occ/core/element.h>
#include <tuple>

bool HBondCriteria::operator==(const HBondCriteria &rhs) const {
  if (std::tie(minAngle, maxAngle, minDistance, maxDistance) !=
      std::tie(rhs.minAngle, rhs.maxAngle, rhs.minDistance, rhs.maxDistance)) {
    return false;
  }

  if (std::tie(includeIntra, vdwOffset, vdwCriteria) !=
      std::tie(rhs.includeIntra, rhs.vdwOffset, rhs.vdwCriteria)) {
    return false;
  }
  if (color != rhs.color)
    return false;
  if (rhs.donors != donors)
    return false;
  if (rhs.acceptors != acceptors)
    return false;
  return true;
}

inline double calculateAngle(const occ::Vec3 &a, const occ::Vec3 &b,
                             const occ::Vec3 &c) {
  Eigen::Vector3d ba = a - b;
  Eigen::Vector3d bc = c - b;
  return std::acos(ba.dot(bc) / (ba.norm() * bc.norm())) * 180.0 / M_PI;
}

bool HBondCriteria::isDonor(int atomicNumber) const {
  return donors.empty() || donors.contains(atomicNumber);
}

bool HBondCriteria::isAcceptor(int atomicNumber) const {
  return acceptors.empty() || acceptors.contains(atomicNumber);
}

std::vector<HBondTriple> HBondCriteria::filter(
    const occ::Mat3N &positions, const occ::IVec &atomicNumbers,
    const std::vector<std::pair<int, int>> &covalentBonds,
    const std::vector<std::pair<int, int>> &candidateBonds) const {

  std::vector<HBondTriple> filteredBonds;

  ankerl::unordered_dense::map<int, int> hydrogenBonds;
  for (const auto &bond : covalentBonds) {
    if (atomicNumbers(bond.first) == 1) {
      hydrogenBonds[bond.first] = bond.second;
    } else if (atomicNumbers(bond.second) == 1) {
      hydrogenBonds[bond.second] = bond.first;
    }
  }

  auto getVdw = [](int n) {
    Element *el = ElementData::elementFromAtomicNumber(n);
    double radius = 0.0;
    if (el) {
      radius = el->vdwRadius();
    } else {
      auto element = occ::core::Element(n);
      radius = element.van_der_waals_radius();
    }
    return (radius > 0.0) ? radius : 2.0;
  };

  double radiusH = getVdw(1);

  for (const auto &bond : candidateBonds) {
    HBondTriple hbond;

    if (atomicNumbers(bond.first) == 1) {
      hbond.h = bond.first;
      hbond.a = bond.second;
    } else if (atomicNumbers(bond.second) == 1) {
      hbond.h = bond.second;
      hbond.a = bond.first;
    } else {
      continue;
    }

    auto it = hydrogenBonds.find(hbond.h);
    if (it == hydrogenBonds.end())
      continue; // Skip if hydrogen is not bonded to any atom
    hbond.d = it->second;

    if (!isDonor(atomicNumbers(hbond.d)) ||
        !isAcceptor(atomicNumbers(hbond.a))) {
      continue;
    }

    double distance = (positions.col(hbond.a) - positions.col(hbond.h)).norm();

    if (vdwCriteria) {
      double radiusA = getVdw(atomicNumbers(hbond.a));
      if (distance > (radiusH + radiusA - vdwOffset))
        continue;
    } else if (distance < minDistance || distance > maxDistance) {
      continue;
    }

    double angle = calculateAngle(
        positions.col(hbond.d), positions.col(hbond.h), positions.col(hbond.a));

    if (angle < minAngle || angle > maxAngle) {
      continue;
    }

    filteredBonds.push_back(hbond);
  }

  return filteredBonds;
}
