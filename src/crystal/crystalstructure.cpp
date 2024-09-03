#include "crystalstructure.h"
#include <iostream>
#include <occ/core/kabsch.h>

using VertexDesc =
    typename occ::core::graph::PeriodicBondGraph::VertexDescriptor;
using EdgeDesc = typename occ::core::graph::PeriodicBondGraph::EdgeDescriptor;
using Connection = occ::core::graph::PeriodicEdge::Connection;
using Vertex = occ::core::graph::PeriodicVertex;
using Edge = occ::core::graph::PeriodicEdge;
using occ::core::graph::PeriodicBondGraph;

using GenericAtomIndexSet =
    ankerl::unordered_dense::set<GenericAtomIndex, GenericAtomIndexHash>;

CrystalStructure::CrystalStructure(QObject *parent)
    : ChemicalStructure(parent) {}

QString edgeTypeString(Connection conn) {
  switch (conn) {
  case Connection::CloseContact:
    return "CC";
  case Connection::HydrogenBond:
    return "HB";
  case Connection::CovalentBond:
    return "COV";
  }
  return "?";
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
  m_fragmentForAtom.resize(numberOfAtoms(), FragmentIndex{-1});

  ankerl::unordered_dense::set<int> visited;
  int currentFragmentIndex{0};

  std::vector<std::vector<int>> fragments;

  auto covalentVisitor = [&](const VertexDesc &v, const VertexDesc &prev,
                             const EdgeDesc &e, const MillerIndex &hkl) {
    auto &idxs = fragments[currentFragmentIndex];
    GenericAtomIndex atomIdx{static_cast<int>(v), hkl.h, hkl.k, hkl.l};
    auto location = m_atomMap.find(atomIdx);
    if (location == m_atomMap.end()) {
      return;
    }
    int idx = location->second;
    if (testAtomFlag(atomIdx, AtomFlag::Contact))
      return;
    visited.insert(idx);
    m_fragmentForAtom[idx] = FragmentIndex{currentFragmentIndex};
    idxs.push_back(idx);
  };

  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };

  for (int i = 0; i < numberOfAtoms(); i++) {
    auto offset = m_unitCellOffsets[i];
    VertexDesc uc_vertex = offset.unique;
    int h = offset.x;
    int k = offset.y;
    int l = offset.z;
    int idx = m_atomMap[offset];

    if (visited.contains(idx) || testAtomFlag(offset, AtomFlag::Contact))
      continue;

    fragments.push_back({});
    filtered_connectivity_traversal_with_cell_offset(
        g, uc_vertex, covalentVisitor, covalentPredicate, {h, k, l});
    currentFragmentIndex++;
  }

  for (const auto &[sourceCrystalIndex, sourceAtomIndex] : m_atomMap) {
    const VertexDesc sourceVertex =
        static_cast<VertexDesc>(sourceCrystalIndex.unique);
    if (testAtomFlag(sourceCrystalIndex, AtomFlag::Contact))
      continue;
    for (const auto &[neighborVertexDesc, edgeDesc] :
         adjacency.at(sourceVertex)) {
      const auto &edge = edges.at(edgeDesc);
      MillerIndex targetHKL = {sourceCrystalIndex.x + edge.h,
                               sourceCrystalIndex.y + edge.k,
                               sourceCrystalIndex.z + edge.l};

      GenericAtomIndex targetIndex{static_cast<int>(neighborVertexDesc),
                                   targetHKL.h, targetHKL.k, targetHKL.l};
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

  for (const auto &idxs : fragments) {
    std::vector<GenericAtomIndex> g;
    std::transform(idxs.begin(), idxs.end(), std::back_inserter(g),
                   [&](int i) { return m_unitCellOffsets[i]; });

    std::sort(g.begin(), g.end());
    auto frag = makeFragment(g);
    m_fragments.insert({frag.index, frag});
    for (const auto &idx : g) {
      m_fragmentForAtom[genericIndexToIndex(idx)] = frag.index;
    }
  }
}

void CrystalStructure::resetAtomsAndBonds(bool toSelection) {
  std::vector<QString> elementSymbols;
  std::vector<occ::Vec3> positions;
  std::vector<QString> newLabels;

  if (toSelection) {
    std::vector<GenericAtomIndex> newUnitCellOffsets;
    ankerl::unordered_dense::map<GenericAtomIndex, int, GenericAtomIndexHash>
        newAtomMap;

    int numAtoms = 0;
    for (int i = 0; i < numberOfAtoms(); i++) {
      if (!atomFlagsSet(m_unitCellOffsets[i], AtomFlag::Selected))
        continue;
      positions.push_back(atomicPositions().col(i));
      elementSymbols.push_back(QString::fromStdString(
          occ::core::Element(atomicNumbers()(i)).symbol()));
      newLabels.push_back(labels()[i]);
      GenericAtomIndex idx = m_unitCellOffsets[i];
      newUnitCellOffsets.push_back(idx);
      newAtomMap.insert({idx, numAtoms});
      numAtoms++;
    }
    clearAtoms();
    m_unitCellOffsets.clear();
    m_atomMap.clear();
    addAtomsByCrystalIndex(newUnitCellOffsets);
  } else {
    clearAtoms();
    const auto &asym = m_crystal.asymmetric_unit();
    occ::Mat3N cartesianPos = m_crystal.to_cartesian(asym.positions);
    m_unitCellOffsets.clear();
    m_atomMap.clear();

    std::vector<GenericAtomIndex> indices;

    ankerl::unordered_dense::set<int> included;
    for (const auto &[fragIndex, frag] : m_symmetryUniqueFragments) {
      const auto &asym = frag.asymmetricUnitIndices;
      for (int i = 0; i < frag.atomIndices.size(); i++) {
        if (included.contains(asym(i)))
          continue;
        indices.push_back(frag.atomIndices[i]);
        included.insert(asym(i));
      }
    }
    addAtomsByCrystalIndex(indices);
  }
  updateBondGraph();
}

inline occ::Mat6N computeUnitCellAtomAdps(const OccCrystal &crystal) {
  const auto &uc_atoms = crystal.unit_cell_atoms();
  const auto &asym = crystal.asymmetric_unit();
  occ::Mat6N result = occ::Mat6N::Zero(6, uc_atoms.size());

  if (asym.adps.cols() < asym.size())
    return result;

  occ::Vec6 tmp;
  for (int i = 0; i < result.cols(); i++) {
    auto symop = occ::crystal::SymmetryOperation(uc_atoms.symop(i));
    auto asym_idx = uc_atoms.asym_idx(i);
    tmp = asym.adps.col(asym_idx);
    result.col(i) = symop.rotate_adp(tmp);
  }
  return crystal.unit_cell().to_cartesian_adp(result);
}

Fragment CrystalStructure::makeFragmentFromOccMolecule(
    const occ::core::Molecule &mol) const {
  std::vector<GenericAtomIndex> idxs;
  const auto &uc_idx = mol.unit_cell_idx();
  const auto &uc_shift = mol.unit_cell_atom_shift();
  const auto &asym_idx = mol.asymmetric_unit_idx();

  for (int i = 0; i < uc_idx.rows(); i++) {
    idxs.push_back(GenericAtomIndex{uc_idx(i), uc_shift(0, i), uc_shift(1, i),
                                    uc_shift(2, i)});
  }
  std::sort(idxs.begin(), idxs.end());

  Fragment result;
  result.atomIndices = idxs;
  result.atomicNumbers = atomicNumbersForIndices(idxs);
  result.positions = atomicPositionsForIndices(idxs);
  result.asymmetricUnitIndices = occ::IVec(idxs.size());
  for (int i = 0; i < idxs.size(); i++) {
    result.asymmetricUnitIndices(i) = asym_idx(i);
  }
  return result;
}

void CrystalStructure::setOccCrystal(const OccCrystal &crystal) {
  m_crystal = crystal;

  m_symmetryUniqueFragments.clear();

  const auto &uc_atoms = m_crystal.unit_cell_atoms();
  std::vector<FragmentIndex> asymmetricMoleculeIndices;

  for (const auto &mol : m_crystal.symmetry_unique_molecules()) {
    FragmentIndex idx{mol.unit_cell_molecule_idx(), 0, 0, 0};
    asymmetricMoleculeIndices.push_back(idx);
    auto frag = makeFragmentFromOccMolecule(mol);
    frag.asymmetricFragmentIndex = idx;
    frag.index = idx;
    m_symmetryUniqueFragments.insert({idx, frag});
  }

  m_unitCellFragments.clear();
  m_unitCellAtomFragments.clear();
  for (const auto &mol : m_crystal.unit_cell_molecules()) {
    FragmentIndex idx{mol.unit_cell_molecule_idx(), 0, 0, 0};
    qDebug() << "Unit cell mol: " << idx;
    auto frag = makeFragmentFromOccMolecule(mol);

    frag.asymmetricFragmentIndex =
        asymmetricMoleculeIndices[mol.asymmetric_molecule_idx()];
    const auto &asymFrag =
        m_symmetryUniqueFragments[frag.asymmetricFragmentIndex];
    frag.index = idx;
    getTransformation(asymFrag.atomIndices, frag.atomIndices,
                      frag.asymmetricFragmentTransform);
    m_unitCellFragments.insert({idx, frag});
    qDebug() << "Inserting unit cell fragment:" << frag.index;
    for (const auto &atomIndex : frag.atomIndices) {
      qDebug() << "AtomIndex: " << atomIndex;
      FragmentIndex thisIndex = idx;
      thisIndex.h = -atomIndex.x;
      thisIndex.k = -atomIndex.y;
      thisIndex.l = -atomIndex.z;
      m_unitCellAtomFragments[atomIndex.unique] = thisIndex;
      qDebug() << "Fragment for unit cell index" << atomIndex.unique
               << thisIndex;
    }
  }

  auto adps = computeUnitCellAtomAdps(crystal);
  for (int i = 0; i < adps.cols(); i++) {
    m_unitCellAdps.insert(
        {i, AtomicDisplacementParameters(adps(0, i), adps(1, i), adps(2, i),
                                         adps(3, i), adps(4, i), adps(5, i))});
  }
  buildDimerMappingTable();
  resetAtomsAndBonds();
}

QString CrystalStructure::chemicalFormula(bool richText) const {
  // TODO rich text
  auto formula = m_crystal.asymmetric_unit().chemical_formula();
  return QString::fromStdString(formula);
}

FragmentIndex CrystalStructure::fragmentIndexForAtom(int atomIndex) const {
  return m_fragmentForAtom[atomIndex];
}

QColor CrystalStructure::getFragmentColor(FragmentIndex fragmentIndex) const {
  const auto kv = m_fragments.find(fragmentIndex);
  if (kv != m_fragments.end()) {
    return kv->second.color;
  }
  return Qt::white;
}

void CrystalStructure::setFragmentColor(FragmentIndex fragment,
                                        const QColor &color) {
  auto kv = m_fragments.find(fragment);
  if (kv != m_fragments.end()) {
    kv->second.color = color;
    emit atomsChanged();
  }
}

void CrystalStructure::setAllFragmentColors(const QColor &color) {
  for (auto &[fragIndex, frag] : m_fragments) {
    frag.color = color;
  }
  emit atomsChanged();
}

FragmentIndex
CrystalStructure::fragmentIndexForAtom(GenericAtomIndex idx) const {
  const auto loc = m_atomMap.find(idx);
  if (loc != m_atomMap.end()) {
    return m_fragmentForAtom[loc->second];
  }
  return FragmentIndex{-1};
}

std::vector<HBondTriple>
CrystalStructure::hydrogenBonds(const HBondCriteria &criteria) const {
  return criteria.filter(atomicPositions(), atomicNumbers(), m_covalentBonds,
                         m_hydrogenBonds);
}

std::vector<CloseContactPair>
CrystalStructure::closeContacts(const CloseContactCriteria &criteria) const {
  return criteria.filter(atomicPositions(), atomicNumbers(), m_covalentBonds,
                         m_vdwContacts);
}

const std::vector<std::pair<int, int>> &
CrystalStructure::covalentBonds() const {
  return m_covalentBonds;
}

const std::pair<int, int> &CrystalStructure::atomsForBond(int bondIndex) const {
  return m_covalentBonds.at(bondIndex);
}

std::vector<GenericAtomIndex>
CrystalStructure::atomIndicesForFragment(FragmentIndex fragmentIndex) const {
  const auto kv = m_fragments.find(fragmentIndex);
  if (kv != m_fragments.end())
    return kv->second.atomIndices;
  return {};
}

void CrystalStructure::addAtomsByCrystalIndex(
    const std::vector<GenericAtomIndex> &unfilteredIndices,
    const AtomFlags &flags) {

  // filter out already existing indices
  std::vector<GenericAtomIndex> indices;
  indices.reserve(unfilteredIndices.size());
  setFlagForAtoms(unfilteredIndices, AtomFlag::Contact, false);

  std::copy_if(unfilteredIndices.begin(), unfilteredIndices.end(),
               std::back_inserter(indices), [&](const GenericAtomIndex &index) {
                 return m_atomMap.find(index) == m_atomMap.end();
               });
  const auto &uc_atoms = m_crystal.unit_cell_atoms();
  occ::IVec nums(indices.size());
  occ::Mat3N pos(3, indices.size());
  const auto &asym = m_crystal.asymmetric_unit();
  std::vector<QString> l;
  const int numAtomsBefore = numberOfAtoms();

  {
    int i = 0;
    for (const auto &idx : indices) {
      nums(i) = uc_atoms.atomic_numbers(idx.unique);
      pos.col(i) =
          uc_atoms.frac_pos.col(idx.unique) + occ::Vec3(idx.x, idx.y, idx.z);
      QString label = "";
      auto asymIdx = uc_atoms.asym_idx(idx.unique);
      if (asymIdx < asym.labels.size() && asymIdx >= 0) {
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
    setAtomFlags(m_unitCellOffsets[i], flags);
  }
}

void CrystalStructure::addVanDerWaalsContactAtoms() {
  GenericAtomIndexSet atomsToShow;
  const auto &g = m_crystal.unit_cell_connectivity();
  const auto &adjacency = g.adjacency_list();
  const auto &edges = g.edges();
  for (const auto &[sourceCrystalIndex, sourceAtomIndex] : m_atomMap) {
    const VertexDesc sourceVertex =
        static_cast<VertexDesc>(sourceCrystalIndex.unique);
    // don't add vdw contacts for vdw contact atoms
    if (atomFlagsSet(sourceCrystalIndex, AtomFlag::Contact))
      continue;

    for (const auto &[neighborVertexDesc, edgeDesc] :
         adjacency.at(sourceVertex)) {
      const auto &edge = edges.at(edgeDesc);
      MillerIndex targetHKL = {sourceCrystalIndex.x + edge.h,
                               sourceCrystalIndex.y + edge.k,
                               sourceCrystalIndex.z + edge.l};

      GenericAtomIndex targetIndex{static_cast<int>(neighborVertexDesc),
                                   targetHKL.h, targetHKL.k, targetHKL.l};
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

  std::vector<GenericAtomIndex> indices(atomsToShow.begin(), atomsToShow.end());
  addAtomsByCrystalIndex(indices, AtomFlag::Contact);
}

void CrystalStructure::deleteAtoms(const std::vector<GenericAtomIndex> &atoms) {
  std::vector<int> offsets;
  offsets.reserve(atoms.size());
  for (const auto &idx : atoms) {
    const auto kv = m_atomMap.find(idx);
    if (kv != m_atomMap.end())
      offsets.push_back(kv->second);
  }
  deleteAtomsByOffset(offsets);
  updateBondGraph();
}

void CrystalStructure::deleteAtomsByOffset(
    const std::vector<int> &atomIndices) {
  const int originalNumAtoms = numberOfAtoms();

  ankerl::unordered_dense::set<int> uniqueIndices;
  for (int idx : std::as_const(atomIndices)) {
    if (idx < originalNumAtoms)
      uniqueIndices.insert(idx);
  }

  std::vector<QString> newElementSymbols;
  std::vector<occ::Vec3> newPositions;
  std::vector<QString> newLabels;
  std::vector<GenericAtomIndex> unitCellOffsets;
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
    if (testAtomFlag(m_unitCellOffsets[i], AtomFlag::Contact)) {
      indicesToRemove.push_back(i);
    }
  }
  deleteAtomsByOffset(indicesToRemove);
}

void CrystalStructure::deleteFragmentContainingAtomIndex(int atomIndex) {
  const auto &fragmentIndex = fragmentIndexForAtom(atomIndex);
  if (fragmentIndex.u < 0)
    return;
  const auto &fragIndices = atomIndicesForFragment(fragmentIndex);
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

void CrystalStructure::completeFragmentContaining(GenericAtomIndex index) {

  bool haveContactAtoms = anyAtomHasFlags(AtomFlag::Contact);
  bool fragmentWasSelected{false};

  FragmentIndex fragmentIndex = m_unitCellAtomFragments[index.unique];
  fragmentIndex.h += index.x;
  fragmentIndex.k += index.y;
  fragmentIndex.l += index.z;

  const Fragment frag = makeFragmentFromFragmentIndex(fragmentIndex);

  for (const auto &idx : frag.atomIndices) {
    qDebug() << idx;
  }

  addAtomsByCrystalIndex(frag.atomIndices, AtomFlag::NoFlag);

  if (haveContactAtoms) {
    addVanDerWaalsContactAtoms();
  }
  updateBondGraph();
}
void CrystalStructure::completeFragmentContaining(int atomIndex) {
  if (atomIndex < 0 || atomIndex >= numberOfAtoms())
    return;
  GenericAtomIndex idx = indexToGenericIndex(atomIndex);
  completeFragmentContaining(idx);
}

bool CrystalStructure::hasIncompleteFragments() const {
  const auto &g = m_crystal.unit_cell_connectivity();
  const auto &edges = g.edges();

  bool incomplete{false};

  auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                     const EdgeDesc &e, const MillerIndex &hkl) {
    GenericAtomIndex atomIdx{static_cast<int>(v), hkl.h, hkl.k, hkl.l};
    auto location = m_atomMap.find(atomIdx);
    if (location == m_atomMap.end()) {
      incomplete = true;
      return;
    }
  };

  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };

  for (const auto &[fragIndex, frag] : m_fragments) {
    if (frag.size() == 0)
      continue;
    const auto &idx = frag.atomIndices[0];
    VertexDesc uc_vertex = idx.unique;
    filtered_connectivity_traversal_with_cell_offset(
        g, uc_vertex, visitor, covalentPredicate, {idx.x, idx.y, idx.z});
    if (incomplete)
      return true;
  }
  return false;
}

bool CrystalStructure::hasIncompleteSelectedFragments() const {
  const auto &g = m_crystal.unit_cell_connectivity();
  const auto &edges = g.edges();

  bool incomplete{false};

  auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                     const EdgeDesc &e, const MillerIndex &hkl) {
    GenericAtomIndex atomIdx{static_cast<int>(v), hkl.h, hkl.k, hkl.l};
    auto location = m_atomMap.find(atomIdx);
    if (location == m_atomMap.end()) {
      incomplete = true;
      return;
    }
  };

  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };

  for (const auto &[fragIndex, frag] : m_fragments) {
    // fragment is length 0 - should not happen
    if (frag.size() == 0)
      continue;

    // ensure fragment is selected
    const auto &fragIndices = frag.atomIndices;
    if (!atomsHaveFlags(fragIndices, AtomFlag::Selected))
      continue;

    const auto &idx = fragIndices[0];
    VertexDesc uc_vertex = idx.unique;
    filtered_connectivity_traversal_with_cell_offset(
        g, uc_vertex, visitor, covalentPredicate, {idx.x, idx.y, idx.z});
    if (incomplete)
      return true;
  }
  return false;
}

std::vector<FragmentIndex> CrystalStructure::completedFragments() const {

  std::vector<FragmentIndex> result;
  const auto &g = m_crystal.unit_cell_connectivity();
  const auto &edges = g.edges();

  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };

  for (const auto &[fragIndex, frag] : m_fragments) {
    if (frag.size() == 0)
      continue;

    bool incomplete{false};

    auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                       const EdgeDesc &e, const MillerIndex &hkl) {
      GenericAtomIndex atomIdx{static_cast<int>(v), hkl.h, hkl.k, hkl.l};
      auto location = m_atomMap.find(atomIdx);
      if (location == m_atomMap.end()) {
        incomplete = true;
        return;
      }
    };

    const auto &idx = frag.atomIndices[0];
    VertexDesc uc_vertex = idx.unique;

    filtered_connectivity_traversal_with_cell_offset(
        g, uc_vertex, visitor, covalentPredicate, {idx.x, idx.y, idx.z});
    if (incomplete)
      continue;
    else
      result.push_back(fragIndex);
  }
  return result;
}

std::vector<FragmentIndex> CrystalStructure::selectedFragments() const {
  std::vector<FragmentIndex> result;
  for (const auto &[fragIndex, frag] : m_fragments) {
    const auto &fragIndices = frag.atomIndices;
    if (fragIndices.size() == 1)
      continue;
    if (atomsHaveFlags(fragIndices, AtomFlag::Selected))
      result.push_back(fragIndex);
  }
  return result;
}

void CrystalStructure::deleteIncompleteFragments() {

  const auto &g = m_crystal.unit_cell_connectivity();
  const auto &edges = g.edges();

  FragmentIndexSet fragmentIndicesToDelete;

  FragmentIndex currentFragmentIndex;
  auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                     const EdgeDesc &e, const MillerIndex &hkl) {
    GenericAtomIndex atomIdx{static_cast<int>(v), hkl.h, hkl.k, hkl.l};
    auto location = m_atomMap.find(atomIdx);
    if (location == m_atomMap.end()) {
      fragmentIndicesToDelete.insert(currentFragmentIndex);
      return;
    }
  };

  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };

  for (const auto &[fragIndex, frag] : m_fragments) {
    if (frag.size() == 0)
      continue;
    currentFragmentIndex = fragIndex;
    const auto &idx = frag.atomIndices[0];
    VertexDesc uc_vertex = idx.unique;
    filtered_connectivity_traversal_with_cell_offset(
        g, uc_vertex, visitor, covalentPredicate, {idx.x, idx.y, idx.z});
  }

  std::vector<GenericAtomIndex> atomIndicesToDelete;
  for (const auto &fragIndex : fragmentIndicesToDelete) {
    const auto &fragAtoms = m_fragments[fragIndex].atomIndices;
    atomIndicesToDelete.insert(atomIndicesToDelete.end(), fragAtoms.begin(),
                               fragAtoms.end());
  }

  if (atomIndicesToDelete.size() > 0) {
    deleteAtoms(atomIndicesToDelete);
    updateBondGraph();
  }
}

void CrystalStructure::completeAllFragments() {
  bool haveContactAtoms = anyAtomHasFlags(AtomFlag::Contact);
  const auto selectedAtoms = atomsWithFlags(AtomFlag::Selected);

  const auto &g = m_crystal.unit_cell_connectivity();
  const auto &edges = g.edges();
  GenericAtomIndexSet atomsToAdd;

  auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                     const EdgeDesc &e, const MillerIndex &hkl) {
    GenericAtomIndex atomIdx{static_cast<int>(v), hkl.h, hkl.k, hkl.l};
    auto location = m_atomMap.find(atomIdx);
    if (location != m_atomMap.end()) {
      setAtomFlag(atomIdx, AtomFlag::Contact, false);
      return;
    }
    atomsToAdd.insert(atomIdx);
  };

  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };

  for (int atomIndex = 0; atomIndex < numberOfAtoms(); atomIndex++) {
    VertexDesc uc_vertex = m_unitCellOffsets[atomIndex].unique;
    const auto &offset = m_unitCellOffsets[atomIndex];

    filtered_connectivity_traversal_with_cell_offset(
        g, uc_vertex, visitor, covalentPredicate,
        {offset.x, offset.y, offset.z});
  }
  std::vector<GenericAtomIndex> indices(atomsToAdd.begin(), atomsToAdd.end());
  addAtomsByCrystalIndex(indices, AtomFlag::NoFlag);
  if (haveContactAtoms)
    addVanDerWaalsContactAtoms();
  updateBondGraph();

  // ensure selection doesn't change
  for (const auto &idx : selectedAtoms) {
    setAtomFlag(idx, AtomFlag::Selected);
  }
}

void CrystalStructure::packUnitCells(
    const QPair<QVector3D, QVector3D> &limits) {
  clearAtoms();

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

  m_unitCellOffsets.clear();
  m_atomMap.clear();

  std::vector<GenericAtomIndex> indices;
  indices.reserve(slab.size());

  for (int i = 0; i < slab.size(); i++) {
    if ((slab.frac_pos.col(i).array() < lowerFrac.array()).any())
      continue;
    if ((slab.frac_pos.col(i).array() > upperFrac.array()).any())
      continue;
    indices.push_back(GenericAtomIndex{slab.uc_idx(i), slab.hkl(0, i),
                                       slab.hkl(1, i), slab.hkl(2, i)});
  }
  addAtomsByCrystalIndex(indices);
  updateBondGraph();
}

void CrystalStructure::expandAtomsWithinRadius(float radius, bool selected) {

  std::vector<GenericAtomIndex> selectedAtoms;
  if (selected) {
    // resets to selection
    resetAtomsAndBonds(true);

    selectedAtoms = m_unitCellOffsets;
    setFlagForAtoms(selectedAtoms, AtomFlag::Selected);
    if (std::abs(radius) < 1e-3)
      return;
  }

  auto uc_regions = m_crystal.unit_cell_atom_surroundings(radius);
  GenericAtomIndexSet atomsToAdd;
  for (int atomIndex = 0; atomIndex < numberOfAtoms(); atomIndex++) {
    const auto &crystal_index = m_unitCellOffsets[atomIndex];
    const auto &region = uc_regions[crystal_index.unique];
    for (int i = 0; i < region.size(); i++) {
      int h = region.hkl(0, i) + crystal_index.x;
      int k = region.hkl(1, i) + crystal_index.y;
      int l = region.hkl(2, i) + crystal_index.z;
      GenericAtomIndex idx{region.uc_idx(i), h, k, l};
      atomsToAdd.insert(idx);
    }
  }

  // set flags here for new atoms
  AtomFlags flags;
  std::vector<GenericAtomIndex> atomIndexes(atomsToAdd.begin(),
                                            atomsToAdd.end());

  // TODO remove atoms not within radius
  if (atomIndexes.size() > 0) {
    addAtomsByCrystalIndex(atomIndexes, flags);
    updateBondGraph();
  }
  setFlagForAtoms(selectedAtoms, AtomFlag::Selected);
}

std::vector<GenericAtomIndex>
CrystalStructure::atomsWithFlags(const AtomFlags &flags, bool set) const {

  GenericAtomIndexSet selected_idxs;

  for (const auto &offset : m_unitCellOffsets) {
    bool check = atomFlagsSet(offset, flags);
    if ((set && check) || (!set && !check)) {
      selected_idxs.insert(offset);
    }
  }

  std::vector<GenericAtomIndex> res(selected_idxs.begin(), selected_idxs.end());
  return res;
}

std::vector<GenericAtomIndex> CrystalStructure::atomsSurroundingAtoms(
    const std::vector<GenericAtomIndex> &idxs, float radius) const {

  GenericAtomIndexSet idx_set(idxs.begin(), idxs.end());
  GenericAtomIndexSet unique_idxs;

  auto uc_neighbors = m_crystal.unit_cell_atom_surroundings(radius);

  for (const auto &idx : idx_set) {
    const auto &region = uc_neighbors[idx.unique];
    for (int n = 0; n < region.size(); n++) {
      int h = region.hkl(0, n) + idx.x;
      int k = region.hkl(1, n) + idx.y;
      int l = region.hkl(2, n) + idx.z;

      auto candidate = GenericAtomIndex{region.uc_idx(n), h, k, l};
      if (!idx_set.contains(candidate)) {
        unique_idxs.insert(candidate);
      }
    }
  }

  std::vector<GenericAtomIndex> res(unique_idxs.begin(), unique_idxs.end());
  return res;
}

std::vector<GenericAtomIndex>
CrystalStructure::atomsSurroundingAtomsWithFlags(const AtomFlags &flags,
                                                 float radius) const {

  GenericAtomIndexSet selected_idxs;

  for (const auto &offset : m_unitCellOffsets) {
    if (atomFlagsSet(offset, flags)) {
      selected_idxs.insert(offset);
    }
  }

  GenericAtomIndexSet unique_idxs;

  auto uc_neighbors = m_crystal.unit_cell_atom_surroundings(radius);

  for (const auto &idx : selected_idxs) {
    const auto &region = uc_neighbors[idx.unique];
    for (int n = 0; n < region.size(); n++) {
      int h = region.hkl(0, n) + idx.x;
      int k = region.hkl(1, n) + idx.y;
      int l = region.hkl(2, n) + idx.z;

      auto candidate = GenericAtomIndex{region.uc_idx(n), h, k, l};
      if (selected_idxs.find(candidate) == selected_idxs.end()) {
        unique_idxs.insert(candidate);
      }
    }
  }

  std::vector<GenericAtomIndex> res(unique_idxs.begin(), unique_idxs.end());
  return res;
}

occ::IVec CrystalStructure::atomicNumbersForIndices(
    const std::vector<GenericAtomIndex> &idxs) const {
  const auto &uc_atoms = m_crystal.unit_cell_atoms();
  occ::IVec result(idxs.size());
  for (int i = 0; i < idxs.size(); i++) {
    const auto &idx = idxs[i];
    result(i) = uc_atoms.atomic_numbers(idx.unique);
  }
  return result;
}

std::vector<QString> CrystalStructure::labelsForIndices(
    const std::vector<GenericAtomIndex> &idxs) const {
  const auto &uc_atoms = m_crystal.unit_cell_atoms();
  const auto &asym = m_crystal.asymmetric_unit();
  std::vector<QString> result;
  result.reserve(idxs.size());
  for (int i = 0; i < idxs.size(); i++) {
    const auto &idx = idxs[i];
    int asym_index = uc_atoms.asym_idx(idx.unique);
    result.push_back(QString::fromStdString(asym.labels[asym_index]));
  }
  return result;
}

occ::Mat3N CrystalStructure::atomicPositionsForIndices(
    const std::vector<GenericAtomIndex> &idxs) const {
  const auto &uc_atoms = m_crystal.unit_cell_atoms();
  occ::Mat3N result(3, idxs.size());
  for (int i = 0; i < idxs.size(); i++) {
    const auto &idx = idxs[i];
    result.col(i) =
        uc_atoms.frac_pos.col(idx.unique) + occ::Vec3(idx.x, idx.y, idx.z);
  }
  result = m_crystal.to_cartesian(result);
  return result;
}

const FragmentMap &CrystalStructure::symmetryUniqueFragments() const {
  return m_symmetryUniqueFragments;
}

Fragment::State CrystalStructure::getSymmetryUniqueFragmentState(
    FragmentIndex fragmentIndex) const {
  const auto kv = m_symmetryUniqueFragments.find(fragmentIndex);
  if (kv == m_symmetryUniqueFragments.end())
    return {};
  else
    return kv->second.state;
}

void CrystalStructure::setSymmetryUniqueFragmentState(
    FragmentIndex fragmentIndex, Fragment::State state) {
  auto kv = m_symmetryUniqueFragments.find(fragmentIndex);
  if (kv == m_symmetryUniqueFragments.end())
    return;
  kv->second.state = state;
}

FragmentIndex
CrystalStructure::findUnitCellFragment(const Fragment &frag) const {
  // assume sorted
  auto findCommonOffset = [](const Fragment &a, const Fragment &b) {
    FragmentIndex result{-1, 0, 0, 0};
    const auto &idxA = a.atomIndices;
    const auto &idxB = b.atomIndices;
    if (idxA.size() != idxB.size())
      return result;

    if (idxA[0].unique != idxB[0].unique)
      return result;
    result.h = idxB[0].x - idxA[0].x;
    result.k = idxB[0].y - idxA[0].y;
    result.l = idxB[0].z - idxA[0].z;
    for (int i = 1; i < idxA.size(); i++) {
      if (idxA[i].unique != idxB[i].unique)
        return result;
      if ((idxB[i].x - idxA[i].x) != result.h)
        return result;
      if ((idxB[i].y - idxA[i].y) != result.k)
        return result;
      if ((idxB[i].z - idxA[i].z) != result.l)
        return result;
    }
    result.u = 0;
    return result;
  };

  for (const auto &[fragIndex, candidate] : m_unitCellFragments) {
    auto offset = findCommonOffset(candidate, frag);
    if (offset.u >= 0) {
      offset.u = fragIndex.u;
      return offset;
    }
  }
  qDebug() << "No Matching unit cell fragment!";
  return FragmentIndex{-1, 0, 0, 0};
}

Fragment
CrystalStructure::makeFragmentFromFragmentIndex(FragmentIndex idx) const {
  FragmentIndex unitCellIndex{idx.u, 0, 0, 0};
  // TODO add error checking
  Fragment result = m_unitCellFragments.at(unitCellIndex);
  for (auto &atomIndex : result.atomIndices) {
    atomIndex.x += idx.h;
    atomIndex.y += idx.k;
    atomIndex.z += idx.l;
  }

  result.positions = atomicPositionsForIndices(result.atomIndices);
  result.index = idx;
  occ::Vec3 translation_frac(result.index.h, result.index.k, result.index.l);
  Eigen::Translation<double, 3> t(m_crystal.to_cartesian(translation_frac));
  result.asymmetricFragmentTransform *= t;
  return result;
}

Fragment CrystalStructure::makeFragment(
    const std::vector<GenericAtomIndex> &idxs) const {
  Fragment result;
  result.atomIndices = idxs;
  std::sort(result.atomIndices.begin(), result.atomIndices.end());
  result.atomicNumbers = atomicNumbersForIndices(idxs);
  result.positions = atomicPositionsForIndices(idxs);

  result.index = findUnitCellFragment(result);
  const FragmentIndex ucIndex = {result.index.u, 0, 0, 0};

  const auto kv = m_unitCellFragments.find(ucIndex);

  if (kv != m_unitCellFragments.end()) {
    const auto &ucFrag = kv->second;
    result.asymmetricFragmentIndex = ucFrag.asymmetricFragmentIndex;
    // translation from unit cell transformation
    occ::Vec3 translation_frac(result.index.h, result.index.k, result.index.l);
    Eigen::Translation<double, 3> t(m_crystal.to_cartesian(translation_frac));
    result.asymmetricFragmentTransform *= t;
  } else {
    const auto &uc_atoms = m_crystal.unit_cell_atoms();
    std::tie(result.asymmetricFragmentIndex,
             result.asymmetricFragmentTransform) = findUniqueFragment(idxs);
    result.asymmetricUnitIndices = occ::IVec(idxs.size());
    for (int i = 0; i < idxs.size(); i++) {
      result.asymmetricUnitIndices(i) = uc_atoms.asym_idx(i);
    }
  }
  return result;
}

const FragmentMap &CrystalStructure::getFragments() const {
  return m_fragments;
}

std::vector<GenericAtomIndex>
CrystalStructure::getAtomIndicesUnderTransformation(
    const std::vector<GenericAtomIndex> &idxs,
    const Eigen::Isometry3d &transform) const {
  std::vector<GenericAtomIndex> result;
  occ::Mat3N pos =
      (transform.rotation() * atomicPositionsForIndices(idxs)).colwise() +
      transform.translation();
  occ::Mat3N fracPos = m_crystal.to_fractional(pos);

  const auto &uc_atoms = m_crystal.unit_cell_atoms();

  for (int i = 0; i < fracPos.cols(); ++i) {
    occ::Vec3 frac = fracPos.col(i);

    double minDistance = std::numeric_limits<double>::max();
    int closestAtomIndex = -1;
    Eigen::Vector3i cellOffset;

    for (size_t j = 0; j < uc_atoms.size(); ++j) {
      Eigen::Vector3d diff = frac - uc_atoms.frac_pos.col(j);
      Eigen::Vector3i currentOffset = diff.array().round().cast<int>();
      Eigen::Vector3d wrappedDiff = diff - currentOffset.cast<double>();

      double distance = wrappedDiff.squaredNorm();
      if (distance < minDistance) {
        minDistance = distance;
        closestAtomIndex = j;
        cellOffset = currentOffset;
      }
    }

    if (closestAtomIndex != -1) {
      if (minDistance > 1e-3)
        qDebug() << "Match has large distance: " << closestAtomIndex
                 << minDistance;
      result.emplace_back(GenericAtomIndex{static_cast<int>(closestAtomIndex),
                                           cellOffset(0), cellOffset(1),
                                           cellOffset(2)});
    }
  }
  return result;
}

int CrystalStructure::genericIndexToIndex(const GenericAtomIndex &idx) const {
  auto location = m_atomMap.find(idx);
  if (location == m_atomMap.end()) {
    return -1;
  }
  return location->second;
}

GenericAtomIndex CrystalStructure::indexToGenericIndex(int idx) const {
  if (idx < 0 || idx >= m_unitCellOffsets.size())
    return GenericAtomIndex{-1};
  return m_unitCellOffsets[idx];
}

void CrystalStructure::setPairInteractionsFromDimerAtoms(
    const QList<QList<PairInteraction *>> &interactions,
    const QList<QList<DimerAtoms>> &offsets) {
  GenericAtomIndexSet idxs;
  for (int i = 0; i < offsets.size(); i++) {
    const auto &molOffsets = offsets[i];
    for (int j = 0; j < molOffsets.size(); j++) {
      const auto &offset = molOffsets[j];
      idxs.insert(offset.a.begin(), offset.a.end());
      idxs.insert(offset.b.begin(), offset.b.end());
    }
  }

  std::vector<GenericAtomIndex> idxsToAdd(idxs.begin(), idxs.end());

  qDebug() << "Adding" << idxsToAdd.size() << "atoms";
  addAtomsByCrystalIndex(idxsToAdd);
  updateBondGraph();

  using occ::crystal::DimerIndex;
  using occ::crystal::DimerIndexHash;

  ankerl::unordered_dense::set<DimerIndex, DimerIndexHash> added;

  const auto &dimerMap = m_dimerMappingTableNoInv;

  auto *p = pairInteractions();
  for (int i = 0; i < interactions.size(); i++) {
    const auto &molInteractions = interactions[i];
    const auto &molOffsets = offsets[i];
    for (int j = 0; j < molInteractions.size(); j++) {
      const auto &offset = molOffsets[j];
      auto fragA = makeFragment(offset.a);
      auto fragB = makeFragment(offset.b);
      auto *pair = molInteractions[j];
      FragmentDimer d(fragA, fragB);
      DimerIndex idx = d.index.toDimerIndex();
      DimerIndex canonical = dimerMap.canonical_dimer_index(idx);
      DimerIndex unique = dimerMap.symmetry_unique_dimer(canonical);
      auto uniquePairIndex = FragmentIndexPair::fromDimerIndex(unique);

      const Fragment uFragA = makeFragmentFromFragmentIndex(uniquePairIndex.a);
      const Fragment uFragB = makeFragmentFromFragmentIndex(uniquePairIndex.b);
      FragmentDimer ud(uFragA, uFragB);
      qDebug() << "Fragment dimer" << d.index;
      qDebug() << "Unique dimer" << ud.index << ud.nearestAtomDistance;

      if (added.find(unique) != added.end()) {
        qDebug() << "Should only import unique dimers:"
                 << FragmentIndexPair::fromDimerIndex(unique);
        continue;
      }

      for (const auto &related : dimerMap.symmetry_related_dimers(idx)) {
        qDebug() << "Related:" << FragmentIndexPair::fromDimerIndex(related);
      }

      added.insert(unique);

      pair_energy::Parameters params;
      params.fragmentDimer = ud;
      params.nearestAtomDistance = d.nearestAtomDistance;
      params.centroidDistance = d.centroidDistance;
      params.hasInversionSymmetry = false;
      pair->setParameters(params);
      p->add(pair);
    }
  }
}

bool CrystalStructure::getTransformation(
    const std::vector<GenericAtomIndex> &from_orig,
    const std::vector<GenericAtomIndex> &to_orig,
    Eigen::Isometry3d &result) const {

  if (from_orig.size() != to_orig.size())
    return false;
  auto from = from_orig;
  auto to = to_orig;
  std::sort(from.begin(), from.end());
  std::sort(to.begin(), to.end());
  auto nums_a = atomicNumbersForIndices(from);
  auto nums_b = atomicNumbersForIndices(to);

  // first check if they're the same elements
  if (!(nums_a.array() == nums_b.array()).all())
    return false;

  auto pos_a = atomicPositionsForIndices(from);
  auto pos_b = atomicPositionsForIndices(to);

  // Convert positions to fractional coordinates
  auto frac_pos_a = m_crystal.to_fractional(pos_a);
  auto frac_pos_b = m_crystal.to_fractional(pos_b);
  occ::Vec3 frac_centroid_b = frac_pos_b.rowwise().mean();

  const auto &symops = m_crystal.space_group().symmetry_operations();

  for (const auto &symop : symops) {
    occ::Mat3N transformed_pos = symop.apply(frac_pos_a);
    occ::Vec3 frac_centroid_a = transformed_pos.rowwise().mean();

    // Calculate the translation between centroids
    occ::Vec3 frac_trans = frac_centroid_b - frac_centroid_a;

    // Apply the translation
    transformed_pos.colwise() += frac_trans;

    // Check if transformed positions match frac_pos_b
    occ::Mat3N diff = transformed_pos - frac_pos_b;
    double rmsd = diff.norm() / std::sqrt(diff.size());

    if (rmsd < 1e-6) { // Tighter tolerance for fractional coordinates
      auto symop_ab = symop.translated(frac_trans);
      qDebug() << QString::fromStdString(symop_ab.to_string());
      // Found a matching symmetry operation
      // Convert back to Cartesian for the result
      Eigen::Matrix3d cart_rot = m_crystal.unit_cell().direct() *
                                 symop_ab.rotation() *
                                 m_crystal.unit_cell().inverse();
      Eigen::Vector3d cart_trans =
          m_crystal.to_cartesian(symop_ab.translation());

      result = Eigen::Isometry3d::Identity();
      result.linear() = cart_rot;
      result.translation() = cart_trans;
      return true;
    }
  }
  return false;
}

CellIndexSet CrystalStructure::occupiedCells() const {
  CellIndexSet result;
  occ::Mat3N pos_frac = m_crystal.to_fractional(atomicPositions());

  auto conv = [](double x) { return static_cast<int>(std::floor(x)); };

  for (int i = 0; i < pos_frac.cols(); i++) {
    CellIndex idx{conv(pos_frac(0, i)), conv(pos_frac(1, i)),
                  conv(pos_frac(2, i))};
    result.insert(idx);
  }
  return result;
}

std::vector<AtomicDisplacementParameters>
CrystalStructure::atomicDisplacementParametersForAtoms(
    const std::vector<GenericAtomIndex> &idxs) const {
  std::vector<AtomicDisplacementParameters> result(idxs.size());
  for (int i = 0; i < idxs.size(); i++) {
    auto kv = m_unitCellAdps.find(idxs[i].unique);
    if (kv != m_unitCellAdps.end()) {
      result[i] = kv->second;
    }
  }
  return result;
}

AtomicDisplacementParameters
CrystalStructure::atomicDisplacementParameters(GenericAtomIndex idx) const {
  auto kv = m_unitCellAdps.find(idx.unique);
  if (kv != m_unitCellAdps.end()) {
    return kv->second;
  }
  return AtomicDisplacementParameters{};
}

void CrystalStructure::buildDimerMappingTable(double maxRadius) {
  m_unitCellDimers = m_crystal.unit_cell_dimers(maxRadius);
  qDebug() << "Building dimer mapping table";
  qDebug() << "Unit cell molecules" << m_unitCellFragments.size();
  qDebug() << "Unique dimers:" << m_unitCellDimers.unique_dimers.size();

  m_dimerMappingTable =
      occ::crystal::DimerMappingTable(m_crystal, m_unitCellDimers, true);
  m_dimerMappingTableNoInv =
      occ::crystal::DimerMappingTable(m_crystal, m_unitCellDimers, false);
  qDebug() << "Built dimer mapping table";
}

FragmentPairs
CrystalStructure::findFragmentPairs(FragmentPairSettings settings) const {
  using occ::crystal::DimerIndex;
  using occ::crystal::DimerIndexHash;
  using occ::crystal::SiteIndex;

  FragmentPairs result;
  result.allowInversion = settings.allowInversion;
  constexpr double tolerance = 1e-1;
  const auto &fragments = getFragments();
  const bool allFragments = settings.keyFragment.u < 0;

  const auto &dimerTable =
      settings.allowInversion ? m_dimerMappingTable : m_dimerMappingTableNoInv;

  std::vector<FragmentIndex> candidateFragments;
  if (allFragments) {
    for (const auto &[idx, frag] : fragments) {
      candidateFragments.push_back(idx);
    }
  } else {
    candidateFragments.push_back(settings.keyFragment);
  }

  ankerl::unordered_dense::set<DimerIndex, DimerIndexHash> symmetryUniquePairs;
  ankerl::unordered_dense::map<DimerIndex, DimerIndex, DimerIndexHash>
      symmetryUniqueMap;

  for (const auto &fragIndexA : candidateFragments) {
    const auto &fragA = fragments.at(fragIndexA);
    for (const auto &[fragIndexB, fragB] : fragments) {
      if (fragIndexA == fragIndexB)
        continue;
      double distance = fragA.nearestAtom(fragB).distance;
      if (distance <= tolerance)
        continue;

      // Create FragmentDimer object
      FragmentDimer d(fragA, fragB);

      DimerIndex dimerIndex = d.index.toDimerIndex();
      if (!dimerTable.have_dimer(dimerIndex)) {
        continue;
      }
      DimerIndex canonicalIndex = dimerTable.canonical_dimer_index(dimerIndex);
      DimerIndex symmetryUniqueDimer =
          dimerTable.symmetry_unique_dimer(canonicalIndex);
      qDebug() << "distance" << d.centroidDistance;
      qDebug() << "Dimer" << FragmentIndexPair::fromDimerIndex(dimerIndex);
      qDebug() << "Canonical"
               << FragmentIndexPair::fromDimerIndex(canonicalIndex);
      qDebug() << "symmetryUnique"
               << FragmentIndexPair::fromDimerIndex(symmetryUniqueDimer);
      symmetryUniqueMap.insert({dimerIndex, symmetryUniqueDimer});
      symmetryUniquePairs.insert(symmetryUniqueDimer);

      FragmentPairs::SymmetryRelatedPair symmetryRelatedPair{d, -1};
      result.pairs[fragA.index].push_back(symmetryRelatedPair);
    }
  }

  for (const auto &dimerIndex : symmetryUniquePairs) {
    auto ab = FragmentIndexPair::fromDimerIndex(dimerIndex);
    auto a = makeFragmentFromFragmentIndex(ab.a);
    auto b = makeFragmentFromFragmentIndex(ab.b);
    FragmentDimer d(a, b);
    qDebug() << "UNIQUE" << d.index << d.nearestAtomDistance << d.centroidDistance;
    qDebug() << "a" << a;
    qDebug() << "b" << b;
    result.uniquePairs.push_back(d);
  }

  // Sort the pairs
  auto fragmentDimerSortFunc = [](const FragmentDimer &a,
                                  const FragmentDimer &b) {
    return a.nearestAtomDistance < b.nearestAtomDistance;
  };

  auto molPairSortFunc = [](const FragmentPairs::SymmetryRelatedPair &a,
                            const FragmentPairs::SymmetryRelatedPair &b) {
    return a.fragments.nearestAtomDistance < b.fragments.nearestAtomDistance;
  };

  std::stable_sort(result.uniquePairs.begin(), result.uniquePairs.end(),
                   fragmentDimerSortFunc);

  for (auto &[idx, vec] : result.pairs) {
    std::stable_sort(vec.begin(), vec.end(), molPairSortFunc);
    for (auto &[d, asym] : vec) {
      auto u = FragmentIndexPair::fromDimerIndex(
          symmetryUniqueMap.at(d.index.toDimerIndex()));
      const auto it =
          std::find_if(result.uniquePairs.begin(), result.uniquePairs.end(),
                       [&u](const FragmentDimer &x) { return x.index == u; });
      asym = std::distance(result.uniquePairs.begin(), it);
    }
  }
  return result;
}

occ::Mat3N CrystalStructure::convertCoordinates(
    const occ::Mat3N &pos, ChemicalStructure::CoordinateConversion conv) const {
  if (conv == ChemicalStructure::CoordinateConversion::FracToCart) {
    return m_crystal.to_cartesian(pos);
  }
  return m_crystal.to_fractional(pos);
}
