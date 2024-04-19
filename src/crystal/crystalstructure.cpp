#include "crystalstructure.h"
#include <QDebug>

using VertexDesc =
    typename occ::core::graph::PeriodicBondGraph::VertexDescriptor;
using EdgeDesc = typename occ::core::graph::PeriodicBondGraph::EdgeDescriptor;
using Connection = occ::core::graph::PeriodicEdge::Connection;
using Vertex = occ::core::graph::PeriodicVertex;
using Edge = occ::core::graph::PeriodicEdge;
using occ::core::graph::PeriodicBondGraph;

CrystalStructure::CrystalStructure(QObject *parent)
    : ChemicalStructure(parent) {}


QString edgeTypeString(Connection conn) {
    switch(conn) {
    case Connection::CloseContact: return "CC";
    case Connection::HydrogenBond: return "HB";
    case Connection::CovalentBond: return "COV";
    }
}

template <typename T, typename Pred>
void filtered_connectivity_traversal_with_cell_offset(
    const PeriodicBondGraph &g, VertexDesc source, T &func, Pred &pred,
    MillerIndex sourceHKL) {
  ankerl::unordered_dense::set<VertexDesc> visited;
  std::queue<std::tuple<VertexDesc, VertexDesc, EdgeDesc, MillerIndex>> store;
  store.push({source, source, 0, sourceHKL});
  const auto &adjacency = g.adjacency_list();
  const auto &edges = g.edges();
  while (!store.empty()) {
    auto [s, predecessor, edge, hkl] = store.front();
    store.pop();
    if (visited.contains(s))
      continue;

    visited.insert(s);
    func(s, predecessor, edge, hkl);
    for (const auto &kv : adjacency.at(s)) {
      const auto &edge = edges.at(kv.second);

      if (pred(kv.second)) {
        MillerIndex nextHKL{hkl.h + edge.h, hkl.k + edge.k, hkl.l + edge.l};
        store.push({kv.first, s, kv.second, nextHKL});
      }
    }
  }
}

void CrystalStructure::updateBondGraph() {
  const auto &g = m_crystal.unit_cell_connectivity();
  const auto &edges = g.edges();
  const auto &adjacency = g.adjacency_list();

  m_covalentBonds.clear();
  m_hydrogenBonds.clear();
  m_vdwContacts.clear();
  m_fragments.clear();
  m_fragmentForAtom.clear();
  m_fragmentForAtom.resize(numberOfAtoms(), -1);

  ankerl::unordered_dense::set<int> visited;
  size_t currentFragmentIndex{0};

  auto covalentVisitor = [&](const VertexDesc &v, const VertexDesc &prev,
                             const EdgeDesc &e, const MillerIndex &hkl) {
    auto &idxs = m_fragments[currentFragmentIndex];
    CrystalIndex atomIdx{static_cast<int>(v), hkl};
    auto location = m_atomMap.find(atomIdx);
    if (location == m_atomMap.end()) {
      return;
    }
    int idx = location->second;
    if (testAtomFlag(idx, AtomFlag::Contact))
      return;
    visited.insert(idx);
    m_fragmentForAtom[idx] = currentFragmentIndex;
    idxs.push_back(idx);
    if (prev != v) {
    }
  };

  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };

  for (int i = 0; i < numberOfAtoms(); i++) {
    if (visited.contains(i) || testAtomFlag(i, AtomFlag::Contact))
      continue;
    m_fragments.push_back({});
    VertexDesc uc_vertex = m_unitCellOffsets[i].unitCellOffset;
    filtered_connectivity_traversal_with_cell_offset(
        g, uc_vertex, covalentVisitor, covalentPredicate,
        m_unitCellOffsets[i].hkl);
    currentFragmentIndex++;
  }

  for (const auto &[sourceCrystalIndex, sourceAtomIndex] : m_atomMap) {
    const VertexDesc sourceVertex =
        static_cast<VertexDesc>(sourceCrystalIndex.unitCellOffset);
    if (testAtomFlag(sourceAtomIndex, AtomFlag::Contact))
      continue;
    for (const auto &[neighborVertexDesc, edgeDesc] :
         adjacency.at(sourceVertex)) {
      const auto &edge = edges.at(edgeDesc);
      MillerIndex targetHKL = {sourceCrystalIndex.hkl.h + edge.h,
                               sourceCrystalIndex.hkl.k + edge.k,
                               sourceCrystalIndex.hkl.l + edge.l};

      CrystalIndex targetIndex{static_cast<int>(neighborVertexDesc), targetHKL};
      const auto targetLoc = m_atomMap.find(targetIndex);
      if (targetLoc != m_atomMap.end()) {
        int targetAtomIdx = targetLoc->second;
        switch (edge.connectionType) {
        case Connection::CovalentBond:
          m_covalentBonds.push_back({sourceAtomIndex, targetAtomIdx});
          break;
        case Connection::HydrogenBond:
          m_hydrogenBonds.push_back({sourceAtomIndex, targetAtomIdx});
          break;
        case Connection::CloseContact:
          m_vdwContacts.push_back({sourceAtomIndex, targetAtomIdx});
          break;
        }
      }
    }
  }
}

void CrystalStructure::resetAtomsAndBonds(bool toSelection) {
  std::vector<QString> elementSymbols;
  std::vector<occ::Vec3> positions;
  std::vector<QString> newLabels;

  if (toSelection) {
    std::vector<CrystalIndex> newUnitCellOffsets;
    ankerl::unordered_dense::map<CrystalIndex, int, CrystalIndexHash> newAtomMap;

    int numAtoms = 0;
    for (int i = 0; i < numberOfAtoms(); i++) {
      if (!atomFlagsSet(i, AtomFlag::Selected))
        continue;
      positions.push_back(atomicPositions().col(i));
      elementSymbols.push_back(QString::fromStdString(
          occ::core::Element(atomicNumbers()(i)).symbol()));
      newLabels.push_back(labels()[i]);
      CrystalIndex idx = m_unitCellOffsets[i];
      newUnitCellOffsets.push_back(idx);
      newAtomMap.insert({idx, numAtoms});
      numAtoms++;
    }
    m_unitCellOffsets = newUnitCellOffsets;
    m_atomMap = newAtomMap;
  } else {
    const auto &asym = m_crystal.asymmetric_unit();
    occ::Mat3N cartesianPos = m_crystal.to_cartesian(asym.positions);
    const auto &asymLabels = asym.labels;
    const auto &asymElements = asym.atomic_numbers;
    m_unitCellOffsets.resize(asymElements.size());
    m_atomMap.clear();
    for (int i = 0; i < m_crystal.asymmetric_unit().size(); i++) {
      positions.push_back(cartesianPos.col(i));
      elementSymbols.push_back(
          QString::fromStdString(occ::core::Element(asymElements(i)).symbol()));
      if (asymLabels.size() > i) {
        newLabels.push_back(QString::fromStdString(asymLabels[i]));
      }
      MillerIndex hkl = {static_cast<int>(floor(asym.positions(0, i))),
                         static_cast<int>(floor(asym.positions(1, i))),
                         static_cast<int>(floor(asym.positions(2, i)))};
      m_unitCellOffsets[i] = {i, hkl};
      m_atomMap.insert({CrystalIndex{i, hkl}, i});
    }
  }
  setAtoms(elementSymbols, positions, newLabels);
  updateBondGraph();
}

void CrystalStructure::setOccCrystal(const OccCrystal &crystal) {
  m_crystal = crystal;

  resetAtomsAndBonds();
}

QString CrystalStructure::chemicalFormula() const {
  auto formula = m_crystal.asymmetric_unit().chemical_formula();
  return QString::fromStdString(formula);
}

int CrystalStructure::fragmentIndexForAtom(int atomIndex) const {
  return m_fragmentForAtom[atomIndex];
}

const std::vector<std::pair<int, int>> &
CrystalStructure::hydrogenBonds() const {
  return m_hydrogenBonds;
}

const std::vector<std::pair<int, int>> &CrystalStructure::vdwContacts() const {
  return m_vdwContacts;
}

const std::vector<std::pair<int, int>> &
CrystalStructure::covalentBonds() const {
  return m_covalentBonds;
}

const std::pair<int, int> &CrystalStructure::atomsForBond(int bondIndex) const {
  return m_covalentBonds.at(bondIndex);
}

const std::vector<int> &
CrystalStructure::atomsForFragment(int fragIndex) const {
  return m_fragments.at(fragIndex);
}

std::vector<GenericAtomIndex> CrystalStructure::atomIndicesForFragment(int fragmentIndex) const {
    std::vector<GenericAtomIndex> result;
    if(fragmentIndex < 0 || fragmentIndex > m_fragments.size()) return result;
    for(int i : m_fragments[fragmentIndex]) {
	auto offset = m_unitCellOffsets[i];
	result.push_back(GenericAtomIndex{offset.unitCellOffset, offset.hkl.h, offset.hkl.k, offset.hkl.l});
    }
    return result;
}

void CrystalStructure::addAtomsByCrystalIndex(
    std::vector<CrystalIndex> &indices, const AtomFlags &flags) {
  const auto &uc_atoms = m_crystal.unit_cell_atoms();
  occ::IVec nums(indices.size());
  occ::Mat3N pos(3, indices.size());
  const auto &asym = m_crystal.asymmetric_unit();
  std::vector<QString> l;
  const int numAtomsBefore = numberOfAtoms();

  {
    int i = 0;
    for (const auto &idx : indices) {
      nums(i) = uc_atoms.atomic_numbers(idx.unitCellOffset);
      pos.col(i) = uc_atoms.frac_pos.col(idx.unitCellOffset) +
                   occ::Vec3(idx.hkl.h, idx.hkl.k, idx.hkl.l);
      QString label = "";
      auto asymIdx = uc_atoms.asym_idx(idx.unitCellOffset);
      if (asymIdx < asym.labels.size()) {
        label = QString::fromStdString(asym.labels[asymIdx]);
      }
      l.push_back(label);
      m_unitCellOffsets.push_back(idx);
      m_atomMap.insert({idx, numAtomsBefore + i});
      i++;
    }
    pos = m_crystal.to_cartesian(pos);
  }

  std::vector<occ::Vec3> positionsToAdd;
  std::vector<QString> elementSymbols;

  for (int i = 0; i < nums.rows(); i++) {
    elementSymbols.push_back(
        QString::fromStdString(occ::core::Element(nums(i)).symbol()));
    positionsToAdd.push_back(pos.col(i));
  }
  addAtoms(elementSymbols, positionsToAdd, l);
  const int numAtomsAfter = numberOfAtoms();
  for (int i = numAtomsBefore; i < numAtomsAfter; i++) {
    setAtomFlags(i, flags);
  }
}

void CrystalStructure::addVanDerWaalsContactAtoms() {
  CrystalIndexSet atomsToShow;
  const auto &g = m_crystal.unit_cell_connectivity();
  const auto &adjacency = g.adjacency_list();
  const auto &edges = g.edges();
  for (const auto &[sourceCrystalIndex, sourceAtomIndex] : m_atomMap) {
    const VertexDesc sourceVertex =
        static_cast<VertexDesc>(sourceCrystalIndex.unitCellOffset);
    // don't add vdw contacts for vdw contact atoms
    if (atomFlagsSet(sourceAtomIndex, AtomFlag::Contact))
      continue;

    for (const auto &[neighborVertexDesc, edgeDesc] :
         adjacency.at(sourceVertex)) {
      const auto &edge = edges.at(edgeDesc);
      MillerIndex targetHKL = {sourceCrystalIndex.hkl.h + edge.h,
                               sourceCrystalIndex.hkl.k + edge.k,
                               sourceCrystalIndex.hkl.l + edge.l};

      CrystalIndex targetIndex{static_cast<int>(neighborVertexDesc), targetHKL};
      if (m_atomMap.find(targetIndex) != m_atomMap.end())
        continue;
      switch (edge.connectionType) {
      case Connection::CovalentBond:
        break;
      case Connection::HydrogenBond:
        atomsToShow.insert(targetIndex);
        break;
      case Connection::CloseContact:
        atomsToShow.insert(targetIndex);
        break;
      }
    }
  }

  std::vector<CrystalIndex> indices(atomsToShow.begin(), atomsToShow.end());
  addAtomsByCrystalIndex(indices, AtomFlag::Contact);
}

void CrystalStructure::deleteAtoms(const std::vector<int> &atomIndices) {
  const int originalNumAtoms = numberOfAtoms();

  ankerl::unordered_dense::set<int> uniqueIndices;
  for (int idx : std::as_const(atomIndices)) {
    if (idx < originalNumAtoms)
      uniqueIndices.insert(idx);
  }

  std::vector<QString> newElementSymbols;
  std::vector<occ::Vec3> newPositions;
  std::vector<QString> newLabels;
  std::vector<CrystalIndex> unitCellOffsets;
  m_unitCellOffsets.clear();
  m_atomMap.clear();
  const auto &currentPositions = atomicPositions();
  const auto &currentLabels = labels();
  const auto &currentNumbers = atomicNumbers();

  int atomIndex = 0;
  for (int i = 0; i < originalNumAtoms; i++) {
    if (uniqueIndices.contains(i))
      continue;
    unitCellOffsets.push_back(m_unitCellOffsets[i]);
    m_atomMap.insert({m_unitCellOffsets[i], atomIndex});
    newPositions.push_back(currentPositions.col(i));
    newElementSymbols.push_back(
        QString::fromStdString(occ::core::Element(currentNumbers(i)).symbol()));
    if (currentLabels.size() > i) {
      newLabels.push_back(currentLabels[i]);
    }
    atomIndex++;
  }
  m_unitCellOffsets = unitCellOffsets;
  setAtoms(newElementSymbols, newPositions, newLabels);
}

void CrystalStructure::removeVanDerWaalsContactAtoms() {

  std::vector<int> indicesToRemove;

  for (int i = 0; i < numberOfAtoms(); i++) {
    if (testAtomFlag(i, AtomFlag::Contact)) {
      indicesToRemove.push_back(i);
    }
  }
  deleteAtoms(indicesToRemove);
}

void CrystalStructure::deleteFragmentContainingAtomIndex(int atomIndex) {
  const auto &fragmentIndex = fragmentIndexForAtom(atomIndex);
  if (fragmentIndex < 0)
    return;
  const auto &fragIndices = atomsForFragment(fragmentIndex);
  if (fragIndices.size() == 0)
    return;

  deleteAtoms(fragIndices);
  updateBondGraph();
}

void CrystalStructure::setShowVanDerWaalsContactAtoms(bool state) {
  if (state) {
    addVanDerWaalsContactAtoms();
    updateBondGraph();
  } else {
    removeVanDerWaalsContactAtoms();
    updateBondGraph();
  }
}

void CrystalStructure::completeFragmentContaining(int atomIndex) {
  if (atomIndex < 0 || atomIndex >= numberOfAtoms())
    return;
  bool haveContactAtoms = anyAtomHasFlags(AtomFlag::Contact);
  bool fragmentWasSelected{false};
  if (!atomFlagsSet(atomIndex, AtomFlag::Contact)) {
    fragmentWasSelected = atomsHaveFlags(
        atomsForFragment(fragmentIndexForAtom(atomIndex)), AtomFlag::Selected);
  }
  setAtomFlag(atomIndex, AtomFlag::Contact, false);

  const auto &g = m_crystal.unit_cell_connectivity();
  const auto &edges = g.edges();
  ankerl::unordered_dense::set<CrystalIndex, CrystalIndexHash> atomsToAdd;

  auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                     const EdgeDesc &e, const MillerIndex &hkl) {
    CrystalIndex atomIdx{static_cast<int>(v), hkl};
    auto location = m_atomMap.find(atomIdx);
    if (location != m_atomMap.end()) {
      setAtomFlag(location->second, AtomFlag::Contact, false);
      return;
    }
    atomsToAdd.insert(atomIdx);
  };

  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };

  VertexDesc uc_vertex = m_unitCellOffsets[atomIndex].unitCellOffset;
  filtered_connectivity_traversal_with_cell_offset(
      g, uc_vertex, visitor, covalentPredicate,
      m_unitCellOffsets[atomIndex].hkl);

  std::vector<CrystalIndex> indices(atomsToAdd.begin(), atomsToAdd.end());
  addAtomsByCrystalIndex(indices, fragmentWasSelected ? AtomFlag::Selected
                                                      : AtomFlag::NoFlag);
  if (haveContactAtoms)
    addVanDerWaalsContactAtoms();
  updateBondGraph();
}

bool CrystalStructure::hasIncompleteFragments() const {
  const auto &g = m_crystal.unit_cell_connectivity();
  const auto &edges = g.edges();

  int currentFragmentIndex{0};
  bool incomplete{false};

  auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                     const EdgeDesc &e, const MillerIndex &hkl) {
    CrystalIndex atomIdx{static_cast<int>(v), hkl};
    auto location = m_atomMap.find(atomIdx);
    if (location == m_atomMap.end()) {
      incomplete = true;
      return;
    }
  };

  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };

  for (currentFragmentIndex = 0; currentFragmentIndex < m_fragments.size();
       currentFragmentIndex++) {
    if (m_fragments[currentFragmentIndex].size() == 0)
      continue;
    int atomIndex = m_fragments[currentFragmentIndex][0];
    VertexDesc uc_vertex = m_unitCellOffsets[atomIndex].unitCellOffset;
    filtered_connectivity_traversal_with_cell_offset(
        g, uc_vertex, visitor, covalentPredicate,
        m_unitCellOffsets[atomIndex].hkl);
    if (incomplete)
      return true;
  }
  return false;
}

void CrystalStructure::deleteIncompleteFragments() {

  const auto &g = m_crystal.unit_cell_connectivity();
  const auto &edges = g.edges();

  ankerl::unordered_dense::set<int> fragmentIndicesToDelete;
  int currentFragmentIndex{0};

  auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                     const EdgeDesc &e, const MillerIndex &hkl) {
    CrystalIndex atomIdx{static_cast<int>(v), hkl};
    auto location = m_atomMap.find(atomIdx);
    if (location == m_atomMap.end()) {
      fragmentIndicesToDelete.insert(currentFragmentIndex);
      return;
    }
  };

  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };

  for (currentFragmentIndex = 0; currentFragmentIndex < m_fragments.size();
       currentFragmentIndex++) {
    if (m_fragments[currentFragmentIndex].size() == 0)
      continue;
    int atomIndex = m_fragments[currentFragmentIndex][0];
    VertexDesc uc_vertex = m_unitCellOffsets[atomIndex].unitCellOffset;
    filtered_connectivity_traversal_with_cell_offset(
        g, uc_vertex, visitor, covalentPredicate,
        m_unitCellOffsets[atomIndex].hkl);
  }

  std::vector<int> atomIndicesToDelete;
  for (const auto &fragIndex : fragmentIndicesToDelete) {
    const auto &fragAtoms = m_fragments[fragIndex];
    atomIndicesToDelete.insert(atomIndicesToDelete.end(), fragAtoms.begin(),
                               fragAtoms.end());
  }

  if (atomIndicesToDelete.size() > 0) {
    std::sort(atomIndicesToDelete.begin(), atomIndicesToDelete.end());
    deleteAtoms(atomIndicesToDelete);
    updateBondGraph();
  }
}

void CrystalStructure::completeAllFragments() {
  bool haveContactAtoms = anyAtomHasFlags(AtomFlag::Contact);

  const auto &g = m_crystal.unit_cell_connectivity();
  const auto &edges = g.edges();
  ankerl::unordered_dense::set<CrystalIndex, CrystalIndexHash> atomsToAdd;

  auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                     const EdgeDesc &e, const MillerIndex &hkl) {
    CrystalIndex atomIdx{static_cast<int>(v), hkl};
    auto location = m_atomMap.find(atomIdx);
    if (location != m_atomMap.end()) {
      setAtomFlag(location->second, AtomFlag::Contact, false);
      return;
    }
    atomsToAdd.insert(atomIdx);
  };

  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };

  for (int atomIndex = 0; atomIndex < numberOfAtoms(); atomIndex++) {
    VertexDesc uc_vertex = m_unitCellOffsets[atomIndex].unitCellOffset;
    filtered_connectivity_traversal_with_cell_offset(
        g, uc_vertex, visitor, covalentPredicate,
        m_unitCellOffsets[atomIndex].hkl);
  }
  std::vector<CrystalIndex> indices(atomsToAdd.begin(), atomsToAdd.end());
  addAtomsByCrystalIndex(indices, AtomFlag::NoFlag);
  if (haveContactAtoms)
    addVanDerWaalsContactAtoms();
  updateBondGraph();
}

void CrystalStructure::packUnitCells(
    const QPair<QVector3D, QVector3D> &limits) {

  std::vector<QString> elementSymbols;
  std::vector<occ::Vec3> positions;
  std::vector<QString> labels;

  occ::Vec3 lowerFrac(limits.first[0], limits.first[1], limits.first[2]);
  occ::Vec3 upperFrac(limits.second[0], limits.second[1], limits.second[2]);

  occ::crystal::HKL lower{static_cast<int>(std::floor(lowerFrac[0])),
                          static_cast<int>(std::floor(lowerFrac[1])),
                          static_cast<int>(std::floor(lowerFrac[2]))};
  occ::crystal::HKL upper{static_cast<int>(std::ceil(upperFrac[0]) - 1),
                          static_cast<int>(std::ceil(upperFrac[1]) - 1),
                          static_cast<int>(std::ceil(upperFrac[2]) - 1)};

  auto slab = m_crystal.slab(lower, upper);

  const auto &asymLabels = m_crystal.asymmetric_unit().labels;
  m_unitCellOffsets.clear();
  m_unitCellOffsets.reserve(slab.size());
  m_atomMap.clear();
  int n_uc = m_crystal.unit_cell_atoms().size();
  for (int i = 0; i < slab.size(); i++) {
    if ((slab.frac_pos.col(i).array() < lowerFrac.array()).any())
      continue;
    if ((slab.frac_pos.col(i).array() > upperFrac.array()).any())
      continue;
    positions.push_back(slab.cart_pos.col(i));
    elementSymbols.push_back(QString::fromStdString(
        occ::core::Element(slab.atomic_numbers(i)).symbol()));
    if (asymLabels.size() > i) {
      labels.push_back(QString::fromStdString(asymLabels[slab.asym_idx(i)]));
    }
    MillerIndex hkl = {static_cast<int>(floor(slab.frac_pos(0, i))),
                       static_cast<int>(floor(slab.frac_pos(1, i))),
                       static_cast<int>(floor(slab.frac_pos(2, i)))};
    int ucOffset = i % n_uc;
    m_unitCellOffsets.push_back({ucOffset, hkl});
    m_atomMap.insert({CrystalIndex{ucOffset, hkl}, i});
  }
  setAtoms(elementSymbols, positions, labels);
  updateBondGraph();
}

void CrystalStructure::expandAtomsWithinRadius(float radius, bool selected) {

  if (selected) {
    resetAtomsAndBonds(true);
    for (int atomIndex = 0; atomIndex < numberOfAtoms(); atomIndex++) {
      setAtomFlag(atomIndex,
                  AtomFlag::Selected); // the call to resetAtomsAndBonds
                                       // unselects everything
    }
    if (std::abs(radius) < 1e-3)
      return;
  }

  auto uc_regions = m_crystal.unit_cell_atom_surroundings(radius);
  ankerl::unordered_dense::set<CrystalIndex, CrystalIndexHash> atomsToAdd;
  for (int atomIndex = 0; atomIndex < numberOfAtoms(); atomIndex++) {
    const auto &crystal_index = m_unitCellOffsets[atomIndex];
    const auto &region = uc_regions[crystal_index.unitCellOffset];
    for (int i = 0; i < region.size(); i++) {
      int h = static_cast<float>(std::floor(region.frac_pos(0, i))) +
              crystal_index.hkl.h;
      int k = static_cast<float>(std::floor(region.frac_pos(1, i))) +
              crystal_index.hkl.k;
      int l = static_cast<float>(std::floor(region.frac_pos(2, i))) +
              crystal_index.hkl.l;
      CrystalIndex idx{region.uc_idx(i), {h, k, l}};
      atomsToAdd.insert(idx);
    }
  }
  AtomFlags flags;
  flags.setFlag(AtomFlag::Selected, true);
  std::vector<CrystalIndex> atomIndexes(atomsToAdd.begin(), atomsToAdd.end());

  // TODO remove atoms not within radius
  if (atomIndexes.size() > 0) {
    addAtomsByCrystalIndex(atomIndexes, flags);
    updateBondGraph();
  }
}

std::vector<GenericAtomIndex> CrystalStructure::atomsWithFlags(const AtomFlags &flags) const {

    ankerl::unordered_dense::set<GenericAtomIndex, GenericAtomIndexHash> selected_idxs;

    for (int i = 0; i < numberOfAtoms(); i++) {
	if (atomFlagsSet(i, flags)) {
	    const auto &offset = m_unitCellOffsets[i];
	    selected_idxs.insert({offset.unitCellOffset, offset.hkl.h, offset.hkl.k, offset.hkl.l});
	}
    }

    std::vector<GenericAtomIndex> res(selected_idxs.begin(), selected_idxs.end());
    return res;
}

std::vector<GenericAtomIndex> CrystalStructure::atomsSurroundingAtomsWithFlags(const AtomFlags &flags, float radius) const {

    ankerl::unordered_dense::set<GenericAtomIndex, GenericAtomIndexHash> selected_idxs;

    for (int i = 0; i < numberOfAtoms(); i++) {
	if (atomFlagsSet(i, flags)) {
	    const auto &offset = m_unitCellOffsets[i];
	    selected_idxs.insert({offset.unitCellOffset, offset.hkl.h, offset.hkl.k, offset.hkl.l});
	}
    }

    ankerl::unordered_dense::set<GenericAtomIndex, GenericAtomIndexHash> unique_idxs;

    auto uc_neighbors = m_crystal.unit_cell_atom_surroundings(radius);

    for (const auto &idx: selected_idxs) {
	const auto &region = uc_neighbors[idx.unique];
	for(int n = 0; n < region.size(); n++) {
	    int h = region.hkl(0, n) + idx.x;
	    int k = region.hkl(1, n) + idx.y;
	    int l = region.hkl(2, n) + idx.z;

	    auto candidate = GenericAtomIndex{
		region.uc_idx(n), h, k, l
	    };
	    if(selected_idxs.find(candidate) == selected_idxs.end()) {
		unique_idxs.insert(candidate);
	    }
	}
    }

    std::vector<GenericAtomIndex> res(unique_idxs.begin(), unique_idxs.end());
    return res;
}


occ::IVec CrystalStructure::atomicNumbersForIndices(const std::vector<GenericAtomIndex> &idxs) const {
    const auto &uc_atoms = m_crystal.unit_cell_atoms();
    occ::IVec result(idxs.size());
    for(int i = 0; i < idxs.size(); i++) {
	const auto &idx = idxs[i];
	result(i) = uc_atoms.atomic_numbers(idx.unique);
    }
    return result;
}

occ::Mat3N CrystalStructure::atomicPositionsForIndices(const std::vector<GenericAtomIndex> &idxs) const {
    const auto &uc_atoms = m_crystal.unit_cell_atoms();
    occ::Mat3N result(3, idxs.size());
    for(int i = 0; i < idxs.size(); i++) {
	const auto &idx = idxs[i];
	result.col(i) = uc_atoms.frac_pos.col(idx.unique) + occ::Vec3(idx.x, idx.y, idx.z);
    }
    result = m_crystal.to_cartesian(result);
    return result;
}
