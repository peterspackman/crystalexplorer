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
  m_symmetryUniqueFragmentStates.clear();
  m_fragmentForAtom.clear();
  m_covalentBonds.clear();
  m_hydrogenBonds.clear();
  m_vdwContacts.clear();
  m_fragmentForAtom.resize(numberOfAtoms(), -1);

  std::vector<std::vector<int>> fragments;

  const auto &edges = m_bondGraph.edges();

  ankerl::unordered_dense::set<VertexDesc> visited;
  size_t currentFragmentIndex{0};

  auto covalentVisitor = [&](const VertexDesc &v, const VertexDesc &prev,
                             const EdgeDesc &e) {
    const auto &edge = edges.at(e);
    if (edge.connectionType != Connection::CovalentBond)
      return;
    auto &idxs = fragments[currentFragmentIndex];
    visited.insert(v);
    m_fragmentForAtom[v] = currentFragmentIndex;
    idxs.push_back(v);
    if (prev != v) {
    }
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
    m_fragments.push_back(makeFragment(sym));
    m_symmetryUniqueFragments.push_back(m_fragments.back());
    m_symmetryUniqueFragmentStates.push_back({});
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

const AtomFlags &ChemicalStructure::atomFlags(int index) const {
  return m_flags[index];
}

AtomFlags &ChemicalStructure::atomFlags(int index) { return m_flags[index]; }

void ChemicalStructure::setAtomFlags(int index, const AtomFlags &flags) {
  m_flags[index] = flags;
}

bool ChemicalStructure::atomFlagsSet(int index, const AtomFlags &flags) const {
  return m_flags[index] & flags;
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
  m_atomColors.clear();
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
  m_atomColors.clear();

  m_labels.reserve(N);
  m_flags.reserve(N);
  m_atomColors.reserve(N);
  m_fragments.reserve(N);
  m_fragmentForAtom.reserve(N);

  for (int i = 0; i < N; i++) {
    element = occ::core::Element(elementSymbols[i].toStdString());
    m_atomicNumbers(i) = element.atomic_number();
    m_atomColors.push_back(Qt::black);

    m_atomicPositions(0, i) = positions[i].x();
    m_atomicPositions(1, i) = positions[i].y();
    m_atomicPositions(2, i) = positions[i].z();

    if (labels.size() > i) {
      m_labels.push_back(labels[i]);
    } else {
      m_labels.push_back(elementSymbols[i]);
    }
    m_flags.push_back(AtomFlag::NoFlag);
  }
  m_origin = m_atomicPositions.rowwise().mean();
  m_bondsNeedUpdate = true;
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
  m_flags.resize(numTotal);
  m_atomColors.resize(numTotal);
  m_fragments.resize(numTotal);
  m_fragmentForAtom.resize(numTotal, -1);

  for (int i = 0; i < numAdded; i++) {
    element = occ::core::Element(elementSymbols[i].toStdString());
    int index = numOld + i;
    m_atomicNumbers(index) = element.atomic_number();
    m_atomColors[index] = Qt::black;

    m_atomicPositions(0, index) = positions[i].x();
    m_atomicPositions(1, index) = positions[i].y();
    m_atomicPositions(2, index) = positions[i].z();
    m_fragmentForAtom[index] = {index};

    if (labels.size() > i) {
      m_labels.push_back(labels[i]);
    } else {
      m_labels.push_back(elementSymbols[i]);
    }
    m_flags[index] = AtomFlag::NoFlag;
  }
  m_origin = m_atomicPositions.rowwise().mean();
  m_bondsNeedUpdate = true;
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
  std::vector<AtomFlags> newFlags;
  std::vector<QColor> newAtomColors;
  newLabels.reserve(newNumAtoms);
  newFlags.reserve(newNumAtoms);
  newAtomColors.reserve(newNumAtoms);
  int newIndex = 0;
  for (int i = 0; i < originalNumAtoms; i++) {
    if (uniqueIndices.contains(i))
      continue;

    newLabels.push_back(m_labels[i]);
    newAtomicPositions.col(newIndex) = m_atomicPositions.col(i);
    newAtomicNumbers(newIndex) = m_atomicNumbers(i);
    newFlags.push_back(m_flags[i]);
    newAtomColors.push_back(m_atomColors[i]);
    newIndex++;
  }
  m_atomicNumbers = newAtomicNumbers;
  m_atomicPositions = newAtomicPositions;
  m_labels = newLabels;
  m_flags = newFlags;
  m_atomColors = newAtomColors;
  m_origin = m_atomicPositions.rowwise().mean();
  m_bondsNeedUpdate = true;
}

void ChemicalStructure::deleteAtom(int atomIndex) {
  // DOES NOT UPDATE BONDS
  // TODO more efficient implementation for a single atom.
  deleteAtomsByOffset({atomIndex});
}

bool ChemicalStructure::anyAtomHasFlags(const AtomFlags &flags) const {
  for (int i = 0; i < numberOfAtoms(); i++) {
    if (m_flags[i] & flags)
      return true;
  }
  return false;
}

bool ChemicalStructure::atomsHaveFlags(const std::vector<int> &idxs,
                                       const AtomFlags &flags) const {
  // TODO check if this is correct, should probably be an and not an xor
  for (const auto &idx : idxs) {
    if (idx >= m_flags.size())
      return false;
    if (m_flags[idx] ^ flags)
      return false;
  }
  return true;
}

bool ChemicalStructure::allAtomsHaveFlags(const AtomFlags &flags) const {
  for (int i = 0; i < numberOfAtoms(); i++) {
    if (m_flags[i] ^ flags)
      return false;
  }
  return true;
}

std::vector<int>
ChemicalStructure::atomIndicesWithFlags(const AtomFlags &flags) const {
  std::vector<int> result;
  for (int i = 0; i < numberOfAtoms(); i++) {
    if (m_flags[i] & flags)
      result.push_back(i);
  }
  return result;
}

void ChemicalStructure::setFlagForAllAtoms(AtomFlag flag, bool on) {
  for (int i = 0; i < numberOfAtoms(); i++) {
    setAtomFlag(i, flag, on);
  }
}

void ChemicalStructure::toggleFlagForAllAtoms(AtomFlag flag) {
  for (int i = 0; i < numberOfAtoms(); i++) {
    m_flags[i] ^= (m_flags[i] & flag);
  }
}

void ChemicalStructure::selectFragmentContaining(int atom) {
  int fragIndex = fragmentIndexForAtom(atom);
  if (fragIndex < 0)
    return;
  for (int idx : atomsForFragment(fragIndex)) {
    setAtomFlags(idx, AtomFlag::Selected);
  }
}

void ChemicalStructure::deleteFragmentContainingAtomIndex(int atomIndex) {
  const auto &fragmentIndex = fragmentIndexForAtom(atomIndex);
  if (fragmentIndex < 0)
    return;
  const auto &fragIndices = atomsForFragment(fragmentIndex);
  if (fragIndices.size() == 0)
    return;

  deleteAtomsByOffset(fragIndices);
  updateBondGraph();
}

std::vector<int> ChemicalStructure::completedFragments() const {
  std::vector<int> result;
  for (int fragmentIndex = 0; fragmentIndex < m_fragments.size();
       fragmentIndex++) {
    const auto &fragIndices = atomsForFragment(fragmentIndex);
    if (fragIndices.size() == 0)
      continue;
    result.push_back(fragmentIndex);
  }
  return result;
}

std::vector<int> ChemicalStructure::selectedFragments() const {
  std::vector<int> result;
  for (int fragmentIndex = 0; fragmentIndex < m_fragments.size();
       fragmentIndex++) {
    const auto &fragIndices = atomsForFragment(fragmentIndex);
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

void ChemicalStructure::setFlagForAtoms(const std::vector<int> &atomIndices,
                                        AtomFlag flag, bool on) {
  for (const auto &idx : atomIndices) {
    setAtomFlag(idx, flag, on);
  }
}

void ChemicalStructure::setFlagForAtomsFiltered(const AtomFlag &flagToSet,
                                                const AtomFlag &query,
                                                bool on) {
  for (int i = 0; i < numberOfAtoms(); i++) {
    if (atomFlagsSet(i, query)) {
      setAtomFlag(i, flagToSet, on);
    }
  }
}

int ChemicalStructure::fragmentIndexForAtom(int atomIndex) const {
  return m_fragmentForAtom[atomIndex];
}

std::vector<HBondTriple>
ChemicalStructure::hydrogenBonds(const HBondCriteria &criteria) const {
  return criteria.filter(m_atomicPositions, m_atomicNumbers, m_covalentBonds, m_hydrogenBonds);
}


std::vector<CloseContactPair> ChemicalStructure::closeContacts(const CloseContactCriteria &criteria) const {
  return criteria.filter(m_atomicPositions, m_atomicNumbers, m_covalentBonds, m_vdwContacts);
}

const std::vector<std::pair<int, int>> &
ChemicalStructure::covalentBonds() const {
  return m_covalentBonds;
}

const std::pair<int, int> &
ChemicalStructure::atomsForBond(int bondIndex) const {
  return m_covalentBonds.at(bondIndex);
}

const std::vector<int> &
ChemicalStructure::atomsForFragment(int fragIndex) const {
  return m_fragments.at(fragIndex)._atomOffset;
}

std::vector<GenericAtomIndex>
ChemicalStructure::atomIndicesForFragment(int fragmentIndex) const {
  if (fragmentIndex < 0 || fragmentIndex >= m_fragments.size())
    return {};
  return m_fragments[fragmentIndex].atomIndices;
}

QColor ChemicalStructure::atomColor(int atomIndex) const {
  const auto loc = m_atomColorOverrides.find(atomIndex);
  if (loc != m_atomColorOverrides.end())
    return loc->second;
  switch (m_atomColoring) {
  case AtomColoring::Element: {
    Element *el =
        ElementData::elementFromAtomicNumber(m_atomicNumbers(atomIndex));
    if (el != nullptr) {
      return el->color();
    } else
      return m_atomColors[atomIndex];
  }
  case AtomColoring::Fragment:
    return Qt::white;
  case AtomColoring::Index:
    return Qt::black;
  }
  return Qt::black; // unreachable
}

void ChemicalStructure::overrideAtomColor(int index, const QColor &color) {
  m_atomColorOverrides[index] = color;
}

void ChemicalStructure::resetAtomColorOverrides() {
  m_atomColorOverrides.clear();
}

void ChemicalStructure::setAtomColoring(AtomColoring atomColoring) {
  m_atomColoring = atomColoring;
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
  for (int i = 0; i < numberOfAtoms(); i++) {
    bool check = m_flags[i].testFlags(flags);
    if((set && check) || (!set && !check)) result.push_back({i});
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

  for (int i = 0; i < numberOfAtoms(); i++) {
    if (m_flags[i] & flags) {
      const double *q = m_atomicPositions.col(i).data();
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

std::vector<QString> ChemicalStructure::labelsForIndices(const std::vector<GenericAtomIndex> &idxs) const {
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
    result.col(i) = m_atomicPositions.col(i % m_atomicPositions.cols());
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

ChemicalStructure::FragmentState
ChemicalStructure::getSymmetryUniqueFragmentState(int fragmentIndex) const {
  if (fragmentIndex < 0 ||
      fragmentIndex >= m_symmetryUniqueFragmentStates.size())
    return {};
  return m_symmetryUniqueFragmentStates[fragmentIndex];
}

void ChemicalStructure::setSymmetryUniqueFragmentState(
    int fragmentIndex, ChemicalStructure::FragmentState state) {
  if (fragmentIndex < 0 ||
      fragmentIndex >= m_symmetryUniqueFragmentStates.size())
    return;
  m_symmetryUniqueFragmentStates[fragmentIndex] = state;
}

const std::vector<Fragment> &
ChemicalStructure::symmetryUniqueFragments() const {
  return m_symmetryUniqueFragments;
}

const std::vector<ChemicalStructure::FragmentState> &
ChemicalStructure::symmetryUniqueFragmentStates() const {
  return m_symmetryUniqueFragmentStates;
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
  std::transform(idxs.begin(), idxs.end(),
                 std::back_inserter(result._atomOffset),
                 [](const GenericAtomIndex &idx) { return idx.unique; });
  result.atomicNumbers = atomicNumbersForIndices(idxs);
  result.positions = atomicPositionsForIndices(idxs);
  std::tie(result.asymmetricFragmentIndex, result.asymmetricFragmentTransform) =
      findUniqueFragment(idxs);
  return result;
}

ChemicalStructure::FragmentSymmetryRelation
ChemicalStructure::findUniqueFragment(
    const std::vector<GenericAtomIndex> &idxs) const {
  int result = -1;
  Eigen::Isometry3d transform;
  bool found = false;
  const auto &sym = symmetryUniqueFragments();
  for (int i = 0; i < sym.size(); i++) {
    found = getTransformation(idxs, sym[i].atomIndices, transform);
    if (found) {
      result = i;
      break;
    }
  }
  return {result, transform};
}

FragmentPairs ChemicalStructure::findFragmentPairs(int keyFragment) const {
    FragmentPairs result;
    constexpr double tolerance = 1e-1;
    const auto &fragments = getFragments();
    const auto &uniqueFragments = symmetryUniqueFragments();

    occ::core::DynamicKDTree<double> tree(occ::core::max_leaf);

    auto &pairs = result.uniquePairs;
    auto &molPairs = result.pairs;
    molPairs.resize(uniqueFragments.size());

    std::vector<size_t> candidateFragments;
    if (keyFragment < 0) {
        for (size_t fragIndexA = 0; fragIndexA < fragments.size(); fragIndexA++) {
            candidateFragments.push_back(fragIndexA);
        }
    } else {
        candidateFragments.push_back(keyFragment);
    }

    for (size_t fragIndexA : candidateFragments) {
        const auto &fragA = fragments[fragIndexA];
        const size_t asymIndex = fragA.asymmetricFragmentIndex;

        for (size_t fragIndexB = 0; fragIndexB < fragments.size(); fragIndexB++) {
            if ((keyFragment < 0) && (fragIndexB <= fragIndexA)) continue;

            const auto &fragB = fragments[fragIndexB];
            double distance = (fragA.nearestAtom(fragB).distance);
            if (distance <= tolerance) continue;

            FragmentDimer d(fragA, fragB);

            // Create a 3D point for the current pair
            Eigen::Vector3d point(d.nearestAtomDistance, d.centroidDistance, d.centerOfMassDistance);

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
            molPairs[asymIndex].push_back({d, -1});
        }
    }

    // Sort the pairs
    auto fragmentDimerSortFunc = [](const FragmentDimer &a, const FragmentDimer &b) {
        return a.nearestAtomDistance < b.nearestAtomDistance;
    };
    auto molPairSortFunc = [](const FragmentPairs::SymmetryRelatedPair &a,
                              const FragmentPairs::SymmetryRelatedPair &b) {
        return a.fragments.nearestAtomDistance < b.fragments.nearestAtomDistance;
    };

    std::stable_sort(pairs.begin(), pairs.end(), fragmentDimerSortFunc);

    occ::core::DynamicKDTree<double> sortedTree(occ::core::max_leaf);
    for (size_t i = 0; i < pairs.size(); ++i) {
        Eigen::Vector3d point(pairs[i].nearestAtomDistance, pairs[i].centroidDistance, pairs[i].centerOfMassDistance);
        sortedTree.addPoint(point);
    }

    for (auto &vec : molPairs) {
        std::stable_sort(vec.begin(), vec.end(), molPairSortFunc);
        for (auto &d : vec) {
            Eigen::Vector3d query(d.fragments.nearestAtomDistance, d.fragments.centroidDistance, d.fragments.centerOfMassDistance);
            auto [idx, dist_sqr] = sortedTree.nearest(query);
            if(dist_sqr > tolerance * tolerance) continue;
            if(pairs[idx] == d.fragments) {
              d.uniquePairIndex = idx;
            }
        }
    }
    return result;
}

const std::vector<Fragment> &ChemicalStructure::getFragments() const {
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
