#include "chemicalstructure.h"
#include "elementdata.h"
#include "mesh.h"
#include "object_tree_model.h"
#include <QEvent>
#include <QIcon>
#include <occ/core/element.h>
#include <occ/core/kabsch.h>
#include <occ/core/kdtree.h>

ChemicalStructure::ChemicalStructure(QObject *parent)
    : QObject(parent), m_interactions(new PairInteractions(this)) {
  // PairInteractions are not set with parent as this, otherwise it will
  // appear in the child list display on the right
  this->installEventFilter(this);

  m_treeModel = new ObjectTreeModel(this);
}

occ::Vec3 ChemicalStructure::atomPosition(GenericAtomIndex idx) const {
  int i = genericIndexToIndex(idx);
  return m_atomicPositions.col(i);
}

void ChemicalStructure::updateBondGraph() { guessBondsBasedOnDistances(); }

void ChemicalStructure::guessBondsBasedOnDistances() {

  occ::core::KDTree<double> tree(3, m_atomicPositions, occ::core::max_leaf);
  tree.index->buildIndex();
  using VertexDesc = typename occ::core::graph::BondGraph::VertexDescriptor;
  using EdgeDesc = typename occ::core::graph::BondGraph::EdgeDescriptor;
  using Connection = occ::core::graph::Edge::Connection;
  using Vertex = occ::core::graph::Vertex;
  using Edge = occ::core::graph::Edge;

  occ::Vec cov = covalentRadii();
  occ::Vec vdw = vdwRadii();
  const double maxVdw = vdw.maxCoeff();
  // TODO allow changing buffer of 0.4
  const double max_dist2 = (maxVdw * 2 + 0.4) * (maxVdw * 2 + 0.4);

  std::vector<std::pair<size_t, double>> idxs_dists;
  nanoflann::RadiusResultSet results(max_dist2, idxs_dists);

  m_bondGraphVertices.clear();
  m_bondGraphEdges.clear();
  m_bondGraph = occ::core::graph::BondGraph();

  m_bondGraphVertices.reserve(cov.rows());

  for (size_t i = 0; i < cov.rows(); i++) {
    m_bondGraphVertices.push_back(m_bondGraph.add_vertex(Vertex{i}));
  }

  int numConnections = 0;

  auto addEdge = [&](double d, size_t l, size_t r, Connection bondType) {
    Edge leftToRight{std::sqrt(d), l, r, bondType};
    m_bondGraphEdges.push_back(m_bondGraph.add_edge(l, r, leftToRight));
    Edge rightToLeft{std::sqrt(d), r, l, bondType};
    m_bondGraph.add_edge(r, l, rightToLeft);
    numConnections++;
  };

  auto canHydrogenBond = [](int a, int b) {
    if (a == 1) {
      if (b == 7 || b == 8 || b == 9)
        return true;
    } else if (b == 1) {
      if (a == 7 || a == 8 || a == 9)
        return true;
    }
    return false;
  };

  for (int a = 0; a < numberOfAtoms(); a++) {
    double cov_a = cov(a);
    double vdw_a = vdw(a);
    const double *q = m_atomicPositions.col(a).data();
    tree.index->findNeighbors(results, q, nanoflann::SearchParams());
    for (const auto &result : idxs_dists) {
      int idx = result.first;
      double d2 = result.second;
      if (idx <= a)
        continue;
      double cov_b = cov(idx);
      double vdw_b = vdw(idx);
      if (d2 < ((cov_a + cov_b + 0.4) * (cov_a + cov_b + 0.4))) {
        // bonded
        addEdge(d2, a, idx, Connection::CovalentBond);
      } else if (d2 < ((vdw_a + vdw_b) * (vdw_a + vdw_b))) {
        addEdge(d2, a, idx, Connection::CloseContact);
        if (canHydrogenBond(m_atomicNumbers(a), m_atomicNumbers(idx))) {
          addEdge(d2, a, idx, Connection::HydrogenBond);
        }
      }
    }
    results.clear();
  }

  m_bondsNeedUpdate = false;
  m_fragments.clear();
  m_symmetryUniqueFragments.clear();
  m_fragmentForAtom.clear();
  m_covalentBonds.clear();
  m_hydrogenBonds.clear();
  m_vdwContacts.clear();
  m_fragmentForAtom.resize(numberOfAtoms(), FragmentIndex{-1});

  std::vector<std::vector<int>> fragments;

  const auto &edges = m_bondGraph.edges();

  ankerl::unordered_dense::set<VertexDesc> visited;
  int currentFragmentIndex{0};

  auto covalentVisitor = [&](const VertexDesc &v, const VertexDesc &prev,
                             const EdgeDesc &e) {
    auto &idxs = fragments[currentFragmentIndex];
    visited.insert(v);
    m_fragmentForAtom[v] = FragmentIndex{currentFragmentIndex};
    idxs.push_back(v);
  };

  auto covalentPredicate = [&edges](const EdgeDesc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };

  for (const auto &v : m_bondGraph.vertices()) {
    if (visited.contains(v.first))
      continue;
    fragments.push_back({});
    m_bondGraph.breadth_first_traversal_with_edge_filtered(
        v.first, covalentVisitor, covalentPredicate);
    currentFragmentIndex++;
  }

  // TODO detect symmetry
  for (int f = 0; f < fragments.size(); f++) {
    std::vector<GenericAtomIndex> sym;
    for (int idx : fragments[f]) {
      sym.push_back(GenericAtomIndex{idx});
    }
    std::sort(sym.begin(), sym.end());
    auto frag = makeFragment(sym);
    frag.index.u = f;
    m_fragments.insert({frag.index, frag});
    if (frag.asymmetricFragmentIndex.u == m_symmetryUniqueFragments.size()) {
      m_symmetryUniqueFragments.insert({frag.index, frag});
    }
  }

  for (const auto &[edge_desc, edge] : m_bondGraph.edges()) {
    switch (edge.connectionType) {
    case Connection::CovalentBond:
      m_covalentBonds.push_back({edge.source, edge.target});
      break;
    case Connection::HydrogenBond:
      m_hydrogenBonds.push_back({edge.source, edge.target});
      break;
    case Connection::CloseContact:
      m_vdwContacts.push_back({edge.source, edge.target});
      break;
    }
  }
}

const AtomFlags &ChemicalStructure::atomFlags(GenericAtomIndex index) const {
  return m_flags.at(index);
}

bool ChemicalStructure::testAtomFlag(GenericAtomIndex idx,
                                     AtomFlag flag) const {
  return m_flags.at(idx).testFlag(flag);
}

void ChemicalStructure::setAtomFlags(GenericAtomIndex index,
                                     const AtomFlags &flags) {
  m_flags[index] = flags;
  emit atomsChanged();
}

void ChemicalStructure::setAtomFlag(GenericAtomIndex idx, AtomFlag flag,
                                    bool on) {
  m_flags[idx].setFlag(flag, on);
  emit atomsChanged();
}

bool ChemicalStructure::atomFlagsSet(GenericAtomIndex index,
                                     const AtomFlags &flags) const {
  return m_flags.at(index) & flags;
}

void ChemicalStructure::resetOrigin() {
  m_origin = m_atomicPositions.rowwise().mean();
}

void ChemicalStructure::setOrigin(const Eigen::Vector3d &vec) {
  m_origin = vec;
}

float ChemicalStructure::radius() const {
  return std::sqrt((m_atomicPositions.colwise() - m_origin)
                       .colwise()
                       .squaredNorm()
                       .maxCoeff());
}

occ::Vec ChemicalStructure::covalentRadii() const {
  occ::Vec result(numberOfAtoms());
  for (int i = 0; i < numberOfAtoms(); i++) {
    double radius = 0.0;
    Element *el = ElementData::elementFromAtomicNumber(m_atomicNumbers(i));
    if (el) {
      radius = el->covRadius();
    } else {
      auto element = occ::core::Element(m_atomicNumbers(i));
      radius = element.covalent_radius();
    }
    result(i) = (radius > 0.0) ? radius : 2.0;
  }
  return result;
}

occ::Vec ChemicalStructure::vdwRadii() const {
  occ::Vec result(numberOfAtoms());
  for (int i = 0; i < numberOfAtoms(); i++) {
    double radius = 0.0;
    Element *el = ElementData::elementFromAtomicNumber(m_atomicNumbers(i));
    if (el) {
      radius = el->vdwRadius();
    } else {
      auto element = occ::core::Element(m_atomicNumbers(i));
      radius = element.van_der_waals_radius();
    }
    result(i) = (radius > 0.0) ? radius : 2.0;
  }
  return result;
}

void ChemicalStructure::clearAtoms() {
  m_atomicNumbers = occ::IVec();
  m_atomicPositions = occ::Mat3N();
  m_flags.clear();
  m_labels.clear();
}

void ChemicalStructure::setAtoms(const std::vector<QString> &elementSymbols,
                                 const std::vector<occ::Vec3> &positions,
                                 const std::vector<QString> &labels) {
  Q_ASSERT(elementSymbols.size() == positions.size());
  occ::core::Element element(1);
  const int N = elementSymbols.size();
  m_atomicNumbers = occ::IVec(N);
  m_atomicPositions = occ::Mat3N(3, N);
  m_flags.clear();
  m_labels.clear();

  m_labels.reserve(N);
  m_fragmentForAtom.reserve(N);

  for (int i = 0; i < N; i++) {
    element = occ::core::Element(elementSymbols[i].toStdString());
    m_atomicNumbers(i) = element.atomic_number();

    m_atomicPositions(0, i) = positions[i].x();
    m_atomicPositions(1, i) = positions[i].y();
    m_atomicPositions(2, i) = positions[i].z();

    if (labels.size() > i) {
      m_labels.push_back(labels[i]);
    } else {
      m_labels.push_back(elementSymbols[i]);
    }
    // TODO flags?
    auto idx = indexToGenericIndex(i);
    setAtomFlags(idx, AtomFlag::NoFlag);
  }
  m_origin = m_atomicPositions.rowwise().mean();
  m_bondsNeedUpdate = true;
  emit atomsChanged();
}

void ChemicalStructure::addAtoms(const std::vector<QString> &elementSymbols,
                                 const std::vector<occ::Vec3> &positions,
                                 const std::vector<QString> &labels) {
  Q_ASSERT(elementSymbols.size() == positions.size());
  occ::core::Element element(1);
  const int numOld = numberOfAtoms();
  const int numAdded = elementSymbols.size();
  const int numTotal = numOld + numAdded;
  m_atomicNumbers.conservativeResize(numTotal, 1);
  m_atomicPositions.conservativeResize(3, numTotal);
  m_fragmentForAtom.resize(numTotal, FragmentIndex{-1});

  for (int i = 0; i < numAdded; i++) {
    element = occ::core::Element(elementSymbols[i].toStdString());
    int index = numOld + i;
    m_atomicNumbers(index) = element.atomic_number();

    m_atomicPositions(0, index) = positions[i].x();
    m_atomicPositions(1, index) = positions[i].y();
    m_atomicPositions(2, index) = positions[i].z();
    m_fragmentForAtom[index] = FragmentIndex{index};

    if (labels.size() > i) {
      m_labels.push_back(labels[i]);
    } else {
      m_labels.push_back(elementSymbols[i]);
    }
    auto genericIndex = indexToGenericIndex(index);
    setAtomFlags(genericIndex, AtomFlag::NoFlag);
  }
  m_origin = m_atomicPositions.rowwise().mean();
  m_bondsNeedUpdate = true;
  emit atomsChanged();
}

QStringList ChemicalStructure::uniqueElementSymbols() const {
  if (numberOfAtoms() < 1)
    return {};
  std::vector<int> vec(m_atomicNumbers.data(),
                       m_atomicNumbers.data() + m_atomicNumbers.size());
  std::sort(vec.begin(), vec.end());
  vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
  QStringList result;
  for (const auto &num : vec) {
    result.push_back(QString::fromStdString(occ::core::Element(num).symbol()));
  }
  return result;
}

std::vector<int> ChemicalStructure::hydrogenBondDonors() const {
  std::vector<int> result;
  const auto nums = atomicNumbers();
  for (const auto &[i, j] : covalentBonds()) {
    if (nums(i) == 1) {
      result.push_back(j);
    } else if (nums(j) == 1) {
      result.push_back(i);
    }
  }
  return result;
}

QStringList ChemicalStructure::uniqueHydrogenDonorElements() const {
  if (numberOfAtoms() < 1)
    return {};

  ankerl::unordered_dense::set<int> uniqueDonors;
  const auto nums = atomicNumbers();
  for (const auto &idx : hydrogenBondDonors()) {
    uniqueDonors.insert(nums(idx));
  }

  QStringList result;
  for (const auto &num : uniqueDonors) {
    result.push_back(QString::fromStdString(occ::core::Element(num).symbol()));
  }
  return result;
}

void ChemicalStructure::deleteAtoms(
    const std::vector<GenericAtomIndex> &atoms) {
  std::vector<int> offsets;
  offsets.reserve(atoms.size());
  for (const auto &idx : atoms) {
    offsets.push_back(idx.unique);
  }
  deleteAtomsByOffset(offsets);
  updateBondGraph();
}

void ChemicalStructure::deleteAtomsByOffset(
    const std::vector<int> &atomIndices) {
  // DOES NOT UPDATE BONDS
  const int originalNumAtoms = numberOfAtoms();

  ankerl::unordered_dense::set<int> uniqueIndices;
  for (int idx : std::as_const(atomIndices)) {
    if (idx < originalNumAtoms)
      uniqueIndices.insert(idx);
  }
  const int numAtomsToRemove = uniqueIndices.size();
  const int newNumAtoms = originalNumAtoms - numAtomsToRemove;

  std::vector<QString> newLabels;
  occ::Mat3N newAtomicPositions(3, newNumAtoms);
  Eigen::VectorXi newAtomicNumbers(newNumAtoms);
  std::vector<QColor> newAtomColors;
  newLabels.reserve(newNumAtoms);
  newAtomColors.reserve(newNumAtoms);
  int newIndex = 0;
  for (int i = 0; i < originalNumAtoms; i++) {
    if (uniqueIndices.contains(i))
      continue;

    newLabels.push_back(m_labels[i]);
    newAtomicPositions.col(newIndex) = m_atomicPositions.col(i);
    newAtomicNumbers(newIndex) = m_atomicNumbers(i);
    newIndex++;
  }
  m_atomicNumbers = newAtomicNumbers;
  m_atomicPositions = newAtomicPositions;
  m_labels = newLabels;
  m_origin = m_atomicPositions.rowwise().mean();
  m_bondsNeedUpdate = true;
  emit atomsChanged();
}

void ChemicalStructure::deleteAtom(int atomIndex) {
  // DOES NOT UPDATE BONDS
  // TODO more efficient implementation for a single atom.
  deleteAtomsByOffset({atomIndex});
}

bool ChemicalStructure::anyAtomHasFlags(const AtomFlags &flags) const {
  for (const auto &[k, v] : m_flags) {
    if (v & flags)
      return true;
  }
  return false;
}

bool ChemicalStructure::atomsHaveFlags(
    const std::vector<GenericAtomIndex> &idxs, const AtomFlags &flags) const {
  // TODO check if this is correct, should probably be an and not an xor
  for (const auto &idx : idxs) {
    const auto loc = m_flags.find(idx);
    if (loc == m_flags.end())
      return false;
    if (loc->second ^ flags)
      return false;
  }
  return true;
}

bool ChemicalStructure::allAtomsHaveFlags(const AtomFlags &flags) const {
  for (const auto &[k, v] : m_flags) {
    if (v ^ flags)
      return false;
  }
  return true;
}

void ChemicalStructure::setFlagForAllAtoms(AtomFlag flag, bool on) {
  for (auto &[k, v] : m_flags) {
    v.setFlag(flag, on);
  }
  emit atomsChanged();
}

void ChemicalStructure::toggleAtomFlag(GenericAtomIndex idx, AtomFlag flag) {
  auto &v = m_flags[idx];
  v ^= flag;
  emit atomsChanged();
}

void ChemicalStructure::toggleFlagForAllAtoms(AtomFlag flag) {
  for (auto &[k, v] : m_flags) {
    v ^= flag;
  }
  emit atomsChanged();
}

void ChemicalStructure::selectFragmentContaining(int atom) {
  const auto fragIndex = fragmentIndexForAtom(atom);
  if (fragIndex.u < 0)
    return;
  for (const auto &idx : atomIndicesForFragment(fragIndex)) {
    setAtomFlags(idx, AtomFlag::Selected);
  }
}

void ChemicalStructure::selectFragmentContaining(GenericAtomIndex atom) {
  selectFragmentContaining(genericIndexToIndex(atom));
}

void ChemicalStructure::deleteFragmentContainingAtomIndex(int atomIndex) {
  const auto fragmentIndex = fragmentIndexForAtom(atomIndex);
  if (fragmentIndex.u < 0)
    return;
  const auto &fragIndices = atomIndicesForFragment(fragmentIndex);
  if (fragIndices.size() == 0)
    return;

  deleteAtoms(fragIndices);
  updateBondGraph();
}

std::vector<FragmentIndex> ChemicalStructure::completedFragments() const {
  std::vector<FragmentIndex> result;
  for (const auto &[fragmentIndex, fragment] : m_fragments) {
    const auto &fragIndices = atomIndicesForFragment(fragmentIndex);
    if (fragIndices.size() == 0)
      continue;
    result.push_back(fragmentIndex);
  }
  return result;
}

std::vector<FragmentIndex> ChemicalStructure::selectedFragments() const {
  std::vector<FragmentIndex> result;
  for (const auto &[fragmentIndex, fragment] : m_fragments) {
    const auto &fragIndices = atomIndicesForFragment(fragmentIndex);
    if (fragIndices.size() == 0)
      continue;
    if (atomsHaveFlags(fragIndices, AtomFlag::Selected))
      result.push_back(fragmentIndex);
  }
  return result;
}

bool ChemicalStructure::hasIncompleteFragments() const { return false; }
bool ChemicalStructure::hasIncompleteSelectedFragments() const { return false; }

void ChemicalStructure::deleteIncompleteFragments() {}

void ChemicalStructure::setFlagForAtoms(
    const std::vector<GenericAtomIndex> &atomIndices, AtomFlag flag, bool on) {
  for (const auto &idx : atomIndices) {
    setAtomFlag(idx, flag, on);
  }
  emit atomsChanged();
}

void ChemicalStructure::setFlagForAtomsFiltered(const AtomFlag &flagToSet,
                                                const AtomFlag &query,
                                                bool on) {
  for (auto &[k, v] : m_flags) {
    if (v & query) {
      v.setFlag(flagToSet, on);
    }
  }
  emit atomsChanged();
}

FragmentIndex ChemicalStructure::fragmentIndexForAtom(int atomIndex) const {
  return m_fragmentForAtom[atomIndex];
}

FragmentIndex
ChemicalStructure::fragmentIndexForAtom(GenericAtomIndex idx) const {
  return m_fragmentForAtom[idx.unique];
}

std::vector<HBondTriple>
ChemicalStructure::hydrogenBonds(const HBondCriteria &criteria) const {
  return criteria.filter(m_atomicPositions, m_atomicNumbers, m_covalentBonds,
                         m_hydrogenBonds);
}

std::vector<CloseContactPair>
ChemicalStructure::closeContacts(const CloseContactCriteria &criteria) const {
  return criteria.filter(m_atomicPositions, m_atomicNumbers, m_covalentBonds,
                         m_vdwContacts);
}

const std::vector<std::pair<int, int>> &
ChemicalStructure::covalentBonds() const {
  return m_covalentBonds;
}

const std::pair<int, int> &
ChemicalStructure::atomsForBond(int bondIndex) const {
  return m_covalentBonds.at(bondIndex);
}

std::pair<GenericAtomIndex, GenericAtomIndex>
ChemicalStructure::atomIndicesForBond(int bondIndex) const {
  auto [a, b] = atomsForBond(bondIndex);
  return {indexToGenericIndex(a), indexToGenericIndex(b)};
}

std::vector<GenericAtomIndex>
ChemicalStructure::atomIndicesForFragment(FragmentIndex fragmentIndex) const {
  const auto kv = m_fragments.find(fragmentIndex);
  if (kv == m_fragments.end())
    return {};
  return kv->second.atomIndices;
}

QColor ChemicalStructure::atomColor(GenericAtomIndex atomIndex) const {
  const auto loc = m_atomColorOverrides.find(atomIndex);
  if (loc != m_atomColorOverrides.end())
    return loc->second;

  int i = genericIndexToIndex(atomIndex);
  switch (m_atomColoring) {
  case AtomColoring::Element: {
    Element *el = ElementData::elementFromAtomicNumber(m_atomicNumbers(i));
    if (el != nullptr) {
      return el->color();
    } else
      return Qt::black;
  }
  case AtomColoring::Fragment: {
    auto fragIndex = fragmentIndexForAtom(atomIndex);
    return getFragmentColor(fragIndex);
  }
  case AtomColoring::Index:
    return Qt::black;
  }
  return Qt::black; // unreachable
}

void ChemicalStructure::overrideAtomColor(GenericAtomIndex index,
                                          const QColor &color) {
  m_atomColorOverrides[index] = color;
  emit atomsChanged();
}

void ChemicalStructure::setColorForAtomsWithFlags(const AtomFlags &flags,
                                                  const QColor &color) {
  for (const auto &[k, v] : m_flags) {
    if (v.testFlags(flags)) {
      m_atomColorOverrides[k] = color;
    }
  }
  emit atomsChanged();
}

void ChemicalStructure::resetAtomColorOverrides() {
  m_atomColorOverrides.clear();
  emit atomsChanged();
}

void ChemicalStructure::setAtomColoring(AtomColoring atomColoring) {
  m_atomColoring = atomColoring;
  emit atomsChanged();
}

int ChemicalStructure::genericIndexToIndex(const GenericAtomIndex &idx) const {
  return idx.unique;
}

GenericAtomIndex ChemicalStructure::indexToGenericIndex(int idx) const {
  return GenericAtomIndex{idx};
}

void ChemicalStructure::resetAtomsAndBonds(bool toSelection) {
  // TODO
}

void ChemicalStructure::setShowVanDerWaalsContactAtoms(bool state) {
  // TODO
}

void ChemicalStructure::completeFragmentContaining(int) {
  // TODO
}

void ChemicalStructure::completeFragmentContaining(GenericAtomIndex) {
  // TODO
}

void ChemicalStructure::completeAllFragments() {
  // TODO
}

void ChemicalStructure::packUnitCells(
    const QPair<QVector3D, QVector3D> &limits) {
  // TODO
}

void ChemicalStructure::expandAtomsWithinRadius(float radius, bool selected) {
  // TODO
}

std::vector<GenericAtomIndex>
ChemicalStructure::atomsWithFlags(const AtomFlags &flags, bool set) const {
  std::vector<GenericAtomIndex> result;
  for (const auto &[k, v] : m_flags) {
    bool check = v.testFlags(flags);
    if ((set && check) || (!set && !check))
      result.push_back(k);
  }
  return result;
}

std::vector<GenericAtomIndex> ChemicalStructure::atomsSurroundingAtoms(
    const std::vector<GenericAtomIndex> &idxs, float radius) const {
  ankerl::unordered_dense::set<GenericAtomIndex, GenericAtomIndexHash>
      unique_idxs;
  ankerl::unordered_dense::set<GenericAtomIndex, GenericAtomIndexHash> idx_set(
      idxs.begin(), idxs.end());

  occ::core::KDTree<double> tree(3, m_atomicPositions, occ::core::max_leaf);
  tree.index->buildIndex();
  const double max_dist2 = radius * radius;

  std::vector<std::pair<size_t, double>> idxs_dists;
  nanoflann::RadiusResultSet results(max_dist2, idxs_dists);

  for (const auto &idx : idx_set) {
    int i = idx.unique;
    const double *q = m_atomicPositions.col(i).data();
    tree.index->findNeighbors(results, q, nanoflann::SearchParams());
    for (const auto &result : idxs_dists) {
      auto candidate = GenericAtomIndex{static_cast<int>(result.first)};
      if (idx_set.contains(candidate)) {
        continue;
      }
      unique_idxs.insert(candidate);
    }
  }
  std::vector<GenericAtomIndex> res(unique_idxs.begin(), unique_idxs.end());
  return res;
}

std::vector<GenericAtomIndex>
ChemicalStructure::atomsSurroundingAtomsWithFlags(const AtomFlags &flags,
                                                  float radius) const {
  ankerl::unordered_dense::set<GenericAtomIndex, GenericAtomIndexHash>
      unique_idxs;

  occ::core::KDTree<double> tree(3, m_atomicPositions, occ::core::max_leaf);
  tree.index->buildIndex();
  const double max_dist2 = radius * radius;

  std::vector<std::pair<size_t, double>> idxs_dists;
  nanoflann::RadiusResultSet results(max_dist2, idxs_dists);

  for (GenericAtomIndex i = {0}; i.unique < numberOfAtoms(); i.unique++) {
    if (m_flags.at(i) & flags) {
      const double *q = m_atomicPositions.col(i.unique).data();
      tree.index->findNeighbors(results, q, nanoflann::SearchParams());
      for (const auto &result : idxs_dists) {
        int idx = result.first;
        unique_idxs.insert({idx});
      }
    }
  }

  std::vector<GenericAtomIndex> res(unique_idxs.begin(), unique_idxs.end());
  return res;
}

bool ChemicalStructure::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::ChildAdded) {
    QChildEvent *childEvent = static_cast<QChildEvent *>(event);
    QObject *newChild = childEvent->child();
    if (newChild) {
      newChild->installEventFilter(this); // Monitor the new child
      emit childAdded(newChild);
    }
  } else if (event->type() == QEvent::ChildRemoved) {
    QChildEvent *childEvent = static_cast<QChildEvent *>(event);
    QObject *removedChild = childEvent->child();
    if (removedChild) {
      removedChild->removeEventFilter(
          this); // Stop monitoring the removed child
      emit childRemoved(removedChild);
    }
  }
  return QObject::eventFilter(obj, event);
}

void ChemicalStructure::connectChildSignals(QObject *child) {
  child->installEventFilter(this); // Monitor the new child

  for (QObject *grandChild : child->children()) {
    connectChildSignals(grandChild);
  }
}

occ::IVec ChemicalStructure::atomicNumbersForIndices(
    const std::vector<GenericAtomIndex> &idxs) const {
  occ::IVec result(idxs.size());
  for (int i = 0; i < idxs.size(); i++) {
    result(i) = m_atomicNumbers(i % m_atomicNumbers.rows());
  }
  return result;
}

std::vector<QString> ChemicalStructure::labelsForIndices(
    const std::vector<GenericAtomIndex> &idxs) const {
  std::vector<QString> result;
  result.reserve(idxs.size());
  for (int i = 0; i < idxs.size(); i++) {
    result.push_back(m_labels[i]);
  }
  return result;
}

occ::Mat3N ChemicalStructure::atomicPositionsForIndices(
    const std::vector<GenericAtomIndex> &idxs) const {
  occ::Mat3N result(3, idxs.size());
  for (int i = 0; i < idxs.size(); i++) {
    result.col(i) = m_atomicPositions.col(idxs[i].unique);
  }
  return result;
}

std::vector<GenericAtomIndex>
ChemicalStructure::getAtomIndicesUnderTransformation(
    const std::vector<GenericAtomIndex> &idxs,
    const Eigen::Isometry3d &transform) const {
  std::vector<GenericAtomIndex> result;
  occ::Mat3N pos =
      (transform.rotation() * atomicPositionsForIndices(idxs)).colwise() +
      transform.translation();

  occ::core::KDTree<double> tree(3, m_atomicPositions, occ::core::max_leaf);
  tree.index->buildIndex();
  size_t idx;
  double d;
  nanoflann::KNNResultSet<double> resultSet(1);
  resultSet.init(&idx, &d);

  for (int i = 0; i < pos.cols(); i++) {
    occ::Vec3 position = pos.col(i);
    tree.index->findNeighbors(resultSet, position.data(),
                              nanoflann::SearchParams(10));
    if (d < 1e-3)
      result.push_back({static_cast<int>(idx)});
  }
  return result;
}

bool ChemicalStructure::getTransformation(
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
  occ::Vec3 centroid_a = pos_a.rowwise().mean();
  auto pos_b = atomicPositionsForIndices(to);
  occ::Vec3 centroid_b = pos_b.rowwise().mean();
  pos_a.array().colwise() -= centroid_a.array();
  pos_b.array().colwise() -= centroid_b.array();

  auto rot = occ::core::linalg::kabsch_rotation_matrix(
      pos_a, pos_b, false); // allow inversions

  // Apply rotation
  pos_a = (rot * pos_a).eval();

  double rmsd = (pos_a - pos_b).norm();
  // tolerance
  if (rmsd > 1e-3)
    return false;

  // Compute the translation
  occ::Vec3 translation = centroid_b - (rot * centroid_a);

  // Construct the final transformation
  result = Eigen::Isometry3d::Identity();
  result.linear() = rot;
  result.translation() = translation;

  return true;
}

std::vector<WavefunctionAndTransform>
ChemicalStructure::wavefunctionsAndTransformsForAtoms(
    const std::vector<GenericAtomIndex> &idxs) {
  std::vector<WavefunctionAndTransform> result;
  for (auto *child : children()) {
    MolecularWavefunction *wfn = qobject_cast<MolecularWavefunction *>(child);
    if (wfn) {
      WavefunctionAndTransform t{wfn};
      for (const auto &idx : wfn->atomIndices()) {
      }
      bool valid = getTransformation(wfn->atomIndices(), idxs, t.transform);

      if (valid) {
        result.push_back(t);
      }
    }
  }
  return result;
}

Fragment::State ChemicalStructure::getSymmetryUniqueFragmentState(
    FragmentIndex fragmentIndex) const {
  const auto kv = m_symmetryUniqueFragments.find(fragmentIndex);
  if (kv == m_symmetryUniqueFragments.end())
    return {};
  return kv->second.state;
}

void ChemicalStructure::setSymmetryUniqueFragmentState(
    FragmentIndex fragmentIndex, Fragment::State state) {
  const auto kv = m_symmetryUniqueFragments.find(fragmentIndex);
  if (kv == m_symmetryUniqueFragments.end())
    return;
  kv->second.state = state;
}

const FragmentMap &ChemicalStructure::symmetryUniqueFragments() const {
  return m_symmetryUniqueFragments;
}

QString
ChemicalStructure::formulaSumForAtoms(const std::vector<GenericAtomIndex> &idxs,
                                      bool richText) const {
  std::vector<QString> symbols;

  occ::IVec nums = atomicNumbersForIndices(idxs);
  for (int i = 0; i < nums.rows(); i++) {
    symbols.push_back(
        QString::fromStdString(occ::core::Element(nums(i)).symbol()));
  }
  return formulaSum(symbols, richText);
}

Fragment ChemicalStructure::makeFragment(
    const std::vector<GenericAtomIndex> &idxs) const {
  Fragment result;
  result.atomIndices = idxs;
  result.atomicNumbers = atomicNumbersForIndices(idxs);
  result.positions = atomicPositionsForIndices(idxs);
  auto [idx, transform] = findUniqueFragment(idxs);
  result.asymmetricFragmentIndex = idx;
  result.asymmetricFragmentTransform = transform;
  result.index = idx;
  return result;
}

ChemicalStructure::FragmentSymmetryRelation
ChemicalStructure::findUniqueFragment(
    const std::vector<GenericAtomIndex> &idxs) const {
  FragmentIndex result{-1};
  Eigen::Isometry3d transform;
  bool found = false;
  const auto &sym = symmetryUniqueFragments();
  for (const auto &[asymIndex, asym] : sym) {
    found = getTransformation(idxs, asym.atomIndices, transform);
    if (found) {
      result = asymIndex;
      break;
    }
  }
  if (result.u < 0) {
    qDebug() << "No asymmetric fragment found for " << idxs;
    result.u = sym.size();
    transform = Eigen::Isometry3d::Identity();
  } else {
    qDebug() << "Found matching fragment: " << result;
  }
  return {result, transform};
}

FragmentPairs
ChemicalStructure::findFragmentPairs(FragmentIndex keyFragment) const {
  FragmentPairs result;
  constexpr double tolerance = 1e-1;
  const auto &fragments = getFragments();
  const auto &uniqueFragments = symmetryUniqueFragments();
  const bool allFragments = keyFragment.u < 0;
  qDebug() << "Fragments" << fragments.size();
  qDebug() << "Unique fragments" << uniqueFragments.size();

  occ::core::DynamicKDTree<double> tree(occ::core::max_leaf);

  auto &pairs = result.uniquePairs;
  auto &molPairs = result.pairs;
  std::vector<FragmentIndex> candidateFragments;
  if (allFragments) {
    for (const auto &[idx, frag] : fragments) {
      candidateFragments.push_back(idx);
    }
  } else {
    candidateFragments.push_back(keyFragment);
  }

  for (const auto &fragIndexA : candidateFragments) {
    const auto &fragA = fragments.at(fragIndexA);
    const auto asymIndex = fragA.asymmetricFragmentIndex;
    for (const auto &[fragIndexB, fragB] : fragments) {
      if (allFragments && (fragIndexB <= fragIndexA))
        continue;

      double distance = (fragA.nearestAtom(fragB).distance);
      if (distance <= tolerance)
        continue;

      FragmentDimer d(fragA, fragB);
      d.index.a = fragIndexA;
      d.index.b = fragIndexB;

      // Create a 3D point for the current pair
      Eigen::Vector3d point(d.nearestAtomDistance, d.centroidDistance,
                            d.centerOfMassDistance);

      // Check if a similar pair already exists using the KD-tree
      bool found_identical = false;
      if (tree.size() > 0) {
        auto [ret_index, out_dist_sqr] = tree.nearest(point);
        if (out_dist_sqr <= tolerance * tolerance) {
          if (pairs[ret_index] == d) {
            found_identical = true;
          }
        }
      }

      if (!found_identical) {
        pairs.push_back(d);
        tree.addPoint(point);
      }
      molPairs[fragA.index].push_back({d, -1});
    }
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

  std::stable_sort(pairs.begin(), pairs.end(), fragmentDimerSortFunc);

  occ::core::DynamicKDTree<double> sortedTree(occ::core::max_leaf);
  for (size_t i = 0; i < pairs.size(); ++i) {
    Eigen::Vector3d point(pairs[i].nearestAtomDistance,
                          pairs[i].centroidDistance,
                          pairs[i].centerOfMassDistance);
    sortedTree.addPoint(point);
  }

  for (auto &[idx, vec] : molPairs) {
    std::stable_sort(vec.begin(), vec.end(), molPairSortFunc);
    for (auto &d : vec) {
      Eigen::Vector3d query(d.fragments.nearestAtomDistance,
                            d.fragments.centroidDistance,
                            d.fragments.centerOfMassDistance);
      auto [idx, dist_sqr] = sortedTree.nearest(query);
      if (dist_sqr > tolerance * tolerance) {
        qDebug() << "Warning: " << dist_sqr << "no similar fragment pair";
        continue;
      }
      const auto &pair = pairs[idx];
      if (pair == d.fragments) {
        d.uniquePairIndex = idx;
        d.forward = pair.index.equivalent(d.fragments.index);
      }
    }
  }
  qDebug() << "Unique dimers:" << pairs.size();
  return result;
}

CellIndexSet ChemicalStructure::occupiedCells() const {
  return CellIndexSet{CellIndex{0, 0, 0}};
}

QColor ChemicalStructure::getFragmentColor(FragmentIndex fragmentIndex) const {
  const auto kv = m_fragments.find(fragmentIndex);
  if (kv != m_fragments.end()) {
    return kv->second.color;
  }
  return Qt::white;
}

void ChemicalStructure::setFragmentColor(FragmentIndex fragment,
                                         const QColor &color) {
  auto kv = m_fragments.find(fragment);
  if (kv != m_fragments.end()) {
    kv->second.color = color;
    emit atomsChanged();
  }
}

void ChemicalStructure::setAllFragmentColors(const QColor &color) {
  for (auto &[fragIndex, frag] : m_fragments) {
    frag.color = color;
  }
  emit atomsChanged();
}

const FragmentMap &ChemicalStructure::getFragments() const {
  return m_fragments;
}

QString ChemicalStructure::chemicalFormula(bool richText) const {
  std::vector<QString> symbols;

  for (int i = 0; i < m_atomicNumbers.rows(); i++) {
    symbols.push_back(QString::fromStdString(
        occ::core::Element(m_atomicNumbers(i)).symbol()));
  }
  return formulaSum(symbols, richText);
}

std::vector<AtomicDisplacementParameters>
ChemicalStructure::atomicDisplacementParametersForAtoms(
    const std::vector<GenericAtomIndex> &idxs) const {
  return std::vector<AtomicDisplacementParameters>(idxs.size());
}

AtomicDisplacementParameters
ChemicalStructure::atomicDisplacementParameters(GenericAtomIndex idx) const {
  return AtomicDisplacementParameters{};
}

std::vector<GenericAtomIndex> ChemicalStructure::atomIndices() const {
  std::vector<GenericAtomIndex> result;
  result.reserve(numberOfAtoms());
  for (int i = 0; i < numberOfAtoms(); i++) {
    result.push_back(indexToGenericIndex(i));
  }
  return result;
}
