#include "periodicstructure.h"
#include <QDebug>
#include <queue>

using VertexDesc = typename occ::core::graph::PeriodicBondGraph::VertexDescriptor;
using EdgeDesc = typename occ::core::graph::PeriodicBondGraph::EdgeDescriptor;
using Connection = occ::core::graph::PeriodicEdge::Connection;
using Vertex = occ::core::graph::PeriodicVertex;
using Edge = occ::core::graph::PeriodicEdge;
using occ::core::graph::PeriodicBondGraph;

using GenericAtomIndexSet = ankerl::unordered_dense::set<GenericAtomIndex, GenericAtomIndexHash>;

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

PeriodicStructure::PeriodicStructure(QObject *parent)
    : ChemicalStructure(parent) {}

// Index conversion methods (common to both 2D and 3D)
int PeriodicStructure::genericIndexToIndex(const GenericAtomIndex &idx) const {
  auto location = m_periodicAtomMap.find(idx);
  if (location == m_periodicAtomMap.end()) {
    return -1;
  }
  return location->second;
}

GenericAtomIndex PeriodicStructure::indexToGenericIndex(int idx) const {
  if (idx < 0 || idx >= m_periodicAtomOffsets.size())
    return GenericAtomIndex{-1};
  return m_periodicAtomOffsets[idx];
}

// Fragment management 
FragmentIndex PeriodicStructure::fragmentIndexForGeneralAtom(GenericAtomIndex index) const {
  // Get the unit cell fragment for the base atom, then translate by the periodic offset
  qDebug() << "Looking up fragment for atom" << index.unique << "at (" << index.x << "," << index.y << "," << index.z << ")";
  
  auto baseAtomIt = m_unitCellAtomFragments.find(index.unique);
  if (baseAtomIt == m_unitCellAtomFragments.end()) {
    qDebug() << "No unit cell fragment found for atom" << index.unique;
    return FragmentIndex{-1}; // Not found
  }
  
  FragmentIndex baseFragIndex = baseAtomIt->second;
  FragmentIndex result{baseFragIndex.u, baseFragIndex.h + index.x, baseFragIndex.k + index.y, baseFragIndex.l + index.z};
  qDebug() << "Found unit cell fragment" << baseFragIndex.u << ", returning fragment" << result.u << "at (" << result.h << "," << result.k << "," << result.l << ")";
  return result;
}

void PeriodicStructure::deleteFragmentContainingAtomIndex(int atomIndex) {
  const auto &fragmentIndex = fragmentIndexForAtom(atomIndex);
  if (fragmentIndex.u < 0)
    return;
  const auto &fragIndices = atomIndicesForFragment(fragmentIndex);
  if (fragIndices.size() == 0)
    return;

  deleteAtoms(fragIndices);
  updateBondGraph();
}

void PeriodicStructure::deleteIncompleteFragments() {
  // Use the same logic as CrystalStructure but with our connectivity
  const auto &g = getUnitCellConnectivity();
  const auto &edges = g.edges();

  FragmentIndexSet fragmentIndicesToDelete;

  FragmentIndex currentFragmentIndex;
  auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                     const EdgeDesc &e, const MillerIndex &hkl) {
    GenericAtomIndex atomIdx{static_cast<int>(v), hkl.h, hkl.k, hkl.l};
    auto location = m_periodicAtomMap.find(atomIdx);
    if (location == m_periodicAtomMap.end()) {
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


void PeriodicStructure::deleteAtoms(const std::vector<GenericAtomIndex> &atoms) {
  std::vector<int> offsets;
  offsets.reserve(atoms.size());
  for (const auto &idx : atoms) {
    const auto kv = m_periodicAtomMap.find(idx);
    if (kv != m_periodicAtomMap.end())
      offsets.push_back(kv->second);
  }
  deleteAtomsByOffset(offsets);
  updateBondGraph();
  emit atomsChanged();
}

void PeriodicStructure::updateBondGraph() {
  // Clear existing bonds and fragments
  m_covalentBonds.clear();
  m_hydrogenBonds.clear();
  m_vdwContacts.clear();
  m_fragments.clear();
  m_fragmentForAtom.clear();
  m_fragmentForAtom.resize(numberOfAtoms(), FragmentIndex{-1});
  
  // Clear unit cell fragment mapping
  m_unitCellFragments.clear();
  m_unitCellAtomFragments.clear();

  // Get connectivity from subclass
  const auto &g = getUnitCellConnectivity();
  const auto &edges = g.edges();
  const auto &adjacency = g.adjacency_list();

  ankerl::unordered_dense::set<int> visited;
  int currentFragmentIndex{0};

  std::vector<std::vector<int>> fragments;

  auto covalentVisitor = [&](const VertexDesc &v, const VertexDesc &prev,
                             const EdgeDesc &e, const MillerIndex &hkl) {
    auto &idxs = fragments[currentFragmentIndex];
    GenericAtomIndex atomIdx{static_cast<int>(v), hkl.h, hkl.k, hkl.l};
    auto location = m_periodicAtomMap.find(atomIdx);
    if (location == m_periodicAtomMap.end()) {
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

  // Build unit cell fragments FIRST
  buildUnitCellFragments();
  
  // Build fragments using bond graph traversal
  for (int i = 0; i < numberOfAtoms(); i++) {
    auto offset = m_periodicAtomOffsets[i];
    VertexDesc uc_vertex = offset.unique;
    int h = offset.x;
    int k = offset.y;
    int l = offset.z;
    int idx = m_periodicAtomMap[offset];

    if (visited.contains(idx) || testAtomFlag(offset, AtomFlag::Contact))
      continue;

    fragments.push_back({});
    filtered_connectivity_traversal_with_cell_offset(
        g, uc_vertex, covalentVisitor, covalentPredicate, {h, k, l});
    currentFragmentIndex++;
  }

  // Build bond lists
  for (const auto &[sourceCrystalIndex, sourceAtomIndex] : m_periodicAtomMap) {
    const VertexDesc sourceVertex =
        static_cast<VertexDesc>(sourceCrystalIndex.unique);
    if (testAtomFlag(sourceCrystalIndex, AtomFlag::Contact))
      continue;
    
    auto adjIt = adjacency.find(sourceVertex);
    if (adjIt == adjacency.end()) continue;
    
    for (const auto &[neighborVertexDesc, edgeDesc] : adjIt->second) {
      const auto &edge = edges.at(edgeDesc);
      MillerIndex targetHKL = {sourceCrystalIndex.x + edge.h,
                               sourceCrystalIndex.y + edge.k,
                               sourceCrystalIndex.z + edge.l};

      GenericAtomIndex targetIndex{static_cast<int>(neighborVertexDesc),
                                   targetHKL.h, targetHKL.k, targetHKL.l};
      const auto targetLoc = m_periodicAtomMap.find(targetIndex);
      if (targetLoc != m_periodicAtomMap.end()) {
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
        default:
          break;
        }
      }
    }
  }

  // Create Fragment objects - these represent complete molecules across all periodic images
  qDebug() << "Creating" << fragments.size() << "fragments from bond graph traversal";
  
  for (int fragIdx = 0; fragIdx < fragments.size(); fragIdx++) {
    const auto &fragAtomIndices = fragments[fragIdx];
    if (fragAtomIndices.empty()) continue;
    
    qDebug() << "Processing fragment" << fragIdx << "with" << fragAtomIndices.size() << "base atoms";
    
    // Get the unit cell fragment type and periodic offset for this fragment
    GenericAtomIndex firstAtom = m_periodicAtomOffsets[fragAtomIndices[0]];
    auto ucFragIt = m_unitCellAtomFragments.find(firstAtom.unique);
    if (ucFragIt == m_unitCellAtomFragments.end()) {
      qDebug() << "Error: No unit cell fragment found for atom" << firstAtom.unique;
      continue;
    }
    
    FragmentIndex unitCellFragIdx = ucFragIt->second;
    // Create fragment index with unit cell fragment ID and periodic offset from first atom
    FragmentIndex fragIndex{unitCellFragIdx.u, firstAtom.x, firstAtom.y, firstAtom.z};
    
    qDebug() << "Fragment" << fragIdx << "maps to unit cell fragment" << unitCellFragIdx.u 
             << "at offset (" << firstAtom.x << "," << firstAtom.y << "," << firstAtom.z << ")";
    
    // This fragment represents the specific molecule instance found by traversal
    // Convert atom indices to GenericAtomIndex
    std::vector<GenericAtomIndex> moleculeAtoms;
    for (int atomIdx : fragAtomIndices) {
      moleculeAtoms.push_back(m_periodicAtomOffsets[atomIdx]);
      m_fragmentForAtom[atomIdx] = fragIndex; // Use the properly encoded fragment index
    }
    
    if (!moleculeAtoms.empty()) {
      qDebug() << "Fragment" << fragIdx << "contains" << moleculeAtoms.size() << "atoms for this specific molecule instance";
      std::sort(moleculeAtoms.begin(), moleculeAtoms.end());
      auto frag = makeFragment(moleculeAtoms);
      frag.index = fragIndex; // Use the properly encoded fragment index
      
      // Set the asymmetricFragmentIndex to point to the unit cell fragment
      FragmentIndex unitCellIndex{fragIndex.u, 0, 0, 0};
      frag.asymmetricFragmentIndex = unitCellIndex;
      qDebug() << "Set asymmetricFragmentIndex to unit cell fragment" << unitCellIndex.u;
      
      // Set the fragment name using the label system
      frag.name = ChemicalStructure::getFragmentLabel(unitCellIndex);
      qDebug() << "Set fragment name to:" << frag.name;
      
      m_fragments.insert({frag.index, frag});
    } else {
      qDebug() << "Fragment" << fragIdx << "has no atoms!";
    }
  }
  
  // Ensure fragment labels work - populate m_fragmentLabels with mappings for all periodic fragments
  // Force the base class to build labels for unit cell fragments first
  if (!m_unitCellFragments.empty()) {
    qDebug() << "Checking unit cell fragments for label generation:";
    for (const auto &[ucFragIndex, ucFrag] : m_unitCellFragments) {
      qDebug() << "Unit cell fragment" << ucFragIndex.u << "has" << ucFrag.atomIndices.size() << "atoms, atomicNumbers size:" << ucFrag.atomicNumbers.size();
      for (int i = 0; i < std::min(3, (int)ucFrag.atomIndices.size()); i++) {
        const auto &atomIdx = ucFrag.atomIndices[i];
        int mappedIndex = genericIndexToIndex(atomIdx);
        int atomicNum = (i < ucFrag.atomicNumbers.size()) ? ucFrag.atomicNumbers(i) : -1;
        qDebug() << "  Atom" << atomIdx.unique << "(" << atomIdx.x << "," << atomIdx.y << "," << atomIdx.z << ") maps to" << mappedIndex << "atomic number" << atomicNum;
      }
    }
    
    // Don't pre-populate m_fragmentLabels - let the base class handle it when getFragmentLabel is called
    qDebug() << "Unit cell fragments built, labels will be generated on demand";
  }
  
  setAllFragmentColors(FragmentColorSettings{});
  emit atomsChanged();
}

void PeriodicStructure::resetAtomsAndBonds(bool toSelection) {
  if (toSelection) {
    std::vector<QString> elementSymbols;
    std::vector<occ::Vec3> positions;
    std::vector<QString> newLabels;
    std::vector<GenericAtomIndex> newPeriodicAtomOffsets;
    ankerl::unordered_dense::map<GenericAtomIndex, int, GenericAtomIndexHash>
        newAtomMap;

    int numAtoms = 0;
    for (int i = 0; i < numberOfAtoms(); i++) {
      if (!atomFlagsSet(m_periodicAtomOffsets[i], AtomFlag::Selected))
        continue;
      positions.push_back(atomicPositions().col(i));
      elementSymbols.push_back(QString::fromStdString(
          occ::core::Element(atomicNumbers()(i)).symbol()));
      newLabels.push_back(labels()[i]);
      GenericAtomIndex idx = m_periodicAtomOffsets[i];
      newPeriodicAtomOffsets.push_back(idx);
      newAtomMap.insert({idx, numAtoms});
      numAtoms++;
    }
    clearAtoms();
    m_periodicAtomOffsets.clear();
    m_periodicAtomMap.clear();
    addPeriodicAtoms(newPeriodicAtomOffsets);
    updateBondGraph();
  } else {
    // Reset to base atoms (periodic structures should override for structure-specific behavior)
    qDebug() << "PeriodicStructure::resetAtomsAndBonds - finding base atoms";
    
    // Find base atoms before clearing
    std::vector<GenericAtomIndex> baseAtoms;
    for (const auto &[idx, atomIndex] : m_periodicAtomMap) {
      if (idx.x == 0 && idx.y == 0 && idx.z == 0) {
        baseAtoms.push_back(idx);
      }
    }
    
    qDebug() << "Found" << baseAtoms.size() << "base atoms";
    
    // Clear everything
    clearAtoms();
    m_periodicAtomOffsets.clear();
    m_periodicAtomMap.clear();
    
    // Re-add only the base atoms
    if (!baseAtoms.empty()) {
      addPeriodicAtoms(baseAtoms);
      updateBondGraph();
    } else {
      qDebug() << "No base atoms found - structure may need specific reset implementation";
    }
  }
  emit atomsChanged();
}

// Fragment operations
void PeriodicStructure::completeFragmentContaining(int atomIndex) {
  if (atomIndex < 0 || atomIndex >= numberOfAtoms())
    return;
  GenericAtomIndex idx = indexToGenericIndex(atomIndex);
  completeFragmentContaining(idx);
}

void PeriodicStructure::completeFragmentContaining(GenericAtomIndex index) {
  bool haveContactAtoms = anyAtomHasFlags(AtomFlag::Contact);
  
  // Get connectivity from subclass
  const auto &g = getUnitCellConnectivity();
  const auto &edges = g.edges();
  
  // Find all atoms connected to this atom via covalent bonds
  ankerl::unordered_dense::set<GenericAtomIndex, GenericAtomIndexHash> atomsToAdd;
  
  auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                     const EdgeDesc &e, const MillerIndex &hkl) {
    GenericAtomIndex atomIdx{static_cast<int>(v), hkl.h, hkl.k, hkl.l};
    auto location = m_periodicAtomMap.find(atomIdx);
    if (location != m_periodicAtomMap.end()) {
      setAtomFlag(atomIdx, AtomFlag::Contact, false);
      return;
    }
    atomsToAdd.insert(atomIdx);
  };

  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };

  VertexDesc uc_vertex = index.unique;
  filtered_connectivity_traversal_with_cell_offset(
      g, uc_vertex, visitor, covalentPredicate,
      {index.x, index.y, index.z});
  
  // Add missing atoms
  if (!atomsToAdd.empty()) {
    std::vector<GenericAtomIndex> indices(atomsToAdd.begin(), atomsToAdd.end());
    addPeriodicAtoms(indices, AtomFlag::NoFlag);
  }

  if (haveContactAtoms) {
    addPeriodicContactAtoms();
  }
  updateBondGraph();
  emit atomsChanged();
}

bool PeriodicStructure::hasIncompleteFragments() const {
  const auto &g = getUnitCellConnectivity();
  const auto &edges = g.edges();

  bool incomplete{false};

  auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                     const EdgeDesc &e, const MillerIndex &hkl) {
    GenericAtomIndex atomIdx{static_cast<int>(v), hkl.h, hkl.k, hkl.l};
    auto location = m_periodicAtomMap.find(atomIdx);
    if (location == m_periodicAtomMap.end()) {
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

bool PeriodicStructure::hasIncompleteSelectedFragments() const {
  const auto &g = getUnitCellConnectivity();
  const auto &edges = g.edges();

  bool incomplete{false};

  auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                     const EdgeDesc &e, const MillerIndex &hkl) {
    GenericAtomIndex atomIdx{static_cast<int>(v), hkl.h, hkl.k, hkl.l};
    auto location = m_periodicAtomMap.find(atomIdx);
    if (location == m_periodicAtomMap.end()) {
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

void PeriodicStructure::completeAllFragments() {
  bool haveContactAtoms = anyAtomHasFlags(AtomFlag::Contact);
  const auto selectedAtoms = atomsWithFlags(AtomFlag::Selected);

  const auto &g = getUnitCellConnectivity();
  const auto &edges = g.edges();
  GenericAtomIndexSet atomsToAdd;

  auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                     const EdgeDesc &e, const MillerIndex &hkl) {
    GenericAtomIndex atomIdx{static_cast<int>(v), hkl.h, hkl.k, hkl.l};
    auto location = m_periodicAtomMap.find(atomIdx);
    if (location != m_periodicAtomMap.end()) {
      setAtomFlag(atomIdx, AtomFlag::Contact, false);
      return;
    }
    atomsToAdd.insert(atomIdx);
  };

  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };

  for (int atomIndex = 0; atomIndex < numberOfAtoms(); atomIndex++) {
    VertexDesc uc_vertex = m_periodicAtomOffsets[atomIndex].unique;
    const auto &offset = m_periodicAtomOffsets[atomIndex];

    filtered_connectivity_traversal_with_cell_offset(
        g, uc_vertex, visitor, covalentPredicate,
        {offset.x, offset.y, offset.z});
  }
  
  std::vector<GenericAtomIndex> indices(atomsToAdd.begin(), atomsToAdd.end());
  addPeriodicAtoms(indices, AtomFlag::NoFlag);
  if (haveContactAtoms)
    addPeriodicContactAtoms();
  updateBondGraph();

  // Ensure selection doesn't change
  for (const auto &idx : selectedAtoms) {
    setAtomFlag(idx, AtomFlag::Selected);
  }
  emit atomsChanged();
}

void PeriodicStructure::expandAtomsWithinRadius(float radius, bool selected) {
  std::vector<GenericAtomIndex> selectedAtoms;
  if (selected) {
    // Reset to selection
    resetAtomsAndBonds(true);
    selectedAtoms = m_periodicAtomOffsets;
    setFlagForAtoms(selectedAtoms, AtomFlag::Selected);
    if (std::abs(radius) < 1e-3)
      return;
  }

  // Find atoms within radius (dimension-aware)
  std::vector<GenericAtomIndex> centerAtoms;
  if (selected) {
    centerAtoms = selectedAtoms;
  } else {
    centerAtoms = m_periodicAtomOffsets;
  }
  
  auto atomsWithinRadius = findAtomsWithinRadius(centerAtoms, radius);
  
  // Filter out already existing atoms
  std::vector<GenericAtomIndex> atomsToAdd;
  for (const auto &idx : atomsWithinRadius) {
    if (genericIndexToIndex(idx) == -1) {
      atomsToAdd.push_back(idx);
    }
  }

  if (!atomsToAdd.empty()) {
    addPeriodicAtoms(atomsToAdd, AtomFlag::NoFlag);
    updateBondGraph();
    emit atomsChanged();
  }
  setFlagForAtoms(selectedAtoms, AtomFlag::Selected);
}

void PeriodicStructure::addPeriodicContactAtoms() {
  qDebug() << "PeriodicStructure::addPeriodicContactAtoms called";
  
  using GenericAtomIndexSet = ankerl::unordered_dense::set<GenericAtomIndex, GenericAtomIndexHash>;
  GenericAtomIndexSet contactAtomsToAdd;
  
  // Get bond graph with contact connections
  const auto &g = getUnitCellConnectivity();
  const auto &edges = g.edges();
  const auto &adjacency = g.adjacency_list();
  
  // For each existing atom, use bond graph to find contact atoms
  for (int i = 0; i < numberOfAtoms(); i++) {
    GenericAtomIndex sourceIdx = indexToGenericIndex(i);
    
    // Skip if this atom is already a contact atom
    if (testAtomFlag(sourceIdx, AtomFlag::Contact)) continue;
    
    // Get the base vertex for this atom
    VertexDesc sourceVertex = static_cast<VertexDesc>(sourceIdx.unique);
    
    // Check if this vertex has adjacencies in the bond graph
    auto adjIt = adjacency.find(sourceVertex);
    if (adjIt == adjacency.end()) continue;
    
    // For each edge from this vertex
    for (const auto &[neighborVertex, edgeDesc] : adjIt->second) {
      const auto &edge = edges.at(edgeDesc);
      
      // Only consider contact edges, not covalent bonds
      if (edge.connectionType != occ::core::graph::PeriodicEdge::Connection::CloseContact) continue;
      
      // Calculate target position: source position + edge offset
      GenericAtomIndex targetIdx{
        static_cast<int>(neighborVertex),
        sourceIdx.x + edge.h,
        sourceIdx.y + edge.k, 
        sourceIdx.z + edge.l
      };
      
      // Check if this contact atom already exists
      if (m_periodicAtomMap.find(targetIdx) == m_periodicAtomMap.end()) {
        contactAtomsToAdd.insert(targetIdx);
      }
    }
  }
  
  // Add the contact atoms
  if (!contactAtomsToAdd.empty()) {
    std::vector<GenericAtomIndex> contactAtoms(contactAtomsToAdd.begin(), contactAtomsToAdd.end());
    addPeriodicAtoms(contactAtoms, AtomFlag::Contact);
    qDebug() << "Added" << contactAtoms.size() << "contact atoms using bond graph";
  }
}

void PeriodicStructure::setShowContacts(const ContactSettings &settings) {
  m_contactSettings = settings;
  if (settings.show) {
    addPeriodicContactAtoms();
    updateBondGraph();
    emit atomsChanged();
  } else {
    removePeriodicContactAtoms();
    updateBondGraph();
    emit atomsChanged();
  }
}

// Fragment utilities
std::vector<FragmentIndex> PeriodicStructure::completedFragments() const {
  std::vector<FragmentIndex> result;
  const auto &g = getUnitCellConnectivity();
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
      auto location = m_periodicAtomMap.find(atomIdx);
      if (location == m_periodicAtomMap.end()) {
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

std::vector<FragmentIndex> PeriodicStructure::selectedFragments() const {
  std::vector<FragmentIndex> result;
  for (const auto &[fragIndex, frag] : m_fragments) {
    const auto &fragIndices = frag.atomIndices;
    // Don't skip single-atom fragments - they can be selected too
    // Check if ANY atom in the fragment is selected
    bool hasSelectedAtom = false;
    for (const auto &atomIdx : fragIndices) {
      if (atomFlagsSet(atomIdx, AtomFlag::Selected)) {
        hasSelectedAtom = true;
        break;
      }
    }
    if (hasSelectedAtom)
      result.push_back(fragIndex);
  }
  return result;
}

Fragment PeriodicStructure::makeFragment(
    const std::vector<GenericAtomIndex> &idxs) const {
  Fragment result;
  result.atomIndices = idxs;
  std::sort(result.atomIndices.begin(), result.atomIndices.end());
  result.atomicNumbers = atomicNumbersForIndices(idxs);
  result.positions = atomicPositionsForIndices(idxs);
  return result;
}

// Atom querying
std::vector<GenericAtomIndex>
PeriodicStructure::atomsWithFlags(const AtomFlags &flags, bool set) const {
  std::vector<GenericAtomIndex> result;
  
  for (const auto &offset : m_periodicAtomOffsets) {
    bool check = atomFlagsSet(offset, flags);
    if ((set && check) || (!set && !check)) {
      result.push_back(offset);
    }
  }
  
  return result;
}

std::vector<GenericAtomIndex>
PeriodicStructure::atomsSurroundingAtoms(const std::vector<GenericAtomIndex> &idxs,
                                        float radius) const {
  qDebug() << "PeriodicStructure::atomsSurroundingAtoms called with" << idxs.size() << "atoms, radius =" << radius;
  auto result = findAtomsWithinRadius(idxs, radius);
  qDebug() << "PeriodicStructure::atomsSurroundingAtoms returning" << result.size() << "atoms";
  return result;
}

std::vector<GenericAtomIndex>
PeriodicStructure::atomsSurroundingAtomsWithFlags(const AtomFlags &flags,
                                                 float radius) const {
  auto flaggedAtoms = atomsWithFlags(flags);
  return findAtomsWithinRadius(flaggedAtoms, radius);
}

// Coordinate utilities
occ::IVec PeriodicStructure::atomicNumbersForIndices(
    const std::vector<GenericAtomIndex> &idxs) const {
  occ::IVec result(idxs.size());
  for (int i = 0; i < idxs.size(); i++) {
    const auto &idx = idxs[i];
    if (idx.unique >= 0 && idx.unique < m_baseAtoms.atomic_numbers.size()) {
      result(i) = m_baseAtoms.atomic_numbers(idx.unique);
    } else {
      qDebug() << "Warning: Invalid unique index" << idx.unique << "for base atoms size" << m_baseAtoms.atomic_numbers.size();
      result(i) = 1; // Default to hydrogen to avoid crashes
    }
  }
  return result;
}

std::vector<QString> PeriodicStructure::labelsForIndices(
    const std::vector<GenericAtomIndex> &idxs) const {
  std::vector<QString> result;
  result.reserve(idxs.size());
  for (const auto &idx : idxs) {
    int atomIndex = genericIndexToIndex(idx);
    if (atomIndex >= 0 && atomIndex < labels().size()) {
      result.push_back(labels()[atomIndex]);
    } else {
      result.push_back("");
    }
  }
  return result;
}

occ::Mat3N PeriodicStructure::atomicPositionsForIndices(
    const std::vector<GenericAtomIndex> &idxs) const {
  occ::Mat3N result(3, idxs.size());
  for (int i = 0; i < idxs.size(); i++) {
    int atomIndex = genericIndexToIndex(idxs[i]);
    if (atomIndex >= 0) {
      result.col(i) = atomicPositions().col(atomIndex);
    }
  }
  return result;
}

// Override ChemicalStructure fragment interface for proper labeling
const FragmentMap &PeriodicStructure::symmetryUniqueFragments() const {
  // The base class getFragmentLabel() builds labels for fragments returned here,
  // then looks up requested fragment indices in that same set.
  // So we need to return ALL fragments that might be requested for labels,
  // but we want them to map to unit cell fragment formulas for labeling.
  
  static FragmentMap allFragmentsForLabeling;
  allFragmentsForLabeling.clear();
  
  // Add all unit cell fragments as-is
  for (const auto &[ucFragIndex, ucFrag] : m_unitCellFragments) {
    allFragmentsForLabeling[ucFragIndex] = ucFrag;
  }
  
  // Add all periodic fragments, but map them to their unit cell fragment data for formula calculation
  for (const auto &[fragIndex, frag] : m_fragments) {
    if (allFragmentsForLabeling.find(fragIndex) == allFragmentsForLabeling.end()) {
      // Find the corresponding unit cell fragment
      FragmentIndex unitCellIndex{fragIndex.u, 0, 0, 0};
      auto ucFragIt = m_unitCellFragments.find(unitCellIndex);
      if (ucFragIt != m_unitCellFragments.end()) {
        // Use the unit cell fragment data but with the periodic fragment index
        Fragment periodicFragForLabeling = ucFragIt->second;
        periodicFragForLabeling.index = fragIndex;
        allFragmentsForLabeling[fragIndex] = periodicFragForLabeling;
      }
    }
  }
  
  return allFragmentsForLabeling;
}

// Transformation utilities
std::vector<GenericAtomIndex> PeriodicStructure::getAtomIndicesUnderTransformation(
    const std::vector<GenericAtomIndex> &idxs,
    const Eigen::Isometry3d &transform) const {
  // Base implementation - subclasses should override for specific behavior
  return idxs;
}

bool PeriodicStructure::getTransformation(
    const std::vector<GenericAtomIndex> &from_orig,
    const std::vector<GenericAtomIndex> &to_orig,
    Eigen::Isometry3d &result) const {
  // Base implementation - subclasses should override for specific behavior
  result = Eigen::Isometry3d::Identity();
  return false;
}

// Private helper methods
void PeriodicStructure::buildUnitCellFragments() {
  // Build unit cell fragments from base atoms (those with x=y=z=0)
  const auto &g = getUnitCellConnectivity();
  const auto &edges = g.edges();
  
  qDebug() << "PeriodicStructure::buildUnitCellFragments() - starting with" << numberOfAtoms() << "atoms";
  qDebug() << "Bond graph has" << g.num_vertices() << "vertices and" << g.num_edges() << "edges";
  
  ankerl::unordered_dense::set<int> visited;
  int fragmentIndex = 0;
  
  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };
  
  // Process each base atom
  for (int i = 0; i < numberOfAtoms(); i++) {
    GenericAtomIndex atomIdx = indexToGenericIndex(i);
    // Only process base atoms (no periodic offset)
    if (atomIdx.x != 0 || atomIdx.y != 0 || atomIdx.z != 0) continue;
    if (testAtomFlag(atomIdx, AtomFlag::Contact)) continue;
    if (visited.contains(atomIdx.unique)) continue;
    
    // Build fragment by traversing connectivity
    std::vector<int> fragmentAtoms;
    
    auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                       const EdgeDesc &e, const MillerIndex &hkl) {
      // Only collect base atoms (hkl should be 0,0,0 for unit cell fragment)
      if (hkl.h == 0 && hkl.k == 0 && hkl.l == 0) {
        int uniqueAtom = static_cast<int>(v);
        if (!visited.contains(uniqueAtom)) {
          visited.insert(uniqueAtom);
          fragmentAtoms.push_back(uniqueAtom);
          m_unitCellAtomFragments[uniqueAtom] = FragmentIndex{fragmentIndex};
        }
      }
    };
    
    // Start traversal from this base atom
    VertexDesc uc_vertex = atomIdx.unique;
    filtered_connectivity_traversal_with_cell_offset(
        g, uc_vertex, visitor, covalentPredicate, {0, 0, 0});
    
    // Create unit cell fragment
    if (!fragmentAtoms.empty()) {
      Fragment frag;
      frag.index = FragmentIndex{fragmentIndex};
      
      // Build fragment data from actual atoms in the structure
      frag.atomicNumbers.resize(fragmentAtoms.size());
      frag.positions.resize(3, fragmentAtoms.size());
      frag.atomIndices.clear();
      
      int atomCount = 0;
      for (int uniqueAtom : fragmentAtoms) {
        GenericAtomIndex atomIdx{uniqueAtom, 0, 0, 0};
        frag.atomIndices.push_back(atomIdx);
        
        // Try to find this atom in the current structure to get its data
        int structureIndex = genericIndexToIndex(atomIdx);
        if (structureIndex >= 0) {
          // Use data from existing atom
          frag.atomicNumbers(atomCount) = atomicNumbers()(structureIndex);
          frag.positions.col(atomCount) = atomicPositions().col(structureIndex);
        } else {
          // This unit cell atom doesn't exist - we need to find any periodic copy
          // Look for any atom with the same unique index but different offsets
          bool found = false;
          for (const auto &[periodicIdx, structIdx] : m_periodicAtomMap) {
            if (periodicIdx.unique == uniqueAtom) {
              frag.atomicNumbers(atomCount) = atomicNumbers()(structIdx);
              // For unit cell fragment, use the base position (no periodic offset)
              occ::Vec3 basePos = atomicPositions().col(structIdx);
              // Remove periodic offset to get unit cell position
              if (periodicIdx.x != 0 || periodicIdx.y != 0 || periodicIdx.z != 0) {
                // We'd need cell vectors to subtract the offset, but for now just use the position
                // This should ideally be calculated properly based on the structure type
              }
              frag.positions.col(atomCount) = basePos;
              found = true;
              break;
            }
          }
          if (!found) {
            qWarning() << "Could not find data for unit cell atom" << uniqueAtom;
            // Use dummy data
            frag.atomicNumbers(atomCount) = 1; // Hydrogen as fallback
            frag.positions.col(atomCount) = occ::Vec3::Zero();
          }
        }
        atomCount++;
      }
      
      std::sort(frag.atomIndices.begin(), frag.atomIndices.end());
      m_unitCellFragments[frag.index] = frag;
      fragmentIndex++;
    }
  }
  
  qDebug() << "Built" << m_unitCellFragments.size() << "unit cell fragments";
  qDebug() << "Unit cell atom->fragment mapping has" << m_unitCellAtomFragments.size() << "entries";
}

void PeriodicStructure::updateFragmentMapping() {
  // Update fragment-to-atom mapping
  m_periodicFragments.clear();
  
  // Basic fragment creation - subclasses should override for sophisticated logic
  for (int i = 0; i < numberOfAtoms(); i++) {
    if (m_fragmentForAtom[i].u >= 0) continue;
    
    Fragment frag;
    frag.index = FragmentIndex{static_cast<int>(m_periodicFragments.size())};
    frag.atomIndices.push_back(m_periodicAtomOffsets[i]);
    frag.atomicNumbers.resize(1);
    frag.atomicNumbers(0) = atomicNumbers()(i);
    frag.positions.resize(3, 1);
    frag.positions.col(0) = atomicPositions().col(i);
    
    m_fragmentForAtom[i] = frag.index;
    m_periodicFragments[frag.index] = frag;
  }
}

void PeriodicStructure::propagateAtomFlagViaConnectivity(GenericAtomIndex startAtom, 
                                                        AtomFlag flag, 
                                                        bool set) {
  // Use bond graph traversal to propagate flags through connected atoms
  const auto &g = getUnitCellConnectivity();
  const auto &edges = g.edges();
  
  ankerl::unordered_dense::set<GenericAtomIndex, GenericAtomIndexHash> atomsToFlag;
  
  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };
  
  auto visitor = [&](const VertexDesc &v, const VertexDesc &prev,
                     const EdgeDesc &e, const MillerIndex &hkl) {
    GenericAtomIndex atomIdx{static_cast<int>(v), hkl.h, hkl.k, hkl.l};
    
    // Check if this atom exists in our structure
    auto location = m_periodicAtomMap.find(atomIdx);
    if (location != m_periodicAtomMap.end()) {
      // Add to atoms to flag
      atomsToFlag.insert(atomIdx);
    }
  };
  
  // Traverse connected atoms via bond graph starting from the clicked atom
  VertexDesc uc_vertex = startAtom.unique;
  filtered_connectivity_traversal_with_cell_offset(
      g, uc_vertex, visitor, covalentPredicate,
      {startAtom.x, startAtom.y, startAtom.z});
  
  // Now apply flags to all found atoms
  for (const auto &atomIdx : atomsToFlag) {
    setAtomFlag(atomIdx, flag, set);
  }
}

void PeriodicStructure::selectFragmentContaining(int atom) {
  if (atom < 0 || atom >= numberOfAtoms()) return;
  GenericAtomIndex atomIdx = indexToGenericIndex(atom);
  selectFragmentContaining(atomIdx);
}

void PeriodicStructure::selectFragmentContaining(GenericAtomIndex atom) {
  // Skip contact atoms
  if (testAtomFlag(atom, AtomFlag::Contact)) return;
  
  // Use bond graph connectivity to select all connected atoms (entire molecule)
  propagateAtomFlagViaConnectivity(atom, AtomFlag::Selected, true);
}

void PeriodicStructure::deleteAtomsByOffsetCommon(const std::vector<int> &atomIndices) {
  // Common atom deletion logic
  const int originalNumAtoms = numberOfAtoms();

  ankerl::unordered_dense::set<int> uniqueIndices;
  for (int idx : std::as_const(atomIndices)) {
    if (idx < originalNumAtoms)
      uniqueIndices.insert(idx);
  }

  std::vector<QString> newElementSymbols;
  std::vector<occ::Vec3> newPositions;
  std::vector<QString> newLabels;
  std::vector<GenericAtomIndex> periodicAtomOffsets;
  m_periodicAtomMap.clear();
  
  const auto &currentPositions = atomicPositions();
  const auto &currentLabels = labels();
  const auto &currentNumbers = atomicNumbers();

  int atomIndex = 0;
  for (int i = 0; i < originalNumAtoms; i++) {
    if (uniqueIndices.contains(i))
      continue;
    periodicAtomOffsets.push_back(m_periodicAtomOffsets[i]);
    m_periodicAtomMap.insert({m_periodicAtomOffsets[i], atomIndex});
    newPositions.push_back(currentPositions.col(i));
    newElementSymbols.push_back(
        QString::fromStdString(occ::core::Element(currentNumbers(i)).symbol()));
    if (currentLabels.size() > i) {
      newLabels.push_back(currentLabels[i]);
    }
    atomIndex++;
  }
  m_periodicAtomOffsets = periodicAtomOffsets;
  setAtoms(newElementSymbols, newPositions, newLabels);
}