#include "chemicalstructure.h"
#include <occ/core/element.h>
#include <occ/core/kdtree.h>
#include "elementdata.h"
#include <QIcon>
#include <QEvent>

ChemicalStructure::ChemicalStructure(QObject *parent) : QAbstractItemModel(parent), m_interactions(new DimerInteractions(this)) {}

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
  m_fragmentForAtom.clear();
  m_covalentBonds.clear();
  m_hydrogenBonds.clear();
  m_vdwContacts.clear();
  m_fragmentForAtom.resize(numberOfAtoms(), -1);

  const auto &edges = m_bondGraph.edges();

  ankerl::unordered_dense::set<VertexDesc> visited;
  size_t currentFragmentIndex{0};

  auto covalentVisitor = [&](const VertexDesc &v, const VertexDesc &prev,
                             const EdgeDesc &e) {
    const auto &edge = edges.at(e);
    if (edge.connectionType != Connection::CovalentBond)
      return;
    auto &idxs = m_fragments[currentFragmentIndex];
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
    m_fragments.push_back({});
    m_bondGraph.breadth_first_traversal_with_edge_filtered(
        v.first, covalentVisitor, covalentPredicate);
    currentFragmentIndex++;
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
      if(el) {
	  radius = el->covRadius();
      }
      else {
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
      if(el) {
	  radius = el->vdwRadius();
      }
      else {
	  auto element = occ::core::Element(m_atomicNumbers(i));
	  radius = element.van_der_waals_radius();
      }
      result(i) = (radius > 0.0) ? radius : 2.0;
  }
  return result;
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
    m_fragments.push_back({i});
    m_fragmentForAtom.push_back(i);

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
    m_fragments[index] = {index};
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

void ChemicalStructure::deleteAtoms(const std::vector<int> &atomIndices) {
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
  deleteAtoms({atomIndex});
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

  deleteAtoms(fragIndices);
  updateBondGraph();
}

bool ChemicalStructure::hasIncompleteFragments() const { return false; }

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

const std::vector<std::pair<int, int>> &
ChemicalStructure::hydrogenBonds() const {
  return m_hydrogenBonds;
}

const std::vector<std::pair<int, int>> &ChemicalStructure::vdwContacts() const {
  return m_vdwContacts;
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
  return m_fragments.at(fragIndex);
}

QColor ChemicalStructure::atomColor(int atomIndex) const {
  const auto loc = m_atomColorOverrides.find(atomIndex);
  if (loc != m_atomColorOverrides.end())
    return loc->second;
  switch (m_atomColoring) {
  case AtomColoring::Element: {
      Element *el = ElementData::elementFromAtomicNumber(m_atomicNumbers(atomIndex));
      if (el != nullptr) {
	return el->color();
      }
      else return m_atomColors[atomIndex];
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


std::vector<GenericAtomIndex> ChemicalStructure::atomsSurroundingAtomsWithFlags(const AtomFlags &flags, float radius) const {
    ankerl::unordered_dense::set<GenericAtomIndex, GenericAtomIndexHash> unique_idxs;

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

void ChemicalStructure::childEvent(QChildEvent *event) {
    // MAKE SURE TO DO THIS
    QAbstractItemModel::childEvent(event); 
    if (event->type() == QEvent::ChildAdded) {
	QObject* child = event->child();
	emit childAdded(child);

	// insert the child into the model
        if (child) {
            int newRow = this->children().indexOf(child);
            QModelIndex parentIndex = QModelIndex();
            beginInsertRows(parentIndex, newRow, newRow);
            endInsertRows();
        }
    } else if (event->type() == QEvent::ChildRemoved) {
	beginResetModel();
	endResetModel();
        emit childRemoved(event->child());
    }
}

occ::IVec ChemicalStructure::atomicNumbersForIndices(const std::vector<GenericAtomIndex> &idxs) const {
    occ::IVec result(idxs.size());
    for(int i = 0; i < idxs.size(); i++) {
	result(i) = m_atomicNumbers(i % m_atomicNumbers.rows());
    }
    return result;
}

occ::Mat3N ChemicalStructure::atomicPositionsForIndices(const std::vector<GenericAtomIndex> &idxs) const {
    occ::Mat3N result(3, idxs.size());
    for(int i = 0; i < idxs.size(); i++) {
	result.col(i) = m_atomicPositions.col(i % m_atomicPositions.cols());
    }
    return result;
}

// Abstract Item Model methods
int ChemicalStructure::topLevelItemsCount() const {
    // Implement this based on how top-level items are stored or managed within ChemicalStructure
    return this->children().count(); // Example: directly using QObject's children count
}

int ChemicalStructure::rowCount(const QModelIndex &parent) const {
    if (!parent.isValid()) {
	// Return the count of top-level items when parent is invalid (root case)
	return topLevelItemsCount();
    } else {
	// For a valid parent index, find the QObject and return its children count
	const QObject* parentObject = static_cast<QObject*>(parent.internalPointer());
	return parentObject->children().count();
    }
}

QVariant ChemicalStructure::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) {
        return QVariant();
    }
    
    if (orientation == Qt::Horizontal) {
        // Assuming column 0 is for "Visibility" and column 1 is for "Name" as an example
        switch (section) {
            case 0:
                return tr("Visibility");
            case 1:
                return tr("Name");
            // Add more cases as needed for additional columns
            default:
                return QVariant();
        }
    }

    // Optionally handle vertical headers or return QVariant() if not needed
    return QVariant();
}


int ChemicalStructure::columnCount(const QModelIndex &parent) const {
    // This typically doesn't depend on the parent for hierarchical models
    return 2; // Or more, depending on the data you wish to display
}

QVariant ChemicalStructure::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
	return QVariant();

    int col = index.column();
    QObject* itemObject = static_cast<QObject*>(index.internalPointer());
    if (role == Qt::DecorationRole && col == 0) { // Visibility column
        QVariant visibleProperty = itemObject->property("visible");
        if (!visibleProperty.isNull()) {
            bool isVisible = visibleProperty.toBool();
            return QIcon(isVisible ? ":/images/tick.png" : ":/images/cross.png");
        }
    }
    else if (role == Qt::DisplayRole && col == 1) {
	return QVariant(itemObject->objectName() + QString(" [%1]").arg(itemObject->metaObject()->className()));
    }
    return QVariant();
}

QModelIndex ChemicalStructure::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent))
	return QModelIndex();

    const QObject* parentObject = parent.isValid() ? static_cast<const QObject*>(parent.internalPointer()) : this;
    QObject* childObject = parentObject->children().at(row);
    if (childObject)
	return createIndex(row, column, childObject);
    else
	return QModelIndex();
}

QModelIndex ChemicalStructure::parent(const QModelIndex &index) const {
    if (!index.isValid())
	return QModelIndex();

    QObject* childObject = static_cast<QObject*>(index.internalPointer());
    QObject* parentObject = childObject->parent();

    if (parentObject == this || !parentObject)
	return QModelIndex();

    QObject* grandparentObject = parentObject->parent();
    const int parentRow = grandparentObject ? grandparentObject->children().indexOf(parentObject) : 0;
    return createIndex(parentRow, 0, parentObject);
}
