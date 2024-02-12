#include <QSignalBlocker>
#include <QString>
#include <QtDebug>

#include "atomid.h"
#include "deprecatedcrystal.h"
#include "elementdata.h"
#include "globals.h"
#include "infodocuments.h"
#include "jobparameters.h"
#include "settings.h"
#include "stldatastream.h"
#include "surface.h"
#include "surfacedata.h"
#include <occ/core/kdtree.h>

// Utility functions

int roundLimit(float limit) {
  // Positive real numbers increase "upward" to nearest integer.
  // Negative real numbers decrease "downward" to nearest integer.
  // So, e.g. +1.36 --> +2;  -1.36 --> -2;
  return (limit < 0.0) ? floor(limit) : ceil(limit);
}

float minShiftLimit(int shift, float minPackingLimit) {
  float result = 0.0;

  if (shift < 0.0) { //-ve shift
    float d = fabs(shift - minPackingLimit);
    if (d < 1.0) { // complete
      result = d;
    }
  }

  return result;
}

float maxShiftLimit(int shift, float maxPackingLimit) {
  float result = 1.0;

  if (shift >= 0.0) { //+ve shift
    float d = fabs(shift - maxPackingLimit);
    if (d < 1.0) { // complete;
      result = d;
    }
  }
  return result;
}

template <class F>
void iterate_over_cell_limits(F &func,
                              const QPair<QVector3D, QVector3D> &packingLimits,
                              bool not000 = false) {

  /*
   If packingLimits = [-1.3,1.3] x [-1.6,1.6] x [-2.4,2.4]
   then   h_min/max = [-2,2]     x [-2,2]     x [-3,3]
   */
  const auto &lower = packingLimits.first;
  const auto &upper = packingLimits.second;
  int h1min = roundLimit(lower[0]);
  int h1max = roundLimit(upper[0] + CELL_DELTA);
  int h2min = roundLimit(lower[1]);
  int h2max = roundLimit(upper[1] + CELL_DELTA);
  int h3min = roundLimit(lower[2]);
  int h3max = roundLimit(upper[2] + CELL_DELTA);

  for (int h1 = h1min; h1 < h1max; h1++) {
    for (int h2 = h2min; h2 < h2max; h2++) {
      for (int h3 = h3min; h3 < h3max; h3++) {
        if (not000 && h1 == 0 && h2 == 0 && h3 == 0) {
          continue;
        }
        func(h1, h2, h3);
      }
    }
  }
}

DeprecatedCrystal::DeprecatedCrystal() : QObject() { init(); }

DeprecatedCrystal::~DeprecatedCrystal() {}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Initialization
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void DeprecatedCrystal::setCrystalCell(QString formula, QString HM_Symbol,
                                       float a, float b, float c, float alpha,
                                       float beta, float gamma) {
  _formula = formula;
  _unitCell = UnitCell(a, b, c, alpha, beta, gamma);
  _spaceGroup = SpaceGroup(HM_Symbol);
}

// brief Set the list of unit cell atoms
// (labels, positions, occupancies etc) when processing the
// CIF file or when processing the tonto .cxc file
void DeprecatedCrystal::setUnitCellAtoms(const QVector<Atom> &atoms) {
  _unitCellAtomList = atoms;

  Q_ASSERT(_unitCellAtomList.size() > 0);

  // Set the atomList atom indices (shifts are already zero)
  for (int a = 1; a < _unitCellAtomList.size(); a++) {
    _unitCellAtomList[a].setUnitCellAtomIndexTo(a);
  }
  makeListOfElementSymbols();
  makeListOfDisorderGroups();
}

void DeprecatedCrystal::setSymopsForUnitCellAtoms(
    const std::vector<int> &symopsForUnitCellAtoms) {
  _symopsForUnitCellAtoms = symopsForUnitCellAtoms;
}

// Set the asymmetric unit atoms
// indices in the unit cell and the corresponding shifts
// (often the asymmetric unit atoms given in the CIF does
// not lie within the unit cell).
void DeprecatedCrystal::setAsymmetricUnitIndicesAndShifts(
    const QMap<int, Shift> &asymUnit) {
  _asymmetricUnitIndicesAndShifts.clear();
  for(auto kv = asymUnit.constKeyValueBegin(); kv != asymUnit.constKeyValueEnd(); kv++) {
    _asymmetricUnitIndicesAndShifts.insert(kv->first, kv->second);

  }

  Q_ASSERT(_asymmetricUnitIndicesAndShifts.keys().size() > 0);

  makeSymopMappingTable();
}

void DeprecatedCrystal::setCrystalName(QString crystalName) {
  _crystalName = crystalName;
  // Steve Wolff:  This has been commented out as it results in problems with
  // Tonto generating surfaces.
  //_crystalName.remove(QRegExp("^CSD_CIF_"));
}

void DeprecatedCrystal::postReadingInit() {
  calculateUnitCellCartesianCoordinates();
  makeConnectionTables();
  resetToAsymmetricUnit();
  makeListOfHydrogenDonors();
}

// brief Create a set of integers corresponding to each disorder group in the
// crystal.
// If atom has a disorder group integer = 0, then assume there is no disorder
// for this atom, so don't add to the set.
void DeprecatedCrystal::makeListOfDisorderGroups() {
  QSet<int> set;
  set.clear();
  for (int i = 0; i < _unitCellAtomList.size(); i++) {
    int disorderGroup = _unitCellAtomList[i].disorderGroup();
    if (disorderGroup !=
        0) { // Assume 0 means there is no disorder for this atom.
      set << disorderGroup;
    }
  }
  _disorderGroups = set.values().toVector();
}

void DeprecatedCrystal::makeSymopMappingTable() {
  int nAtoms = _unitCellAtomList.size();

  // initialise the table
  m_symopMappingTable = MatrixXq(nAtoms, nAtoms);
  m_symopMappingTable.setConstant(NOSYMOP);

  // For each atom in asymmetric unit (1)
  // Get the atoms in the unit cell that are symmetry related (2)
  // For each unique pair (i,j) of unit cell atoms that are symmetry related (3)
  // Determine symop for i-->j and store it in the symop mapping table (4)
  // Determine the inverse i.e. j-->i and store it too (5)
  // As sanity test calculate j-->i by alternative method and check they are the
  // same (6)

  foreach (int asymAtomIndex, _asymmetricUnitIndicesAndShifts.keys()) { // (1)
    QVector<int> symmetryRelatedAtoms =
        symmetryRelatedUnitCellAtomsForUnitCellAtom(asymAtomIndex); // (2)

    for (int k = 0; k < symmetryRelatedAtoms.size(); ++k) { //
      for (int l = 0; l <= k; ++l) {                        //
        int i = symmetryRelatedAtoms[k];                    //
        int j = symmetryRelatedAtoms[l];                    // (3)

        int p = _symopsForUnitCellAtoms[j];                            //
        int q = spaceGroup().inverseSymop(_symopsForUnitCellAtoms[i]); //
        int i_to_j = spaceGroup().symopProduct(p, q);                  //
        m_symopMappingTable(i, j) = i_to_j;                            // (4)

        int j_to_i = spaceGroup().inverseSymop(i_to_j); //
        m_symopMappingTable(j, i) = j_to_i;             // (5)

#ifdef QT_DEBUG                         // sanity test
        p = _symopsForUnitCellAtoms[i]; //
        q = spaceGroup().inverseSymop(_symopsForUnitCellAtoms[j]); //
        int j_to_i_alt = spaceGroup().symopProduct(p, q);          //
        Q_ASSERT(j_to_i == j_to_i_alt);                            // (6)
#endif
      }
    }
  }
}

void DeprecatedCrystal::init() {
  _covalentCutOff = 10.0f;
  _vdwCutOff = 10.0f;

  _includeIntraHBonds = false;
  _hbondDonor = ANY_ITEM;
  _hbondAcceptor = ANY_ITEM;
  _hbondDistanceCriteria = 0.0;
  _closeContactsX << ANY_ITEM << ANY_ITEM << ANY_ITEM;
  _closeContactsY << ANY_ITEM << ANY_ITEM << ANY_ITEM;
  for (int i = 0; i <= CCMAX_INDEX; ++i) {
    _closeContactsTable << ContactsList();
  }
}

void DeprecatedCrystal::calculateUnitCellCartesianCoordinates() {
  for (Atom &atom : _unitCellAtomList) {
    atom.evaluateCartesianCoordinates(_unitCell.directCellMatrix());
  }
}

void DeprecatedCrystal::updateAtomListInfo() { updateConnectivityInfo(); }

void DeprecatedCrystal::clearUnitCellAtomList() { _unitCellAtomList.clear(); }

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Atom List Connectivity
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void DeprecatedCrystal::updateConnectivityInfo() {
  calculateConnectivityInfo(m_atoms);
}

void DeprecatedCrystal::calculateConnectivityInfo(const QVector<Atom> &atoms) {
  m_bondedAtomsForAtom.clear();
  _atomsForBond.clear();
  m_bondsForAtom.clear();
  m_atomsForFragment.clear();
  _fragmentForAtom.clear();

  if (atoms.size() > 0) {
    calculateCovalentBondInfo(atoms);
    calculateFragmentInfo(atoms);
    calculateVdwContactInfo();
  }
}

void DeprecatedCrystal::calculateCovalentBondInfo(const QVector<Atom> &atoms) {

  int N = atoms.size();
  Eigen::VectorXd bondMatrix((N * (N + 1)) / 2);
  bondMatrix.setConstant(-1.0);
  Eigen::Matrix3Xd cart_pos(3, N);
  Eigen::VectorXd cov_radius(N);
  for (int i = 0; i < N; i++) {
    const Atom &a = atoms[i];
    cart_pos.col(i) = a.posvector();
    if (a.isContactAtom() || a.isSuppressed()) {
      cov_radius(i) = -1.0;
    } else {
      cov_radius(i) = a.covRadius();
    }
  }

  occ::core::KDTree<double> tree(cart_pos.rows(), cart_pos,
                                 occ::core::max_leaf);
  tree.index->buildIndex();
  double max_cov = 2.0;
  double max_dist = (max_cov * 2 + 0.4) * (max_cov * 2 + 0.4);
  std::vector<std::pair<size_t, double>> idxs_dists;
  nanoflann::RadiusResultSet results(max_dist, idxs_dists);
  m_bondedAtomsForAtom.resize(N);
  m_bondsForAtom.resize(N);
  // TODO clean this up
  int bonds = 0;
  for (int i = 0; i < N; i++) {
    if (cov_radius(i) < 0)
      continue;
    double *q = cart_pos.col(i).data();
    tree.index->findNeighbors(results, q, nanoflann::SearchParams());

    for (const auto &r : idxs_dists) {
      size_t j = r.first;
      if (j >= i || cov_radius(j) < 0)
        continue;
      int k = (i * (i - 1)) / 2 + j;
      double tmp = (cov_radius(i) + cov_radius(j));
      double l = (tmp - BONDING_TOLERANCE) * (tmp - BONDING_TOLERANCE);
      double u = (tmp + BONDING_TOLERANCE) * (tmp + BONDING_TOLERANCE);
      if ((l < r.second) && (r.second < u)) {
        bondMatrix(k) = std::sqrt(r.second);
        m_bondedAtomsForAtom[i].push_back(j);
        m_bondedAtomsForAtom[j].push_back(i);
        _atomsForBond.append({i, static_cast<int>(j)});
        m_bondsForAtom[i].push_back(bonds);
        m_bondsForAtom[j].push_back(bonds);
        bonds++;
      }
    }
    idxs_dists.clear();
  }
}

void DeprecatedCrystal::calculateFragmentInfo(const QVector<Atom> &atoms) {
  int fragment = 0;
  _fragmentForAtom.resize(atoms.size());
  m_atomsForFragment.clear();
  std::fill(_fragmentForAtom.begin(), _fragmentForAtom.end(), -1);

  for (int i = 0; i < atoms.size(); i++) {
    if (_fragmentForAtom[i] > -1)
      continue;
    QVector<int> atomsInFragment;
    std::stack<int> atomsToProcessForFragment;
    atomsToProcessForFragment.push(i);

    while (!atomsToProcessForFragment.empty()) {

      int atom = atomsToProcessForFragment.top();
      atomsToProcessForFragment.pop();

      if (_fragmentForAtom[atom] == -1) {
        atomsInFragment.append(atom);
        _fragmentForAtom[atom] = fragment;

        for (const auto &x : m_bondedAtomsForAtom[atom]) {
          atomsToProcessForFragment.push(x);
        }
      }
    }
    m_atomsForFragment.push_back(atomsInFragment);
    fragment++;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Connection Tables
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void DeprecatedCrystal::makeConnectionTables() {
  makeUnitCellConnectionTableAlt();
  makeVdwCellConnectionTableAlt();
}

void DeprecatedCrystal::makeUnitCellConnectionTableAlt() {
  Q_ASSERT(_elementSymbols.size() != 0);

  float biggestBond = 0.0;

  for (int i = 0; i < _elementSymbols.size(); ++i) {
    for (int j = 0; j <= i; ++j) {
      Element *element_i = ElementData::elementFromSymbol(_elementSymbols[i]);
      Element *element_j = ElementData::elementFromSymbol(_elementSymbols[j]);
      float sumOfCovRadii = element_i->covRadius() + element_j->covRadius();
      biggestBond = qMax(biggestBond, sumOfCovRadii + BONDING_TOLERANCE);
    }
  }

  const QVector<Atom> atoms = packedUnitCellsAtomList(
      fractionalPackingLimitsFromPadding(biggestBond), false);

  for (int i = 0; i < _unitCellAtomList.size(); ++i) {
    _unitCellConnectionTable.append(Connection());
    for (int j = 0; j < atoms.size(); ++j) {
      const Atom &atom_i = _unitCellAtomList[i];
      const Atom &atom_j = atoms[j];

      if (!atom_j.isSameAtom(atom_i)) {
        if (areCovalentBondedAtoms(atom_i, atom_j)) {
          _unitCellConnectionTable[i].insert(atom_j.unitCellAtomIndex(),
                                             atom_j.unitCellShift());
        }
      }
    }
  }
}

void DeprecatedCrystal::makeVdwCellConnectionTableAlt() {
  Q_ASSERT(_elementSymbols.size() != 0);

  float biggestBond = 0.0;

  for (int i = 0; i < _elementSymbols.size(); ++i) {
    for (int j = 0; j <= i; ++j) {
      Element *element_i = ElementData::elementFromSymbol(_elementSymbols[i]);
      Element *element_j = ElementData::elementFromSymbol(_elementSymbols[j]);
      float sumOfVdwRadii = element_i->vdwRadius() + element_j->vdwRadius();
      biggestBond = qMax(biggestBond, sumOfVdwRadii);
    }
  }

  const QVector<Atom> atoms = packedUnitCellsAtomList(
      fractionalPackingLimitsFromPadding(biggestBond), false);

  for (int i = 0; i < _unitCellAtomList.size(); ++i) {
    _vdwCellConnectionTable.append(Connection());
    for (int j = 0; j < atoms.size(); ++j) {
      const Atom &atom_i = _unitCellAtomList[i];
      const Atom &atom_j = atoms[j];

      if (_unitCellConnectionTable[i].keys().contains(
              atom_j.unitCellAtomIndex())) {
        continue; // skip covalently bonded pairs
      }

      if (!atom_j.isSameAtom(atom_i)) {
        if (areVdwBondedAtoms(atom_i, atom_j)) {
          _vdwCellConnectionTable[i].insert(atom_j.unitCellAtomIndex(),
                                            atom_j.unitCellShift());
        }
      }
    }
  }
}

void DeprecatedCrystal::makeUnitCellConnectionTable() {
  Q_ASSERT(_unitCellConnectionTable.size() == 0);

  // First make connections in the unit cell and the distinct fragments
  // Loop over the (0,0,0) unit cell atoms i
  Shift shift;
  for (int i = 0; i < _unitCellAtomList.size(); ++i) {
    _unitCellConnectionTable.append(Connection());
    const Atom &atom_i = _unitCellAtomList[i];

    // Loop over the (0,0,0) unit cell atoms j
    for (int j = 0; j < _unitCellAtomList.size(); ++j) {
      const Atom &atom_j = _unitCellAtomList[j];
      if (areCovalentBondedAtoms(atom_i, atom_j)) {
        _unitCellConnectionTable[i].insert(j, shift);
      }
    }
  }

  // Now make the connections in the neighboring cells
  for (int h1 = -1; h1 <= 1; h1++) {
    for (int h2 = -1; h2 <= 1; h2++) {
      for (int h3 = -1; h3 <= 1; h3++) {
        if (h1 == 0 && h2 == 0 && h3 == 0) {
          continue; // already done (0,0,0) cell
        }
        shift = {h1, h2, h3};

        for (int i = 0; i < _unitCellAtomList.size(); ++i) {
          const Atom &atom_i = _unitCellAtomList[i];

          for (int j = 0; j < _unitCellAtomList.size(); ++j) {
            Atom atom_j = generateAtomFromIndexAndShift(j, shift);
            if (areCovalentBondedAtoms(atom_i, atom_j)) {
              _unitCellConnectionTable[i].insert(j, shift);
            }
          }
        }
      }
    }
  }
}

void DeprecatedCrystal::makeVdwCellConnectionTable() {
  Q_ASSERT(_vdwCellConnectionTable.size() == 0);

  // First make connections in the unit cell and the distinct fragments
  // Loop over the (0,0,0) unit cell atoms i
  Shift shift{0, 0, 0};
  for (int i = 0; i < _unitCellAtomList.size(); ++i) {
    _vdwCellConnectionTable.append(Connection());
    const Atom &atom_i = _unitCellAtomList[i];

    // Loop over the (0,0,0) unit cell atoms j
    for (int j = 0; j < _unitCellAtomList.size(); ++j) {
      const Atom &atom_j = _unitCellAtomList[j];
      if (_unitCellConnectionTable[i].keys().contains(j)) {
        continue; // skip covalently bonded pairs
      }
      if (areVdwBondedAtoms(atom_i, atom_j)) {
        _vdwCellConnectionTable[i].insert(j, shift);
      }
    }
  }

  // Now make the connections in the neighboring cells
  for (int h1 = -1; h1 <= 1; h1++) {
    for (int h2 = -1; h2 <= 1; h2++) {
      for (int h3 = -1; h3 <= 1; h3++) {
        if (h1 == 0 && h2 == 0 && h3 == 0) {
          continue; // already done (0,0,0) cell
        }
        shift = {h1, h2, h3};

        for (int i = 0; i < _unitCellAtomList.size(); ++i) {
          const Atom &atom_i = _unitCellAtomList[i];

          for (int j = 0; j < _unitCellAtomList.size(); ++j) {
            const Atom &atom_j = generateAtomFromIndexAndShift(j, shift);
            if (_unitCellConnectionTable[i].keys().contains(j)) {
              continue; // skip covalently bonded pairs
            }
            if (areVdwBondedAtoms(atom_i, atom_j)) {
              _vdwCellConnectionTable[i].insert(j, shift);
            }
          }
        }
      }
    }
  }
}

bool DeprecatedCrystal::areVdwBondedAtoms(const Atom &atom_i,
                                          const Atom &atom_j,
                                          const float closeContactTolerance) {
  QVector3D diff = atom_i.pos() - atom_j.pos();
  if (fabs(diff.x()) > _vdwCutOff || fabs(diff.y()) > _vdwCutOff ||
      fabs(diff.z()) > _vdwCutOff) {
    return false;
  }

  float distance = diff.length();

  float sumOfVdwRadii = atom_i.vdwRadius() + atom_j.vdwRadius();

  if (distance < sumOfVdwRadii + closeContactTolerance + 0.2) {
    if (!atom_i.isDisordered() || !atom_j.isDisordered() ||
        atom_i.disorderGroup() ==
            atom_j.disorderGroup()) { // don't bond if in different disorder
                                      // groups
      return true;
    }
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Atoms, Bonding, Packing and Cluster Generation
//
///////////////////////////////////////////////////////////////////////////////////////////////////

QVector<Atom> DeprecatedCrystal::generateAtomsFromAtomIds(
    const QVector<AtomId> &atomIds) const {
  QVector<Atom> atoms;
  atoms.reserve(atomIds.size());
  for (const auto &id : atomIds) {
    Atom atom = generateAtomFromIndexAndShift(id.unitCellIndex, id.shift);
    atoms.push_back(atom);
  }
  return atoms;
}

void DeprecatedCrystal::expandAtomsBondedToAtom(const Atom &atom) {
  QVector<int> limits = shiftLimits(m_atoms);
  int sizeBefore = m_atoms.size();
  appendConnectionsToAtom(atom, _unitCellConnectionTable, m_atoms, limits);
  for (int i = sizeBefore; i < m_atoms.size(); ++i) {
    appendConnectionsToAtom(m_atoms[i], _unitCellConnectionTable, m_atoms,
                            limits);
  }
}

void DeprecatedCrystal::addAsymmetricAtomsToAtomList() {
  for (auto i = _asymmetricUnitIndicesAndShifts.constKeyValueBegin();
       i != _asymmetricUnitIndicesAndShifts.constKeyValueEnd(); i++) {
    Atom atom = generateAtomFromIndexAndShift(i->first, i->second);
    m_atoms.append(atom);
  }
}

void DeprecatedCrystal::resetToAsymmetricUnit() {
  clearAtomList();
  addAsymmetricAtomsToAtomList();
  updateAtomListInfo();
  resetOrigin();
  calculateRadius();
  emit atomsChanged();
}

void DeprecatedCrystal::expandAtomsWithinRadius(float radius) {
  setSelectStatusForAllAtoms(true);
  expandAtomsWithinRadiusOfSelectedAtoms(radius);
}

void DeprecatedCrystal::expandAtomsWithinRadiusOfSelectedAtoms(float radius) {
  // Suppress other emitted signals until this is done.
  {
    QSignalBlocker blocker(this);
    QVector<AtomId> selectedAtomIds =
        selectedAtomsAsIds(); // Used to reselect atoms after generation

    QVector<Atom> selectedAtoms = removeSelectedAtoms();
    clearAtomList();

    QVector<Shift> cellShifts = getCellShifts(selectedAtoms, radius);
    QVector<Atom> clusterAtoms = generateAtomsFromShifts(cellShifts);
    keepClusterAtomsWithinRadiusOfSelectedAtoms(clusterAtoms, selectedAtoms,
                                                radius);

    // didn't find any atoms within radius and should reinstate the selected
    // atoms so there is at least something on the screen
    if (m_atoms.size() < selectedAtoms.size()) {
      reinstateAtoms(selectedAtoms);
    }

    selectAtomsWithEquivalentAtomIds(selectedAtomIds);

    updateAtomListInfo();
    resetOrigin();
  }

  emit atomsChanged();
}

// Returns a list containing fractional limits for cell packing.
// The padding is assumed to be absolute and in Angstroms
QPair<QVector3D, QVector3D>
DeprecatedCrystal::fractionalPackingLimitsFromPadding(
    float paddingInAngstroms) {
  QVector<float> packingLimits;

  float paddingFractionA = paddingInAngstroms / _unitCell.a();
  float paddingFractionB = paddingInAngstroms / _unitCell.b();
  float paddingFractionC = paddingInAngstroms / _unitCell.c();

  /*
   packingLimits = "cubic-like region wider than unit cell" =

   [ -paddingFractionA , 1 + paddingFractionA ]
   x [ -paddingFractionB , 1 + paddingFractionB ]
   x [ -paddingFractionC , 1 + paddingFractionC ]

   */

  QVector3D lower(0.0 - paddingFractionA, 0.0 - paddingFractionB,
                  0.0 - paddingFractionC);
  QVector3D upper(1.0 + paddingFractionA, 1.0 + paddingFractionB,
                  1.0 + paddingFractionC);

  return qMakePair(lower, upper);
}

void DeprecatedCrystal::packUnitCells(const QVector3D &upperLimit) {
  packMultipleCells(QVector3D(0, 0, 0), upperLimit);
}

void DeprecatedCrystal::packMultipleCells(const QVector3D &lower,
                                          const QVector3D &upper) {
  {
    QSignalBlocker blocker(this);
    /* cancel contact atoms if on (toggle toolbar button too) */
    clearAtomList();
    setAtomListToBufferedUnitCellAtomList(lower, upper);
    updateAtomListInfo();
    resetOrigin();
    calculateRadius();
  }

  emit atomsChanged();
}

QVector<Atom> DeprecatedCrystal::packedUnitCellsAtomList(
    const QPair<QVector3D, QVector3D> &packingLimits, bool calcConnectivity) {
  QVector<Atom> atoms = bufferedUnitCellAtomList(packingLimits);
  if (calcConnectivity) {
    calculateConnectivityInfo(atoms);
  }
  return atoms;
}

void DeprecatedCrystal::appendAtom(const Atom &atom, const Shift &shift) {
  Atom newAtom = atom;
  newAtom.displace(shift, _unitCell.directCellMatrix());
  m_atoms.append(newAtom);
}

bool DeprecatedCrystal::isCompleteCellFromLimits(QVector<float> limits) {
  const float UPPERLIMIT = 1.0;

  return qFuzzyIsNull(limits[0]) && qFuzzyCompare(limits[1], UPPERLIMIT) &&
         qFuzzyIsNull(limits[2]) && qFuzzyCompare(limits[3], UPPERLIMIT) &&
         qFuzzyIsNull(limits[4]) && qFuzzyCompare(limits[5], UPPERLIMIT);
}

bool DeprecatedCrystal::atomLocatedInPartialCell(Atom *atom,
                                                 QVector<float> limits) {
  bool xtest = atom->fx() >= limits[0] && atom->fx() < limits[1];
  bool ytest = atom->fy() >= limits[2] && atom->fy() < limits[3];
  bool ztest = atom->fz() >= limits[4] && atom->fz() < limits[5];

  return xtest && ytest && ztest;
}

void DeprecatedCrystal::setAtomListToBufferedUnitCellAtomList(
    const QVector3D &lower, const QVector3D &upper) {
  QVector<Atom> expandedList =
      bufferedUnitCellAtomList(qMakePair(lower, upper));
  m_atoms.append(expandedList);
}

QVector<AtomId> DeprecatedCrystal::voidCluster(const float padding) {
  QVector<AtomId> cluster;

  QVector<int> atomsToSuppress = suppressedAtomsAsUnitCellAtomIndices();

  auto expandedList =
      bufferedUnitCellAtomList(fractionalPackingLimitsFromPadding(padding));
  for (const auto &atom : expandedList) {
    if (!atomsToSuppress.contains(atom.atomId().unitCellIndex)) {
      cluster.append(atom.atomId());
    }
  }
  return cluster;
}

void DeprecatedCrystal::updateForChangeInAtomConnectivity() {
  updateConnectivityInfo();
  emit atomsChanged();
}

bool DeprecatedCrystal::areCovalentBondedAtoms(const Atom &atom_i,
                                               const Atom &atom_j) {
  bool conventionallyBonded =
      areCovalentBondedAtomsByDistanceCriteria(atom_i, atom_j);

  if (doNotBond(atom_i, atom_j, conventionallyBonded)) {
    return false;
  }
  if (doBond(atom_i, atom_j, conventionallyBonded)) {
    return true;
  }
  return conventionallyBonded;
}

bool DeprecatedCrystal::areCovalentBondedAtomsByDistanceCriteria(
    const Atom &atom_i, const Atom &atom_j) {
  QVector3D diff = atom_i.pos() - atom_j.pos();
  if (fabs(diff.x()) > _covalentCutOff || fabs(diff.y()) > _covalentCutOff ||
      fabs(diff.z()) > _covalentCutOff) {
    return false;
  }

  float distance = diff.length();

  float sumOfCovRadii = atom_i.covRadius() + atom_j.covRadius();

  if (sumOfCovRadii - BONDING_TOLERANCE < distance &&
      distance < sumOfCovRadii + BONDING_TOLERANCE) {
    if (!atom_i.isDisordered() || !atom_j.isDisordered() ||
        atom_i.disorderGroup() ==
            atom_j.disorderGroup()) { // don't bond if in different disorder
                                      // groups
      return true;
    }
  }
  return false;
}

QVector<Atom> DeprecatedCrystal::bufferedUnitCellAtomList(
    const QPair<QVector3D, QVector3D> &packingLimits) {
  return bufferedAtomList(_unitCellAtomList, packingLimits, true);
}

QVector<Atom> DeprecatedCrystal::bufferedAtomList(
    const QVector<Atom> &atomList, const QPair<QVector3D, QVector3D> &boxLimits,
    bool withinBox, bool includeCellBoundaryAtoms) {
  /*
   atomList  = the list of atoms (usually the list of user selected atoms)
   boxLimits = box limits
   withinBox = keep atoms within the single box limits
   includeCellBoundaryAtoms = include atoms sitting right on the cell boundary
   */

  QVector<Atom> expandedCellAtomList;

  // To avoid duplication of atom positions, keep track of positions
  QVector<QVector3D> positions;

  // Set zero to the unit cell shift of atom 0 in atomList
  Shift atom0UnitCellShift = atomList[0].unitCellShift();
  QVector3D zero = QVector3D(atom0UnitCellShift.h, atom0UnitCellShift.k,
                             atom0UnitCellShift.l);

  // Find the unit cell shifts for all the atoms in atomList
  QVector<Shift> unitCellShifts;
  for (const Atom &atom : atomList) {
    Shift unitCellShift = atom.unitCellShift();
    if (!isZeroShift(unitCellShift) &&
        !shiftListContainsShift(unitCellShifts, unitCellShift)) {
      unitCellShifts.append(unitCellShift);
    }
  }

  // Expand cellShifts to include all shifts
  QVector<Shift> multiCellShifts;
  auto lambda = [&](int h, int k, int l) {
    multiCellShifts.append({h, k, l});
    for (const Shift &unitCellShift : unitCellShifts) {
      Shift multiCellShift{h + unitCellShift.h, k + unitCellShift.k,
                           l + unitCellShift.l};
      if (!shiftListContainsShift(multiCellShifts, multiCellShift)) {
        multiCellShifts.append(multiCellShift);
      }
    }
  };

  iterate_over_cell_limits(lambda, boxLimits);

  for (const auto &shift : multiCellShifts) {
    for (const Atom &atom : _unitCellAtomList) {
      QVector3D pos = QVector3D(atom.fx() + shift.h, atom.fy() + shift.k,
                                atom.fz() + shift.l);
      if (!positions.contains(pos)) {
        if (withinBox && !positionIsWithinBoxCentredAtZero(
                             pos, boxLimits, zero, includeCellBoundaryAtoms)) {
          continue;
        }
        Atom newAtom = atom;
        newAtom.displace(shift, _unitCell.directCellMatrix());
        expandedCellAtomList.append(newAtom);
      }
    }
  }
  return expandedCellAtomList;
}

bool DeprecatedCrystal::positionIsWithinBoxCentredAtZero(
    QVector3D pos, const QPair<QVector3D, QVector3D> &boxLimits, QVector3D zero,
    bool includeBoxBoundaryPositions) {
  float expansion = 0.0f;
  if (includeBoxBoundaryPositions) {
    expansion = 0.000001f;
  }

  const auto &lower = boxLimits.first;
  const auto &upper = boxLimits.second;
  return (zero.x() + lower[0] - expansion < pos.x() &&
          pos.x() < zero.x() + upper[0] + expansion &&
          zero.y() + lower[1] - expansion < pos.y() &&
          pos.y() < zero.y() + upper[1] + expansion &&
          zero.z() + lower[2] - expansion < pos.z() &&
          pos.z() < zero.z() + upper[2] + expansion);
}

bool DeprecatedCrystal::anyAtomHasAdp() const {
  for (const Atom &atom : m_atoms) {
    if (atom.hasAdp()) {
      return true;
    }
  }
  return false;
}

bool DeprecatedCrystal::hasCovalentBondedAtoms(int i, int j) {
  // See calculateCovalentBondInfo
  Q_ASSERT(m_bondedAtomsForAtom.size() > 0);
  return std::find(m_bondedAtomsForAtom[i].begin(),
                   m_bondedAtomsForAtom[i].end(),
                   j) != m_bondedAtomsForAtom[i].end();
}

Atom DeprecatedCrystal::generateAtomFromIndexAndShift(
    int index, const Shift &shift) const {
  Atom atom = _unitCellAtomList[index];
  atom.displace(shift, _unitCell.directCellMatrix());
  return atom;
}

int DeprecatedCrystal::numberOfCovalentBondedAtomsBetweenAtoms(int i, int j) {
  auto i_atoms = m_bondedAtomsForAtom[i];
  auto j_atoms = m_bondedAtomsForAtom[j];

  int n = 0;
  for (const auto &j : j_atoms) {
    if (std::find(i_atoms.begin(), i_atoms.end(), j) != i_atoms.end()) {
      n++;
    }
  }
  return n;
}

void DeprecatedCrystal::clearAtomList() { m_atoms.clear(); }

void DeprecatedCrystal::removeLastAtoms(int n) {
  int newAtomCount = std::max(static_cast<int>(m_atoms.size()) - n, 0);
  m_atoms.resize(newAtomCount);
  updateAtomListInfo();
}

void DeprecatedCrystal::appendUniqueAtomsOnly(const QVector<Atom> &atoms) {
  for (const Atom &atom : atoms) {
    AtomId atomId = atom.atomId();
    if (std::find_if(m_atoms.begin(), m_atoms.end(), [&atomId](const Atom &a) {
          return a.atomId() == atomId;
        }) == m_atoms.end()) {
      m_atoms.append(atom);
    }
  }
}

bool DeprecatedCrystal::hasAllAtomsBondedToAtom(const Atom &atom) {
  int u = atom.unitCellAtomIndex();
  Shift ushift = atom.unitCellShift();

  // For each connection in "connections", get the unit cell
  // atom "c" and its shift "cshift" and append it to the
  // "map" only if it is not already in there
  const auto &conn = _unitCellConnectionTable[u];
  for (auto kv = conn.constKeyValueBegin();
        kv != conn.constKeyValueEnd(); kv++) {
    int c = kv->first;
    Shift cshift = kv->second;

    // Add the existing "ushift" for atom "u"
    cshift.h += ushift.h;
    cshift.k += ushift.k;
    cshift.l += ushift.l;
    AtomId atomId{c, cshift};
    if (std::find_if(m_atoms.begin(), m_atoms.end(), [&atomId](const Atom &a) {
          return a.atomId() == atomId;
        }) == m_atoms.end()) {
      return false; // has a connection to a atom not in _atomList
    }
  }
  return true;
}

/// Same as 'appendVdwContactAtoms' except it checks that the shift of any new
/// atoms are within plus 1 unit cell of the
/// the set of shiftLimits passed as the 4th argument to this function. If the
/// atoms are outside
/// +1 unit cell then they are ignored. This allows appendConnectionsToAtom to
/// be used on network
/// structures that would otherwise expand indefinitely
void DeprecatedCrystal::appendConnectionsToAtom(const Atom &atom,
                                                ConnectionTable connections,
                                                QVector<Atom> &atomList,
                                                QVector<int> shiftLimits,
                                                bool addContactAtoms) {
  int u = atom.unitCellAtomIndex();
  Shift ushift = atom.unitCellShift();

  const auto &conns = connections[u]; 

  for (auto it = conns.constKeyValueBegin(); it != conns.constKeyValueEnd(); ++it) {
    int c = it->first; 
    Shift cshift = it->second; 

    // Add the existing "ushift" for atom "u"
    cshift.h += ushift.h;
    cshift.k += ushift.k;
    cshift.l += ushift.l;

    if ((shiftLimits.size() == 0) || 
        shiftWithinPlusOneOfLimits(cshift, shiftLimits)) {
      AtomId atomId{c, cshift};
      if (std::find_if(atomList.begin(), atomList.end(),
                       [&atomId](const Atom &a) {
                         return a.atomId() == atomId;
                       }) == atomList.end()) {
        Atom newAtom = generateAtomFromIndexAndShift(c, cshift);
        newAtom.setContactAtom(addContactAtoms);
        atomList.append(newAtom);
      }
    }
  }
}

bool DeprecatedCrystal::hasAtom(const Atom &atomToFind) {
  return std::find_if(m_atoms.begin(), m_atoms.end(),
                      [&atomToFind](const Atom &atom) {
                        return atom.isSameAtom(atomToFind);
                      }) != m_atoms.end();
}

QVector<int> DeprecatedCrystal::shiftLimits(const QVector<Atom> &atomList) {
  QVector<int> limits{0, 0, 0, 0, 0, 0};

  for (const Atom &atom : atomList) {
    Shift s = atom.unitCellShift();
    limits[0] = qMin(limits[0], s.h);
    limits[1] = qMin(limits[1], s.k);
    limits[2] = qMin(limits[2], s.l);

    limits[3] = qMax(limits[3], s.h);
    limits[4] = qMax(limits[4], s.k);
    limits[5] = qMax(limits[5], s.l);
  }

  return limits;
}

QVector<int>
DeprecatedCrystal::shiftLimitsForFragmentContainingAtom(const Atom &atom) {
  QVector<Atom> atomList;

  int atomIndex = atom.unitCellAtomIndex();
  QVector<int> fragmentAtomIndices =
      m_atomsForFragment[_fragmentForAtom[atomIndex]];
  foreach (int atomIndex, fragmentAtomIndices) {
    atomList.append(m_atoms[atomIndex]);
  }

  return shiftLimits(atomList);
}

bool DeprecatedCrystal::shiftWithinPlusOneOfLimits(
    const Shift &shift, const QVector<int> &shiftLimits) const {
  return shift.h >= shiftLimits[0] - 1 && shift.h <= shiftLimits[3] + 1 &&
         shift.k >= shiftLimits[1] - 1 && shift.k <= shiftLimits[4] + 1 &&
         shift.l >= shiftLimits[2] - 1 && shift.l <= shiftLimits[5] + 1;
}

bool DeprecatedCrystal::isZeroShift(const Shift &shift) const {
  return shift == Shift{0, 0, 0};
}

bool DeprecatedCrystal::shiftListContainsShift(const QVector<Shift> &shiftList,
                                               const Shift &shift) const {
  for (const auto &s : shiftList) {
    if (s == shift)
      return true;
  }
  return false;
}

QVector<Atom> DeprecatedCrystal::removeSelectedAtoms() {
  auto partition_point =
      std::partition(m_atoms.begin(), m_atoms.end(),
                     [](const Atom &a) { return !a.isSelected(); });
  QVector<Atom> selectedAtoms(partition_point, m_atoms.end());
  m_atoms.erase(partition_point, m_atoms.end());
  return selectedAtoms;
}

QVector<Atom>
DeprecatedCrystal::generateAtomsFromShifts(QVector<Shift> shifts) {
  QVector<Atom> clusterAtoms;

  foreach (Shift shift, shifts) {
    for (const Atom &unitCellAtom : _unitCellAtomList) {
      Atom atom = unitCellAtom;
      atom.displace(shift, _unitCell.directCellMatrix());
      clusterAtoms.append(atom);
    }
  }
  return clusterAtoms;
}

void DeprecatedCrystal::keepClusterAtomsWithinRadiusOfSelectedAtoms(
    const QVector<Atom> &clusterAtoms, const QVector<Atom> &atoms,
    float radius) {
  float radius2 = radius * radius;

  for (const auto &clusterAtom : clusterAtoms) {
    for (const auto &atom : atoms) {
      float dx = atom.x() - clusterAtom.x();
      float dy = atom.y() - clusterAtom.y();
      float dz = atom.z() - clusterAtom.z();

      float d2 = dx * dx + dy * dy + dz * dz;
      if (d2 < radius2) {
        m_atoms.append(clusterAtom);
        break;
      }
    }
  }
}

void DeprecatedCrystal::reinstateAtoms(const QVector<Atom> &atoms) {
  for (const auto &atom : atoms) {
    if (!hasAtom(atom))
      m_atoms.append(atom);
  }
}

int DeprecatedCrystal::unitCellAtomIndexOf(AtomId atomId, SymopId symopId,
                                           Vector3q shift) {
  Atom atom = generateAtomFromIndexAndShift(atomId.unitCellIndex, atomId.shift);

  atom.applySymopAlt(spaceGroup(), _unitCell.directCellMatrix(), symopId, 0,
                     shift);

  atom.shiftToUnitCell(_unitCell.directCellMatrix());
  auto location =
      std::find_if(_unitCellAtomList.begin(), _unitCellAtomList.end(),
                   [&atom](const Atom &a) { return a.atSamePosition(atom); });
  if (location == _unitCellAtomList.end())
    return -1;
  return location->atomId().unitCellIndex;
}

void DeprecatedCrystal::resetAllAtomColors() {
  for (auto &atom : m_atoms) {
    atom.clearCustomColor();
  }
  emit atomsChanged();
}

bool DeprecatedCrystal::hasAtomsWithCustomColor() const {
  return std::find_if(m_atoms.begin(), m_atoms.end(), [](const Atom &a) {
           return a.hasCustomColor();
         }) != m_atoms.end();
}

void DeprecatedCrystal::colorAllAtoms(QColor color) {
  Q_ASSERT(color.isValid());
  for (auto &atom : m_atoms) {
    atom.setCustomColor(color);
  }
}

void DeprecatedCrystal::colorAtomsByFragment(QVector<int> atoms) {
  for (int atomIndex : atoms) {
    m_atoms[atomIndex].setCustomColor(
        fragmentColor(_fragmentForAtom[atomIndex]));
  }
}

void DeprecatedCrystal::colorSelectedAtoms(QColor color) {
  Q_ASSERT(color.isValid());
  for (auto &atom : m_atoms) {
    if (atom.isSelected())
      atom.setCustomColor(color);
  }
  emit atomsChanged();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Fragments
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void DeprecatedCrystal::completeFragmentContainingAtomIndex(int atomIndex) {
  {
    QSignalBlocker blocker(this);
    bool usingContactAtoms = hasAnyVdwContactAtoms();

    bool wasContactAtom = false;
    if (m_atoms[atomIndex].isContactAtom()) {
      m_atoms[atomIndex].setContactAtom(false);
      wasContactAtom = true;
    }

    // Make use of atomIndex BEFORE removing contacts atoms
    // because if atom was contact atom then its index is likely to change
    const Atom &atom = m_atoms[atomIndex];
    if (usingContactAtoms) {
      removeVdwContactAtoms();
    }

    // contact atoms don't have fragment index AND
    // even if they did the fragment would only contain
    // themselves.
    if (wasContactAtom) {
      expandAtomsBondedToAtom(atom);
    } else {
      int fragment = _fragmentForAtom[atomIndex];
      for (int a : m_atomsForFragment[fragment]) {
        expandAtomsBondedToAtom(m_atoms[a]);
      }
    }

    if (usingContactAtoms) {
      appendVdwContactAtoms();
    }
    updateAtomListInfo();
  }

  emit atomsChanged();
}

void DeprecatedCrystal::deleteFragmentContainingAtomIndex(int atomIndex) {
  {
    QSignalBlocker blocker(this);
    bool usingContactAtoms = hasAnyVdwContactAtoms();
    if (usingContactAtoms) {
      removeVdwContactAtoms();
    }

    QVector<int> fragmentAtomIndices =
        m_atomsForFragment[_fragmentForAtom[atomIndex]];
    for (int atomIndex : fragmentAtomIndices) {
      m_atoms[atomIndex].setContactAtom(true);
    }
    removeVdwContactAtoms();

    if (usingContactAtoms) {
      appendVdwContactAtoms();
    }
  }

  emit atomsChanged();
}

void DeprecatedCrystal::discardIncompleteFragments() {
  auto it =
      std::remove_if(m_atoms.begin(), m_atoms.end(), [&](const Atom &atom) {
        return !hasAllAtomsBondedToAtom(atom);
      });
  m_atoms.erase(it, m_atoms.end());
  updateAtomListInfo();
  emit atomsChanged();
}

void DeprecatedCrystal::completeAllFragments() {
  {
    QSignalBlocker blocker(this);
    bool usingContactAtoms = hasAnyVdwContactAtoms();

    QVector<int> limits = shiftLimits(m_atoms);
    // the size of m_atoms changes throughout this loop!
    // this should be refactored to a depth-first search or similar
    for (int i = 0; i < m_atoms.size(); i++) {
      m_atoms[i].setContactAtom(false);
      appendConnectionsToAtom(m_atoms[i], _unitCellConnectionTable, m_atoms,
                              limits);
    }
    if (usingContactAtoms) {
      appendVdwContactAtoms();
    }
    updateAtomListInfo();
  }

  emit atomsChanged();
}

void DeprecatedCrystal::completeSelectedFragments() {
  {
    QSignalBlocker blocker(this);
    bool usingContactAtoms = hasAnyVdwContactAtoms();

    QVector<int> limits = shiftLimits(m_atoms);

    int originalSize = m_atoms.size();

    // Expands all atoms in fragments with a least one selected atom
    QVector<int> atomsToExpand;
    foreach (int fragmentIndex, fragmentIndicesOfSelection()) {
      atomsToExpand.append(m_atomsForFragment[fragmentIndex]);
    }
    foreach (int atomIndex, atomsToExpand) {
      appendConnectionsToAtom(m_atoms[atomIndex], _unitCellConnectionTable,
                              m_atoms, limits);
    }

    // Expand all the newly added atoms
    for (int i = originalSize; i < m_atoms.size(); ++i) {
      appendConnectionsToAtom(m_atoms[i], _unitCellConnectionTable, m_atoms,
                              limits);
    }

    if (usingContactAtoms) {
      appendVdwContactAtoms();
    }
    updateAtomListInfo();
  }
  emit atomsChanged();
}

void DeprecatedCrystal::toggleFragmentColors() {
  const bool skipFragment0 = true;

  if (skipFragment0 && numberOfFragments() == 1) {
    return;
  }

  for (int i = 0; i < m_atoms.size(); ++i) {
    if (skipFragment0 && _fragmentForAtom[i] == 0) {
      continue;
    }

    Atom &atom = m_atoms[i];

    if (atom.hasCustomColor()) {
      atom.clearCustomColor();
    } else {

      atom.setCustomColor(fragmentColor(_fragmentForAtom[i], skipFragment0));
    }
  }
  emit atomsChanged();
}

bool DeprecatedCrystal::hasIncompleteFragments() {
  for (int i = 0; i < m_atoms.size(); ++i) {
    if (!hasAllAtomsBondedToAtom(m_atoms[i])) {
      return true;
    }
  }
  return false;
}

bool DeprecatedCrystal::hasIncompleteSelectedFragments() {
  for (int i = 0; i < m_atoms.size(); ++i) {
    if (m_atoms[i].isSelected() && (!hasAllAtomsBondedToAtom(m_atoms[i]))) {
      return true;
    }
  }
  return false;
}

QVector3D DeprecatedCrystal::centroidOfFragment(int fragmentIndex) {
  int natoms = m_atomsForFragment[fragmentIndex].size();
  Q_ASSERT(natoms > 0); // prevent division by zero

  QVector3D centroid;
  foreach (int atomIndex, m_atomsForFragment[fragmentIndex]) {
    centroid = centroid + m_atoms[atomIndex].pos();
  }

  return centroid / natoms;
}

QVector3D DeprecatedCrystal::centroidOfAtomIds(QVector<AtomId> atomIds) {
  int natoms = atomIds.size();
  Q_ASSERT(natoms > 0); // prevent division by zero

  QVector3D centroid;

  foreach (AtomId atomId, atomIds) {
    Atom atom =
        generateAtomFromIndexAndShift(atomId.unitCellIndex, atomId.shift);
    centroid = centroid + atom.pos();
  }

  return centroid / natoms;
}

QVector3D DeprecatedCrystal::centerOfMassOfAtomIds(QVector<AtomId> atomIds) {
  int natoms = atomIds.size();
  Q_ASSERT(natoms > 0); // prevent division by zero

  float totalMass = 0.0;
  QVector3D com;

  foreach (AtomId atomId, atomIds) {
    Atom atom =
        generateAtomFromIndexAndShift(atomId.unitCellIndex, atomId.shift);
    float mass = atom.element()->mass();
    totalMass += mass;
    com = com + mass * atom.pos();
  }

  return com / totalMass;
}

QVector<QVector3D> DeprecatedCrystal::centroidsOfFragments() {
  QVector<QVector3D> centroids;
  centroids.reserve(m_atomsForFragment.size());

  for (int i = 0; i < m_atomsForFragment.size(); i++) {
    centroids.append(centroidOfFragment(i));
  }
  return centroids;
}

int DeprecatedCrystal::numberOfFragments() const {
  return m_atomsForFragment.size();
}

bool DeprecatedCrystal::fragmentIsComplete(int fragIndex) {
  const QVector<int> &atomIndices = m_atomsForFragment[fragIndex];
  return std::all_of(atomIndices.begin(), atomIndices.end(),
                     [&](int atomIndex) {
                       return hasAllAtomsBondedToAtom(m_atoms[atomIndex]);
                     });
}

QVector<int> DeprecatedCrystal::fragmentIndicesOfCompleteFragments() {
  QVector<int> completeFragments;
  for (int i = 0; i < m_atomsForFragment.size(); i++) {
    if (fragmentIsComplete(i)) {
      completeFragments.push_back(i);
    }
  }
  return completeFragments;
}

int DeprecatedCrystal::numberOfCompleteFragments() {
  return fragmentIndicesOfCompleteFragments().size();
}

QVector<AtomId>
DeprecatedCrystal::atomIdsForFragment(int fragmentIndex,
                                      bool skipContactAtoms) const {
  QVector<AtomId> result;
  const auto &fragAtoms = atomsForFragment(fragmentIndex, skipContactAtoms);
  for (const auto &atom : fragAtoms) {
    result.append(atom.atomId());
  }
  return result;
}

QVector<QVector<AtomId>>
DeprecatedCrystal::atomIdsForFragments(const QVector<int> &fragmentIndices,
                                       bool skipContactAtoms) const {
  QVector<QVector<AtomId>> result;
  for (const auto &fragmentIndex : fragmentIndices) {
    result.append(atomIdsForFragment(fragmentIndex, skipContactAtoms));
  }
  return result;
}

bool DeprecatedCrystal::fragmentAtomsAreSymmetryRelated(
    const QVector<AtomId> &fragAtoms1,
    const QVector<AtomId> &fragAtoms2) const {
  CrystalSymops crystalSymops = calculateCrystalSymops(fragAtoms1, fragAtoms2);
  return crystalSymops.size() > 0;
}

bool DeprecatedCrystal::fragmentsAreSymmetryRelated(int frag1, int frag2) {
  return fragmentAtomsAreSymmetryRelated(atomIdsForFragment(frag1),
                                         atomIdsForFragment(frag2));
}

int DeprecatedCrystal::fragmentIndexOfFirstSelectedFragment() {
  return fragmentIndexOfSelectedFragmentAtOrdinal(0);
}

int DeprecatedCrystal::fragmentIndexOfSecondSelectedFragment() {
  return fragmentIndexOfSelectedFragmentAtOrdinal(1);
}

void DeprecatedCrystal::colorFragmentsByEnergyPair() {
  const int keyFrag = keyFragment();

  QMap<int, QColor> colors;

  for (int fragIndex = 0; fragIndex < m_atomsForFragment.size(); fragIndex++) {
    if (fragIndex == keyFrag) {
      continue;
    }

    colors[fragIndex] = energyColorForPair(keyFrag, fragIndex);
  }

  for (int i = 0; i < m_atoms.size(); ++i) {
    if (_fragmentForAtom[i] == keyFrag) {
      continue;
    }
    Atom &atom = m_atoms[i];
    atom.setCustomColor(colors[_fragmentForAtom[i]]);
  }
}

void DeprecatedCrystal::clearFragmentColors() {
  for (int i = 0; i < m_atoms.size(); ++i) {
    Atom &atom = m_atoms[i];

    if (atom.hasCustomColor()) {
      atom.clearCustomColor();
    }
  }
}

// Return the "key" fragment index
// The criteria for choosing the key fragment is
// (i) If there's any selected atoms choose the first fragment with a selected
// atom
// (ii) Failing that choose the first fragment (fragment index 0).
int DeprecatedCrystal::keyFragment() {
  Q_ASSERT(m_atoms.size() > 0);

  int keyFragment;

  if (hasSelectedAtoms()) {
    keyFragment = firstFragmentWithSelectedAtom();
  } else {
    keyFragment = 0;
  }

  return keyFragment;
}

QVector<Atom> DeprecatedCrystal::atomsForFragment(int fragmentIndex,
                                                  bool skipContactAtoms) const {
  Q_ASSERT(fragmentIndex >= 0);
  Q_ASSERT(fragmentIndex < m_atomsForFragment.size());

  QVector<Atom> result;

  foreach (int atomIndex, m_atomsForFragment[fragmentIndex]) {
    if (skipContactAtoms && m_atoms[atomIndex].isContactAtom()) {
      continue;
    }
    result.append(m_atoms[atomIndex]);
  }
  return result;
}

int DeprecatedCrystal::generateFragmentFromAtomIdAssociatedWithASurface(
    Surface *sourceSurface, const AtomId atomId) {
  int atomIndex = -1;
  {
    QSignalBlocker blocker(this);
    Atom atom =
        generateAtomFromIndexAndShift(atomId.unitCellIndex, atomId.shift);

    QVector<float> relativeShift = sourceSurface->relativeShift();
    Vector3q shift;
    for (int i = 0; i < 3; ++i) {
      shift[i] = relativeShift[i];
    }

    int unitCellIndex =
        unitCellAtomIndexOf(atomId, sourceSurface->symopId(), shift);
    atom.applySymopAlt(spaceGroup(), _unitCell.directCellMatrix(),
                       sourceSurface->symopId(), unitCellIndex, shift);
    auto loc =
        std::find_if(m_atoms.begin(), m_atoms.end(),
                     [&atom](const Atom &a) { return a.atSamePosition(atom); });
    if (loc == m_atoms.end()) {
      m_atoms.append(atom);
      atomIndex = m_atoms.size() - 1;
      updateAtomListInfo();
    } else {
      atomIndex = std::distance(m_atoms.begin(), loc);
    }

    completeFragmentContainingAtomIndex(atomIndex);
    updateAtomListInfo();
  }

  emit atomsChanged();
  return atomIndex;
}

QColor DeprecatedCrystal::fragmentColor(int fragmentIndex, bool skipFragment0) {
  int minFrag = skipFragment0 ? 1 : 0;
  int maxFrag = skipFragment0 ? numberOfFragments() - 1 : numberOfFragments();
  ColorScheme colorScheme = colorSchemeFromString(
      settings::readSetting(settings::keys::ENERGY_COLOR_SCHEME).toString());
  return ColorSchemer::color(colorScheme, fragmentIndex, minFrag, maxFrag,
                             false);
}

int DeprecatedCrystal::firstFragmentWithSelectedAtom() {
  Q_ASSERT(hasSelectedAtoms());
  return _fragmentForAtom[selectedAtomIndices()[0]];
}

QVector<int> DeprecatedCrystal::fragmentIndicesOfSelection() {
  QSet<int> indices;
  foreach (int atomIndex, selectedAtomIndices()) {
    indices << _fragmentForAtom[atomIndex];
  }
  return indices.values().toVector();
}

bool DeprecatedCrystal::fragmentWithIndexExists(int fragmentIndex) const {
  return (fragmentIndex > 0) && (fragmentIndex < m_atomsForFragment.size());
}

// returns the index into the list of all fragments of the n-th selected
// fragment
// returns -1 if there isn't an n-th selected fragment
int DeprecatedCrystal::fragmentIndexOfSelectedFragmentAtOrdinal(int n) {
  int selectedFragmentIndex = -1;

  int numSelectedFragments = 0;
  for (int fragIndex = 0; fragIndex < numberOfFragments(); fragIndex++) {
    if (fragmentIsSelected(fragIndex)) {
      if (n == numSelectedFragments) {
        selectedFragmentIndex = fragIndex;
        break;
      }
      numSelectedFragments++;
    }
  }
  return selectedFragmentIndex;
}

bool DeprecatedCrystal::hasSameFragmentAtoms(int i, int j) {
  Q_ASSERT(_fragmentForAtom.size() != 0);
  return _fragmentForAtom[i] == _fragmentForAtom[j];
}

bool DeprecatedCrystal::moreThanOneSymmetryUniqueFragment() {
  // Save current cluster
  QVector<AtomId> selectedAtomIds = selectedAtomsAsIds();
  QVector<Atom> savedAtoms = m_atoms;

  m_atoms.clear();
  addAsymmetricAtomsToAtomList();
  updateAtomListInfo();
  bool result = numberOfFragments() > 1;

  m_atoms = savedAtoms;
  selectAtomsWithEquivalentAtomIds(selectedAtomIds);
  updateAtomListInfo();

  return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Bonding Modification
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void DeprecatedCrystal::bondSelectedAtoms() {
  addUniqueAtomPairsToBondingList(selectedAtomIndices());
  updateConnectivityInfo();
  emit atomsChanged();
}

void DeprecatedCrystal::unbondSelectedAtoms() {
  addUniqueAtomPairsToNonBondingList(selectedAtomIndices());
  updateConnectivityInfo();
  emit atomsChanged();
}

void DeprecatedCrystal::resetBondingModifications() {
  m_doBondList.clear();
  m_doNotBondList.clear();
}

bool DeprecatedCrystal::doNotBond(const Atom &atom_i, const Atom &atom_j,
                                  bool conventionallyBonded) {
  // Do not add pairs to doNotBondList if they never be bonded by conventional
  // bonding criteria
  if (!conventionallyBonded) {
    return false;
  }

  int iIndex = atom_i.atomId().unitCellIndex;
  int jIndex = atom_j.atomId().unitCellIndex;

  if (pairInList(iIndex, jIndex, m_doNotBondList) ||
      pairInList(jIndex, iIndex, m_doNotBondList)) {
    return true;
  }
  return false;
}

bool DeprecatedCrystal::doBond(const Atom &atom_i, const Atom &atom_j,
                               bool conventionallyBonded) {
  // Do not add pairs to doBondList if they would otherwise be bonded by
  // conventional bonding criteria
  if (conventionallyBonded) {
    return false;
  }

  int iIndex = atom_i.atomId().unitCellIndex;
  int jIndex = atom_j.atomId().unitCellIndex;

  if (pairInList(iIndex, jIndex, m_doBondList) ||
      pairInList(jIndex, iIndex, m_doBondList)) {
    return true;
  }
  return false;
}

bool DeprecatedCrystal::removeFromBondingList(int iIndex, int jIndex) {
  return m_doBondList.removeAll(qMakePair(iIndex, jIndex)) > 0;
}

bool DeprecatedCrystal::removeFromNonBondingList(int iIndex, int jIndex) {
  return m_doNotBondList.removeAll(qMakePair(iIndex, jIndex)) > 0;
}

void DeprecatedCrystal::addUniqueAtomPairsToBondingList(
    const QVector<int> &atomIds) {
  for (int i : atomIds) {
    for (int j : atomIds) {

      bool conventionallyBonded =
          areCovalentBondedAtomsByDistanceCriteria(m_atoms[i], m_atoms[j]);

      int iIndex = m_atoms[i].atomId().unitCellIndex;
      int jIndex = m_atoms[j].atomId().unitCellIndex;

      // Two cases need to be handled
      // 1. Bond would be added by conventional distance criteria
      // therefore only reason to ask for bond is that it's been suppressed by
      // the user
      // and need to be removed from the _doNotBondList
      // 2. Bond would not be added by conventional distance criteria
      // therefore add it to the _doBondList
      if (conventionallyBonded) { // 1.
        removeFromNonBondingList(iIndex, jIndex);
        removeFromNonBondingList(jIndex, iIndex);
      } else { // 2.
        m_doBondList.append(qMakePair(iIndex, jIndex));
      }
    }
  }
}

void DeprecatedCrystal::addUniqueAtomPairsToNonBondingList(
    const QVector<int> &atomIds) {
  for (int i : atomIds) {
    for (int j : atomIds) {

      bool conventionallyBonded =
          areCovalentBondedAtomsByDistanceCriteria(m_atoms[i], m_atoms[j]);

      int iIndex = m_atoms[i].atomId().unitCellIndex;
      int jIndex = m_atoms[j].atomId().unitCellIndex;

      // Two cases need to be handled
      // 1. Bond would be added by conventional distance criteria
      // therefore add it to the _doNotBondList to suppress it
      // 2. Bond would not be added by conventional distance criteria
      // therefore only reason to try to unbond it is if it's been added by the
      // user
      // and needs to be removed from the _doBondList
      if (conventionallyBonded) { // 1.
        m_doNotBondList.append(qMakePair(iIndex, jIndex));
      } else { // 2.
        removeFromBondingList(iIndex, jIndex);
        removeFromBondingList(jIndex, iIndex);
      }
    }
  }
}

bool DeprecatedCrystal::pairInList(
    int atom_i_index, int atom_j_index,
    const QVector<QPair<int, int>> &listOfPairs) {
  typedef QPair<int, int> pair;
  foreach (pair atomIndices, listOfPairs) {
    int symopId1 = symopIdForUnitCellAtoms(atom_i_index, atomIndices.first);
    int symopId2 = symopIdForUnitCellAtoms(atom_j_index, atomIndices.second);
    if (symopId1 != NOSYMOP && symopId2 != NOSYMOP && symopId1 == symopId2) {
      return true;
    }
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Disorder
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool DeprecatedCrystal::isDisordered() { return _disorderGroups.size() > 0; }

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Visibility
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool DeprecatedCrystal::hasVisibleAtoms() const {
  return std::any_of(m_atoms.begin(), m_atoms.end(),
                     [](const Atom &a) { return a.isVisible(); });
}

bool DeprecatedCrystal::hasHiddenAtoms() const {
  return std::any_of(m_atoms.begin(), m_atoms.end(),
                     [](const Atom &a) { return !a.isVisible(); });
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Selections
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void DeprecatedCrystal::setSelectStatusForAllAtoms(bool selected) {
  for (auto &atom : m_atoms) {
    if (!atom.isContactAtom())
      atom.setSelected(selected);
  }
  emit atomsChanged();
}

void DeprecatedCrystal::setSelectStatusForFragment(int fragment,
                                                   bool selected) {
  Q_ASSERT(fragment >= 0 && fragment < m_atomsForFragment.size());
  const auto &fragAtoms = m_atomsForFragment[fragment];
  for (int atomIndex : fragAtoms) {
    if (!m_atoms[atomIndex].isContactAtom() && m_atoms[atomIndex].isVisible()) {
      m_atoms[atomIndex].setSelected(selected);
    }
  }
  emit atomsChanged();
}

QVector<int> DeprecatedCrystal::selectedAtomIndices() {
  QVector<int> selectedAtoms;

  for (int i = 0; i < m_atoms.size(); ++i) {
    if (m_atoms[i].isSelected()) {
      selectedAtoms.append(i);
    }
  }

  return selectedAtoms;
}

QVector<AtomId> DeprecatedCrystal::selectedAtomsAsIds() {
  QVector<AtomId> selectedAtoms;

  for (int i = 0; i < m_atoms.size(); ++i) {
    if (m_atoms[i].isSelected()) {
      selectedAtoms.append(m_atoms[i].atomId());
    }
  }

  return selectedAtoms;
}

QVector<AtomId> DeprecatedCrystal::selectedAtomsAsIdsOrderedByFragment() {
  QVector<AtomId> result;

  for (int i = 0; i < m_atomsForFragment.size(); i++) {
    if (fragmentIsSelected(i)) {
      for (const auto &atomIndex : m_atomsForFragment[i]) {
        result.append(m_atoms[atomIndex].atomId());
      }
    }
  }
  return result;
}

bool DeprecatedCrystal::hasSelectedAtoms() const {
  return std::any_of(m_atoms.begin(), m_atoms.end(),
                     [](const Atom &a) { return a.isSelected(); });
}

bool DeprecatedCrystal::hasAllAtomsSelected() {
  return std::all_of(m_atoms.begin(), m_atoms.end(),
                     [](const Atom &a) { return a.isSelected(); });
}

bool DeprecatedCrystal::fragmentContainingAtomIndexIsSelected(
    int refAtomIndex) {
  return fragmentIsSelected(_fragmentForAtom[refAtomIndex]);
}

bool DeprecatedCrystal::fragmentIsSelected(int fragmentIndex) {
  const auto &fragAtoms = m_atomsForFragment[fragmentIndex];
  return std::all_of(fragAtoms.begin(), fragAtoms.end(),
                     [&](int i) { return m_atoms[i].isSelected(); });
}

void DeprecatedCrystal::discardSelectedAtoms() {
  auto it = std::remove_if(m_atoms.begin(), m_atoms.end(),
                           [](const Atom &a) { return a.isSelected(); });
  m_atoms.erase(it, m_atoms.end());
  updateAtomListInfo();
  emit atomsChanged();
}

void DeprecatedCrystal::selectAllAtoms() {
  for (auto &atom : m_atoms)
    atom.setSelected(true);
  emit atomsChanged();
}

void DeprecatedCrystal::unselectAllAtoms() {
  for (auto &atom : m_atoms)
    atom.setSelected(false);
  emit atomsChanged();
}

void DeprecatedCrystal::selectAllSuppressedAtoms() {
  bool needs_update{false};
  for (auto &atom : m_atoms) {
    if (atom.isSuppressed()) {
      atom.setSelected(true);
      needs_update = true;
    }
  }

  if (needs_update)
    emit atomsChanged();
}

void DeprecatedCrystal::selectAtomsInsideSurface(Surface *surface) {
  Q_ASSERT(surface);
  bool needs_update{false};

  unselectAllAtoms();

  for (Atom &atom : m_atoms) {
    for (AtomId insideAtomId : surface->insideAtoms()) {
      if (atom.atomId() == insideAtomId) {
        atom.setSelected(true);
        needs_update = true;
      }
    }
  }
  if (needs_update) {
    emit atomsChanged();
  }
}

// This selects all atoms outside the surface not just
// the surfaces de atoms (as returned by currentSurface()->outsideAtoms())
void DeprecatedCrystal::selectAtomsOutsideSurface(Surface *surface) {
  Q_ASSERT(surface);

  unselectAllAtoms();

  for (Atom &atom : m_atoms) {
    bool isInsideAtom = false;
    for (AtomId insideAtomId : surface->insideAtoms()) {
      if (atom.atomId() == insideAtomId) {
        isInsideAtom = true;
      }
    }
    if (!isInsideAtom) {
      atom.setSelected(true);
    }
  }
  emit atomsChanged();
}

void DeprecatedCrystal::selectFragmentContaining(int refAtomIndex) {
  int fragment = _fragmentForAtom[refAtomIndex];
  const auto &fragAtoms = m_atomsForFragment[fragment];
  for (int atomIndex : fragAtoms) {
    m_atoms[atomIndex].setSelected(true);
  }
  emit atomsChanged();
}

void DeprecatedCrystal::selectAtomsWithEquivalentAtomIds(
    QVector<AtomId> atomIds) {
  for (Atom &atom : m_atoms) {
    for (AtomId atomId : atomIds) {
      if (atom.atomId() == atomId) {
        atom.setSelected(true);
      }
    }
  }
  emit atomsChanged();
}

int DeprecatedCrystal::numberOfSelectedFragments() {
  int result = 0;
  if (m_atoms.size() == 0)
    return result;

  for (int fragIndex = 0; fragIndex < numberOfFragments(); fragIndex++) {
    if (fragmentIsSelected(fragIndex)) {
      result++;
    }
  }
  return result;
}

void DeprecatedCrystal::invertSelection() {
  for (auto &atom : m_atoms)
    atom.setSelected(!atom.isSelected());
  emit atomsChanged();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Symmetry
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// Used by Info Document
SymopId DeprecatedCrystal::symopIdForFragment(int fragIndex) {
  CrystalSymops crystSymops = calculateCrystalSymops(0, fragIndex);
  if (crystSymops.size() > 0) {
    return crystSymops.keys()[0];
  } else {
    return NOSYMOP;
  }
}

Vector3q DeprecatedCrystal::cellTranslation(Shift shift) {
  return _unitCell.directCellMatrix() * vectorFromShift(shift);
}

// This depends on symmetry related atoms having the same label.
// At the moment this is the only way I can tell.
QVector<int>
DeprecatedCrystal::symmetryRelatedUnitCellAtomsForUnitCellAtom(int atomIndex) {
  QVector<int> result;

  QString label = _unitCellAtomList[atomIndex].label();
  for (int i = 0; i < _unitCellAtomList.size(); ++i) {
    if (label == _unitCellAtomList[i].label()) {
      result.append(i);
    }
  }
  return result;
}

// Find all of the CrystalSymops which transform the fragment given by
// fragmentIndex1
// into the the fragment given by fragmentIndex2
// There is often more than one CrystalSymop that will do the transformation.
CrystalSymops
DeprecatedCrystal::calculateCrystalSymops(int fragmentIndex1,
                                          int fragmentIndex2) const {
  return calculateCrystalSymops(atomIdsForFragment(fragmentIndex1),
                                atomIdsForFragment(fragmentIndex2));
}

// Find all of the CrystalSymops between the atoms of a fragment and the atoms
// inside a surface.
// There is often more than one CrystalSymop that will do the transformation.
CrystalSymops
DeprecatedCrystal::calculateCrystalSymops(const Surface *surface,
                                          int fragmentIndex) const {
  QVector<AtomId> l(surface->insideAtoms().begin(),
                    surface->insideAtoms().end());
  return calculateCrystalSymops(l, atomIdsForFragment(fragmentIndex));
}

// Find all of the CrystalSymops that transform fragment given by sourceAtoms
// into the fragment given by the destAtoms.
// There is often more than one CrystalSymop that will do the transformation.
// A CrystalSymop is combination of
// (i) Rotation Matrix (the rotation part of a spacegroup symop hence we store a
// SymopId)
// (ii) Shift (Vector3q)
// These CrystalSymops transform fractional coordinates.
// They can be easily transformed to a cartesian coordinate system (see
// Surface::symmetryTransform)
CrystalSymops DeprecatedCrystal::calculateCrystalSymops(
    const QVector<AtomId> &sourceAtoms,
    const QVector<AtomId> &destAtoms) const {
  CrystalSymops crystalSymops;

  // (1) Loop over every destAtom
  // (2) Loop over every sourceAtom
  // (3) Is there a spacegroup symmetry operation that takes from atom inside
  // the surface to the fragment atom?
  // (4) Yes, then calculate shift
  // (5) Sanity Check: There shouldn't be two different shifts for the same
  // symopId
  // (6) Store CrystalStymop
  for (const auto &destAtom : destAtoms) {       // (1)
    for (const auto &sourceAtom : sourceAtoms) { // (2)

      SymopId symopId = NOSYMOP; //
      symopId = symopIdForUnitCellAtoms(sourceAtom.unitCellIndex,
                                        destAtom.unitCellIndex); //
      if (symopId != NOSYMOP) {                                  // (3)

        Vector3q shift = calculateShift(destAtom, sourceAtom, symopId); // (4)

#ifdef QT_DEBUG                                //
        if (crystalSymops.contains(symopId)) { //
          // Q_ASSERT(isSameShift(crystalSymops[symopId],shift));         //
        } //
#endif    // (5)

        crystalSymops[symopId] = shift; // (6)
      }
    }
  }
  return crystalSymops;
}

SymopId DeprecatedCrystal::symopIdForUnitCellAtoms(int sourceAtomIndex,
                                                   int transAtomIndex) const {
  return m_symopMappingTable(sourceAtomIndex, transAtomIndex);
}

// To transform sourceAtom to destAtom you need to apply a rotation, R and a
// shift, T.
// R is given by:
//		spaceGroup()->rotationMatrixForSymop(symopId)
// T is calculated by this function
Vector3q DeprecatedCrystal::calculateShift(const AtomId &destAtom,
                                           const AtomId &sourceAtom,
                                           const SymopId &symopId) const {
  Vector3q T_dest = vectorFromShift(destAtom.shift);
  Vector3q T_source = vectorFromShift(sourceAtom.shift);
  Vector3q T_symop = spaceGroup().translationForSymop(symopId);

  Atom unitAtom = _unitCellAtomList[sourceAtom.unitCellIndex];
  unitAtom.applySymop(spaceGroup(), _unitCell.directCellMatrix(), symopId, 0);
  Vector3q T_unit = vectorFromShift(unitAtom.unitCellShift());

  Vector3q T_final = T_dest - T_unit + T_symop -
                     spaceGroup().rotationMatrixForSymop(symopId) * T_source;

  return T_final;
}

Vector3q DeprecatedCrystal::vectorFromShift(const Shift &shift) const {
  return {static_cast<double>(shift.h), static_cast<double>(shift.k),
          static_cast<double>(shift.l)};
}

bool DeprecatedCrystal::isSameShift(const Vector3q &s1,
                                    const Vector3q &s2) const {
  const float TOL = 0.001f;
  bool result = true;
  for (int i = 0; i < 3; ++i) {
    result = result && fabs(s1[i] - s2[i]) < TOL;
  }
  return result;
}

QVector<Vector3q>
DeprecatedCrystal::cellShiftsFromCellLimits(const QVector3D &cellLimits) const {
  QVector<Vector3q> shifts;

  for (int i = 0; i <= cellLimits[0]; ++i) {
    for (int j = 0; j <= cellLimits[1]; ++j) {
      for (int k = 0; k <= cellLimits[2]; ++k) {
        Vector3q shift;
        shift << i, j, k;
        shifts << shift;
      }
    }
  }
  return shifts;
}

QVector<Shift> DeprecatedCrystal::getCellShifts(const QVector<Atom> &atoms,
                                                float radius) const {
  QVector<Shift> shiftList;

  for (const auto &atom : atoms) {
    Shift referenceShift = atom.unitCellShift();
    QVector<Shift> radialShifts =
        shiftsWithinRadiusOfReferenceShift(referenceShift, radius);
    foreach (Shift shift, radialShifts) {
      if (!shiftListContainsShift(shiftList, shift)) {
        shiftList << shift;
      }
    }
  }
  return shiftList;
}

QVector<Shift> DeprecatedCrystal::shiftsWithinRadiusOfReferenceShift(
    const Shift &referenceShift, float radius) const {
  QVector<Shift> radialShifts;

  int aa = ceil(radius / _unitCell.a());
  int bb = ceil(radius / _unitCell.b());
  int cc = ceil(radius / _unitCell.c());

  int amin = referenceShift.h - aa;
  int amax = referenceShift.h + aa;
  int bmin = referenceShift.k - bb;
  int bmax = referenceShift.k + bb;
  int cmin = referenceShift.l - cc;
  int cmax = referenceShift.l + cc;

  for (int a = amin; a <= amax; ++a) {
    for (int b = bmin; b <= bmax; ++b) {
      for (int c = cmin; c <= cmax; ++c) {
        radialShifts.push_back({a, b, c});
      }
    }
  }
  return radialShifts;
}

///////////////////////////////////////////////////////////////////////////////
//
// If the symop mapping table was complete I could use this routine
// instead of unitCellAtomIndexOf (which is very brute force).
//
///////////////////////////////////////////////////////////////////////////////
// int Crystal::applySymopToUnitCellAtomIndex(SymopId symopId, int
// sourceAtomIndex)
//{
//	int result = _symopMappingTable[sourceAtomIndex].indexOf(symopId);
//	Q_ASSERT(result != -1);
//	return result;
//}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Elements
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void DeprecatedCrystal::makeListOfElementSymbols() {
  Q_ASSERT(_unitCellAtomList.size() != 0);

  for (const auto &atom : _unitCellAtomList) {
    if (!_elementSymbols.contains(atom.symbol(), Qt::CaseSensitive)) {
      _elementSymbols.append(atom.symbol());
    }
  }
}

QStringList DeprecatedCrystal::listOfElementSymbols() {
  Q_ASSERT(_elementSymbols.size() != 0);
  return _elementSymbols;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Unitcell, Radius and Origin
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void DeprecatedCrystal::translateOrigin(const QVector3D &t) {
  _origin(0) += t.x();
  _origin(1) += t.y();
  _origin(2) += t.z();
}

void DeprecatedCrystal::resetOrigin() {
  _origin = Vector3q::Zero();
  for (const auto &atom : m_atoms) {
    _origin.noalias() += atom.posvector();
  }
  _origin = _origin / m_atoms.size();
}

void DeprecatedCrystal::setOrigin(Vector3q pos) { _origin = pos; }

void DeprecatedCrystal::calculateRadius() {
  _radius = 0.0;
  for (const auto &atom : m_atoms) {
    float distanceFromOrigin = (_origin - atom.posvector()).norm();
    _radius = std::max(_radius, distanceFromOrigin);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Contact Atoms
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void DeprecatedCrystal::showVdwContactAtoms(bool show) {
  show ? appendVdwContactAtoms() : removeVdwContactAtoms();
}

void DeprecatedCrystal::removeVdwContactAtoms() {
  if (hasAnyVdwContactAtoms()) {
    auto it = std::remove_if(m_atoms.begin(), m_atoms.end(),
                             [](const Atom &a) { return a.isContactAtom(); });
    m_atoms.erase(it, m_atoms.end());
    updateAtomListInfo();
    emit atomsChanged();
  }
}

bool DeprecatedCrystal::hasAnyVdwContactAtoms() const {
  return std::any_of(m_atoms.begin(), m_atoms.end(),
                     [](const Atom &a) { return a.isContactAtom(); });
}

void DeprecatedCrystal::calculateVdwContactInfo() {
  clearVdwContactInfo();

  for (int i = 0; i < m_atoms.size(); ++i) {
    const auto &atom_i = m_atoms[i];
    if (atom_i.isContactAtom()) {
      continue;
    }

    for (int j = 0; j < i; ++j) {
      const auto &atom_j = m_atoms[j];
      if (atom_j.isContactAtom()) {
        continue;
      }

      // atom_i and atom_j is a unique pair of atom (non-contact)
      float distance = atom_i.distanceToAtom(atom_j);
      float sumVdwRadii = atom_i.vdwRadius() + atom_j.vdwRadius();
      double distanceCriteria = sumVdwRadii * CLOSECONTACT_FACTOR;

      if (distance <= distanceCriteria) {
        if (hasSameFragmentAtoms(i, j)) {
          if (isSuitableIntraCloseContact(i, j)) {
            addVdwContact({i, j, distance, sumVdwRadii, true});
          }
        } else {
          addVdwContact({i, j, distance, sumVdwRadii, false});
        }
      }
    }
  }
  calculateHBondList();
  calculateCloseContactsTable();
}

bool DeprecatedCrystal::isSuitableIntraCloseContact(int i, int j) {
  return numberOfCovalentBondedAtomsBetweenAtoms(i, j) >=
         GLOBAL_MIN_NUM_BONDS_FOR_INTRA;
}

void DeprecatedCrystal::clearVdwContactInfo() { m_vanDerWaalsContacts.clear(); }

void DeprecatedCrystal::addVdwContact(const VanDerWaalsContact &contact) {
  m_vanDerWaalsContacts.push_back(contact);
}

void DeprecatedCrystal::appendVdwContactAtoms() {
  QVector<Atom> newAtoms;
  for (int i = 0; i < m_atoms.size(); ++i) {
    appendConnectionsToAtom(m_atoms[i], _vdwCellConnectionTable, newAtoms,
                            QVector<int>(), true);
  }
  appendUniqueAtomsOnly(newAtoms);
  updateAtomListInfo();
  emit atomsChanged();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Atom Suppression
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool DeprecatedCrystal::hasSuppressedAtoms() const {
  return std::any_of(m_atoms.begin(), m_atoms.end(),
                     [](const Atom &a) { return a.isSuppressed(); });
}

void DeprecatedCrystal::suppressSelectedAtoms() {
  bool needs_update{false};
  for (auto &atom : m_atoms) {
    if (atom.isSelected()) {
      atom.setSuppressed(true);
      needs_update = true;
    }
  }
  if (needs_update) {
    updateConnectivityInfo();
    emit atomsChanged();
  }
}

void DeprecatedCrystal::unsuppressSelectedAtoms() {
  bool needs_update{false};
  for (auto &atom : m_atoms) {
    if (atom.isSelected()) {
      atom.setSuppressed(false);
      needs_update = true;
    }
  }
  if (needs_update) {
    updateConnectivityInfo();
    emit atomsChanged();
  }
}

void DeprecatedCrystal::unsuppressAllAtoms() {
  for (auto &atom : m_atoms)
    atom.setSuppressed(false);
  updateConnectivityInfo();
  emit atomsChanged();
}

QVector<int> DeprecatedCrystal::suppressedAtomsAsUnitCellAtomIndices() {
  QSet<int> result;
  for (const auto &atom : m_atoms) {
    if (atom.isSuppressed())
      result.insert(atom.unitCellAtomIndex());
  }
  return symmetryRelatedAtomsForUnitCellAtomIndices(result).values().toVector();
}

QSet<int> DeprecatedCrystal::symmetryRelatedAtomsForUnitCellAtomIndices(
    QSet<int> indices) {
  QSet<int> result;
  for (int i = 0; i < _unitCellAtomList.size(); ++i) {
    foreach (int indexToCheck, indices) {
      if (symopIdForUnitCellAtoms(indexToCheck, i) != -1) {
        result.insert(i);
      }
    }
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Wavefunctions
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DeprecatedCrystal::addWavefunction(const Wavefunction &wavefunction) {
  _wavefunctions.push_back(wavefunction);
}

void DeprecatedCrystal::addMonomerEnergy(const MonomerEnergy &m) {
  m_monomerEnergies.push_back(m);
}

// Replaces an existing wavefunction but ensures the wavefunction is placed at
// the end of the list of wavefunctions.
void DeprecatedCrystal::replaceExistingWavefunction(
    const Wavefunction &wavefunction) {
  int wavefunctionIndex =
      indexOfWavefunctionMatchingParameters(wavefunction.jobParameters());
  _wavefunctions.erase(_wavefunctions.begin() + wavefunctionIndex);
  _wavefunctions.push_back(wavefunction);
}

QVector<TransformableWavefunction>
DeprecatedCrystal::transformableWavefunctionForCurrentSelection() {
  return transformableWavefunctionsForAtoms(selectedAtomsAsIds());
}

QVector<TransformableWavefunction>
DeprecatedCrystal::transformableWavefunctionsForFragment(int fragmentIndex) {
  return transformableWavefunctionsForAtoms(atomIdsForFragment(fragmentIndex));
}

QVector<TransformableWavefunction>
DeprecatedCrystal::transformableWavefunctionsForAtoms(
    const QVector<AtomId> &fragAtomIds) {
  QVector<TransformableWavefunction> result;

  foreach (const Wavefunction &wavefunction, _wavefunctions) {
    QVector<AtomId> wavefunctionAtomIds = wavefunction.atomIds();

    if (wavefunctionAtomIds.size() != fragAtomIds.size()) {
      continue;
    }

    CrystalSymops crystalSymops =
        calculateCrystalSymops(wavefunctionAtomIds, fragAtomIds);
    if (crystalSymops.size() > 0) {
      Q_ASSERT(wavefunctionAtomIds.size() == fragAtomIds.size());
      TransformableWavefunction tw;
      tw.first = wavefunction;
      int symopId = crystalSymops.firstKey();
      Matrix3q m = unitCell().directCellMatrix() *
                   spaceGroup().rotationMatrixForSymop(symopId) *
                   unitCell().inverseCellMatrix();
      Vector3q v = unitCell().directCellMatrix() * crystalSymops[symopId];
      tw.second = QPair<Matrix3q, Vector3q>(m, v);
      result.append(tw);
    }
  }

  return result;
}

std::optional<TransformableWavefunction>
DeprecatedCrystal::transformableWavefunctionForAtomsFromWavefunction(
    const Wavefunction &wavefunction, const QVector<AtomId> &fragAtomIds) {

  CrystalSymops crystalSymops =
      calculateCrystalSymops(wavefunction.atomIds(), fragAtomIds);
  if (crystalSymops.size() <= 0)
    return {};

  int symopId = crystalSymops.firstKey();
  Matrix3q m = unitCell().directCellMatrix() *
               spaceGroup().rotationMatrixForSymop(symopId) *
               unitCell().inverseCellMatrix();
  Vector3q v = unitCell().directCellMatrix() * crystalSymops[symopId];

  return std::make_optional(
      TransformableWavefunction{wavefunction, QPair<Matrix3q, Vector3q>(m, v)});
}

QVector<QPair<TransformableWavefunction, TransformableWavefunction>>
DeprecatedCrystal::transformableWavefunctionsForFragmentAtoms(
    const QVector<AtomId> &fragAtomsA, const QVector<AtomId> &fragAtomsB) {
  QVector<QPair<TransformableWavefunction, TransformableWavefunction>> result;

  bool AandBSymmetryRelated =
      fragmentAtomsAreSymmetryRelated(fragAtomsA, fragAtomsB);

  if (AandBSymmetryRelated) {

    for (const Wavefunction &wavefunction : _wavefunctions) {
      auto twForA = transformableWavefunctionForAtomsFromWavefunction(
          wavefunction, fragAtomsA);
      auto twForB = transformableWavefunctionForAtomsFromWavefunction(
          wavefunction, fragAtomsB);
      if (twForA && twForB) {
        result.push_back(
            QPair<TransformableWavefunction, TransformableWavefunction>(
                *twForA, *twForB));
      }
    }

  } else {
    QVector<TransformableWavefunction> wfnsForA;
    QVector<TransformableWavefunction> wfnsForB;
    for (const Wavefunction &wavefunction : _wavefunctions) {
      auto twForA = transformableWavefunctionForAtomsFromWavefunction(
          wavefunction, fragAtomsA);
      auto twForB = transformableWavefunctionForAtomsFromWavefunction(
          wavefunction, fragAtomsB);
      if (twForA) { // Got a wavefunction for A
        wfnsForA.push_back(*twForA);
      } else if (twForB) { // Got a wavefunction for B
        wfnsForB.push_back(*twForB);
      }
    }
    for (const auto &wfnA : wfnsForA) {
      for (const auto &wfnB : wfnsForB) {
        if (wfnA.first.description() == wfnB.first.description()) {
          result.push_back(
              QPair<TransformableWavefunction, TransformableWavefunction>(
                  wfnA, wfnB));
        }
      }
    }
  }

  return result;
}

std::optional<Wavefunction> DeprecatedCrystal::wavefunctionMatchingParameters(
    const JobParameters &jobParams) {
  int index = indexOfWavefunctionMatchingParameters(jobParams);
  if (index < 0)
    return {};
  return _wavefunctions[index];
}

std::optional<MonomerEnergy> DeprecatedCrystal::monomerEnergyMatchingParameters(
    const JobParameters &jobParams) {
  int index = indexOfMonomerEnergymatchingParams(jobParams);
  if (index < 0)
    return {};
  return m_monomerEnergies[index];
}

// Finds *first* matching wavefunction
//
// We do include the wavefunction source between we treat wavefunctions
// at the same level of theory but generated with different QM packages
// as distinct - this allows to easily compare the results of these programs
// (hopefully they are the same).
// We also check if the wavefunction can be symmetry transformed (so we get the
// proper behaviour for z'>1 structures)
int DeprecatedCrystal::indexOfWavefunctionMatchingParameters(
    const JobParameters &jobParams) {
  int result = -1;
  const auto &atoms = jobParams.atoms;
  for (int i = 0; i < _wavefunctions.size(); ++i) {
    if (!sameWavefunctionInJobParameters(jobParams,
                                         _wavefunctions[i].jobParameters()))
      continue;
    auto tw = transformableWavefunctionForAtomsFromWavefunction(
        _wavefunctions[i], atoms);
    if (tw) {
      qDebug() << "Matching wavefunction: " << i;
      result = i;
      break;
    }
  }

  return result;
}

std::optional<MonomerEnergy>
DeprecatedCrystal::monomerEnergyForAtomsFromMonomerEnergy(
    const MonomerEnergy &monomerEnergy, const QVector<AtomId> &atoms) {

  CrystalSymops crystalSymops =
      calculateCrystalSymops(monomerEnergy.jobParams.atoms, atoms);
  if (crystalSymops.size() <= 0)
    return {};

  return monomerEnergy;
}

int DeprecatedCrystal::indexOfMonomerEnergymatchingParams(
    const JobParameters &jobParams) {
  int result = -1;
  const auto &atoms = jobParams.atoms;
  for (int i = 0; i < m_monomerEnergies.size(); ++i) {
    if (!jobParams.hasSameWavefunctionParameters(
            m_monomerEnergies[i].jobParams))
      continue;
    auto m =
        monomerEnergyForAtomsFromMonomerEnergy(m_monomerEnergies[i], atoms);
    if (m) {
      qDebug() << "Matching Monomer Energy: " << i;
      result = i;
      break;
    }
  }
  return result;
}

bool DeprecatedCrystal::sameWavefunctionInJobParameters(
    const JobParameters &jobParamsA, const JobParameters &jobParamsB) {
  return jobParamsA.hasSameWavefunctionParameters(jobParamsB);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Interaction Energies
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DeprecatedCrystal::addInteractionEnergyData(
    QMap<EnergyType, double> interactionEnergyData,
    const JobParameters &jobParams) {
  InteractionEnergy energy;
  energy.first = interactionEnergyData;
  energy.second = jobParams;
  m_interactionEnergies.append(energy);
  updateTotalEnergy(m_interactionEnergies.size() - 1);
  updateEnergyTables(m_interactionEnergies.size() - 1);
}

bool DeprecatedCrystal::hasInteractionEnergies() {
  return m_interactionEnergies.size() > 0;
}

// This also checks the wavefunctions are the same
bool DeprecatedCrystal::haveInteractionEnergyForPairInJobParameters(
    const JobParameters &jobParams) {
  int fragA1Size = jobParams.atomGroups[0];
  QVector<AtomId> fragA1AtomIds = jobParams.atoms.mid(0, fragA1Size);
  QVector<AtomId> fragA2AtomIds = jobParams.atoms.mid(fragA1Size);
  return indexOfInteractionEnergyForAtomIdsWithWavefunctionComparison(
             fragA1AtomIds, fragA2AtomIds, jobParams) != -1;
}

// Returns the first entry that matches
int DeprecatedCrystal::indexOfInteractionEnergyForFragments(
    int fragmentIndex1, int fragmentIndex2) {
  return indexOfInteractionEnergyForAtomIds(atomIdsForFragment(fragmentIndex1),
                                            atomIdsForFragment(fragmentIndex2));
}

// Returns the first entry that matches
int DeprecatedCrystal::indexOfInteractionEnergyForAtomIds(
    QVector<AtomId> fragA1AtomIds, QVector<AtomId> fragA2AtomIds) {
  int energyIndex = -1;

  for (int i = 0; i < m_interactionEnergies.size(); ++i) {
    const JobParameters &jobParams = m_interactionEnergies[i].second;
    int fragB1Size = jobParams.atomGroups[0];
    QVector<AtomId> fragB1AtomIds = jobParams.atoms.mid(0, fragB1Size);
    QVector<AtomId> fragB2AtomIds = jobParams.atoms.mid(fragB1Size);

    if (pairsAreEquivalent(fragA1AtomIds, fragA2AtomIds, fragB1AtomIds,
                           fragB2AtomIds)) {
      energyIndex = i;
      break;
    }
  }

  return energyIndex;
}

int DeprecatedCrystal::
    indexOfInteractionEnergyForFragmentsWithEnergyTheoryComparison(
        int fragmentIndex1, int fragmentIndex2, EnergyTheory theory) {
  return indexOfInteractionEnergyForAtomIdsWithEnergyTheoryComparison(
      atomIdsForFragment(fragmentIndex1), atomIdsForFragment(fragmentIndex2),
      theory);
}

int DeprecatedCrystal::
    indexOfInteractionEnergyForAtomIdsWithEnergyTheoryComparison(
        QVector<AtomId> fragA1AtomIds, QVector<AtomId> fragA2AtomIds,
        EnergyTheory theory) {
  int energyIndex = -1;

  for (int i = 0; i < m_interactionEnergies.size(); ++i) {
    const JobParameters &jobParams = m_interactionEnergies[i].second;
    int fragB1Size = jobParams.atomGroups[0];
    QVector<AtomId> fragB1AtomIds = jobParams.atoms.mid(0, fragB1Size);
    QVector<AtomId> fragB2AtomIds = jobParams.atoms.mid(fragB1Size);

    bool pairsEquivalent = pairsAreEquivalent(fragA1AtomIds, fragA2AtomIds,
                                              fragB1AtomIds, fragB2AtomIds);
    bool sameEnergyTheory =
        theory.first == jobParams.theory && theory.second == jobParams.basisset;
    if (sameEnergyTheory && pairsEquivalent) {
      energyIndex = i;
      break;
    }
  }

  return energyIndex;
}

int DeprecatedCrystal::
    indexOfInteractionEnergyForFragmentsWithWavefunctionComparison(
        int fragmentIndex1, int fragmentIndex2,
        const JobParameters &fragJobParams) {
  return indexOfInteractionEnergyForAtomIdsWithWavefunctionComparison(
      atomIdsForFragment(fragmentIndex1), atomIdsForFragment(fragmentIndex2),
      fragJobParams);
}

int DeprecatedCrystal::
    indexOfInteractionEnergyForAtomIdsWithWavefunctionComparison(
        QVector<AtomId> fragA1AtomIds, QVector<AtomId> fragA2AtomIds,
        const JobParameters &fragJobParams) {
  int energyIndex = -1;

  for (int i = 0; i < m_interactionEnergies.size(); ++i) {
    const JobParameters &jobParams = m_interactionEnergies[i].second;
    int fragB1Size = jobParams.atomGroups[0];
    QVector<AtomId> fragB1AtomIds = jobParams.atoms.mid(0, fragB1Size);
    QVector<AtomId> fragB2AtomIds = jobParams.atoms.mid(fragB1Size);

    if (sameWavefunctionInJobParameters(fragJobParams, jobParams) &&
        pairsAreEquivalent(fragA1AtomIds, fragA2AtomIds, fragB1AtomIds,
                           fragB2AtomIds)) {
      energyIndex = i;
      break;
    }
  }

  return energyIndex;
}

QVector<QColor> DeprecatedCrystal::interactionEnergyColors() {
  int numberOfUniqueEnergies = m_sameEnergyDifferentTheory.size();
  ColorScheme colorScheme = colorSchemeFromString(
      settings::readSetting(settings::keys::ENERGY_COLOR_SCHEME).toString());
  QVector<QColor> uniqueEnergyColors;
  for (int i = 0; i < numberOfUniqueEnergies; ++i) {
    uniqueEnergyColors.append(ColorSchemer::color(
        colorScheme, i, 0, numberOfUniqueEnergies - 1, false));
  }

  // Build complete list of colors for all interactions energies
  int numberOfEnergies = m_interactionEnergies.size();
  QVector<QColor> energyColors(numberOfEnergies);

  for (int i = 0; i < m_sameEnergyDifferentTheory.size(); ++i) {
    QColor color = uniqueEnergyColors[i];
    foreach (int energyIndex, m_sameEnergyDifferentTheory[i]) {
      energyColors[energyIndex] = color;
    }
  }

  return QVector<QColor>(energyColors.begin(), energyColors.end());
}

QColor DeprecatedCrystal::energyColorForPair(int fragmentIndex1,
                                             int fragmentIndex2) {
  QColor color;

  int energyIndex =
      indexOfInteractionEnergyForFragments(fragmentIndex1, fragmentIndex2);
  if (energyIndex != -1) {
    auto colors = interactionEnergyColors();
    color = colors[energyIndex];
  } else {
    color = Qt::gray;
  }

  return color;
}

QVector<SymopId> DeprecatedCrystal::interactionEnergySymops() {
  QVector<SymopId> symops;

  for (int i = 0; i < m_interactionEnergies.size(); ++i) {
    const JobParameters &jobParams = m_interactionEnergies[i].second;

    QVector<AtomId> fragAAtomIds =
        jobParams.atoms.mid(0, jobParams.atomGroups[0]);
    QVector<AtomId> fragBAtomIds = jobParams.atoms.mid(jobParams.atomGroups[0]);
    CrystalSymops crystalSymops =
        calculateCrystalSymops(fragAAtomIds, fragBAtomIds);
    if (crystalSymops.size() > 0) {
      SymopId symopId = crystalSymops.firstKey();
      symops.append(symopId);
    } else { // If co-crystal or z'>1 there will be no symop relating the
             // fragments
      symops.append(-1);
    }
  }
  return symops;
}

QString
DeprecatedCrystal::calculateFragmentPairIdentifier(QVector<AtomId> fragAtomsA,
                                                   QVector<AtomId> fragAtomsB) {
  auto symops = calculateCrystalSymops(fragAtomsA, fragAtomsB);
  auto sym = -1;
  if (!symops.isEmpty()) {
    sym = symops.firstKey();
  }
  QVector3D p1 = centerOfMassOfAtomIds(fragAtomsA);
  QVector3D p2 = centerOfMassOfAtomIds(fragAtomsB);
  auto distance = (p1 - p2).length();

  return QString("symop = %1, r = %2")
      .arg(spaceGroup().symopAsString(sym))
      .arg(QString::number(distance));
}

QVector<double> DeprecatedCrystal::interactionEnergyDistances() {
  QVector<double> distances;
  for (int i = 0; i < m_interactionEnergies.size(); ++i) {
    const JobParameters &jobParams = m_interactionEnergies[i].second;

    QVector<AtomId> fragAAtomIds =
        jobParams.atoms.mid(0, jobParams.atomGroups[0]);
    QVector<AtomId> fragBAtomIds = jobParams.atoms.mid(jobParams.atomGroups[0]);
    QVector3D p1 = centerOfMassOfAtomIds(fragAAtomIds);
    QVector3D p2 = centerOfMassOfAtomIds(fragBAtomIds);
    auto distance = (p1 - p2).length();
    distances.append(distance);
  }
  return distances;
}

QMap<int, int> DeprecatedCrystal::interactionEnergyFragmentCount() {
  QMap<int, int> numFragments;
  const int keyFrag = keyFragment();

  for (int fragIndex = 0; fragIndex < m_atomsForFragment.size(); fragIndex++) {
    if (fragIndex == keyFrag) {
      continue;
    }
    int id = indexOfInteractionEnergyForFragments(keyFrag, fragIndex);
    numFragments[id] = numFragments[id] + 1;
  }
  return numFragments;
}

bool DeprecatedCrystal::energyIsBenchmarked(InteractionEnergy energy) {
  return energyModelFromJobParameters(energy.second) != EnergyModel::None;
}

QVector<bool> DeprecatedCrystal::interactionEnergyBenchmarkedEnergyStatuses() {
  QVector<bool> benchmarkedEnergyStatuses;
  for (int i = 0; i < m_interactionEnergies.size(); ++i) {
    benchmarkedEnergyStatuses.append(
        energyIsBenchmarked(m_interactionEnergies[i]));
  }
  return benchmarkedEnergyStatuses;
}

void DeprecatedCrystal::updateEnergyInfo(FrameworkType frameworkType) {
  clearEnergyInfos();
  auto energyTypes = getEnergyTypes();
  auto frameworkColors = getFrameworkColors();
  const auto centroids = centroidsOfFragments();

  double energyCutoff = energyCutoffForEnergyFramework(frameworkType);

  /*
   * The below code works, but doesn't show all symmetry related copies
   *
    double energyCutoff = energyCutoffForEnergyFramework(frameworkType);
    QMap<AtomId, int> atomId2AtomIndex;
    for(int i = 0; i < m_atoms.size(); i++) {
        atomId2AtomIndex.insert(m_atoms[i]->atomId(), i);
    }

    const auto energyType = energyTypes[frameworkType];

    for (int i = 0; i < m_interactionEnergies.size(); ++i) {
      const JobParameters &jobParams = m_interactionEnergies[i].second;
      if(m_energyTheory.first != jobParams.theory) continue;
      if(m_energyTheory.second != jobParams.basisset) continue;

      int fragB1Size = jobParams.atomGroups[0];
      auto leftIt = atomId2AtomIndex.find(jobParams.atoms[0]);
      if(leftIt == atomId2AtomIndex.end()) continue;
      int leftAtomIndex = *leftIt;
      auto rightIt = atomId2AtomIndex.find(jobParams.atoms[fragB1Size]);
      if(rightIt == atomId2AtomIndex.end()) continue;
      int rightAtomIndex = *rightIt;

      int leftFrag = _fragmentForAtom[leftAtomIndex];
      int rightFrag = _fragmentForAtom[rightAtomIndex];
      double energy = energyForEnergyType(i, energyType);

      const int ENERGY_WIDTH = 6;
      const int ENERGY_PRECISION = 1;
      QString energyString =
          QString("%1").arg(energy, ENERGY_WIDTH, 'f', ENERGY_PRECISION);

      QColor color = frameworkColors[frameworkType];

      if (fabs(energy) > energyCutoff) {
        _energyInfos.append(FragmentPairInfo(centroids[leftFrag],
   centroids[rightFrag], color, energyString));
      }
    }
    */

  // TODO speed this up - it's super inefficient.
  for (int i = 0; i < numberOfFragments(); ++i) {
    for (int j = i + 1; j < numberOfFragments(); ++j) {

      int energyIndex =
          indexOfInteractionEnergyForFragmentsWithEnergyTheoryComparison(
              i, j, m_energyTheory);

      if (energyIndex != -1) {

        double energy =
            energyForEnergyType(energyIndex, energyTypes[frameworkType]);

        const int ENERGY_WIDTH = 6;
        const int ENERGY_PRECISION = 1;
        QString energyString =
            QString("%1").arg(energy, ENERGY_WIDTH, 'f', ENERGY_PRECISION);

        QColor color = frameworkColors[frameworkType];

        if (fabs(energy) > energyCutoff) {
          _energyInfos.append(FragmentPairInfo(centroids[i], centroids[j],
                                               color, energyString));
        }
      }
    }
  }
}

EnergyModel DeprecatedCrystal::energyModelFromJobParameters(
    const JobParameters &jobParams) {
  EnergyModel model = EnergyModel::None;
  if (jobParams.theory == EnergyDescription::qualitativeEnergyModelTheory()) {
    if (jobParams.basisset ==
        EnergyDescription::qualitativeEnergyModelBasisset()) {
      model = EnergyDescription::qualitativeEnergyModel();
    }
  } else if (jobParams.theory ==
             EnergyDescription::quantitativeEnergyModelTheory()) {
    if (jobParams.basisset ==
            EnergyDescription::quantitativeEnergyModelBasisset() ||
        jobParams.basisset == BasisSet::DGDZVP) {
      model = EnergyDescription::quantitativeEnergyModel();
    }
  }
  if (jobParams.theory == Method::DLPNO)
    model = EnergyModel::DLPNO;
  if (jobParams.isXtbJob())
    model = EnergyModel::DFTB;
  return model;
}

float DeprecatedCrystal::coulombFactor(InteractionEnergy ie) {
  EnergyModel model = energyModelFromJobParameters(ie.second);
  return coulombScaleFactors[model];
}

float DeprecatedCrystal::polarizationFactor(InteractionEnergy ie) {
  EnergyModel model = energyModelFromJobParameters(ie.second);
  return polarizationScaleFactors[model];
}

float DeprecatedCrystal::dispersionFactor(InteractionEnergy ie) {
  EnergyModel model = energyModelFromJobParameters(ie.second);
  return dispersionScaleFactors[model];
}

float DeprecatedCrystal::repulsionFactor(InteractionEnergy ie) {
  EnergyModel model = energyModelFromJobParameters(ie.second);
  return repulsionScaleFactors[model];
}

void DeprecatedCrystal::updateTotalEnergy(int energyIndex) {
  InteractionEnergy ie = m_interactionEnergies[energyIndex];

  const QVector<Method> NON_CE_MODELS{Method::DLPNO, Method::GFN0xTB,
                                      Method::GFN1xTB, Method::GFN2xTB};
  // don't rescale if it's not a CE model energy;
  for (const auto &m : NON_CE_MODELS) {
    if (ie.second.theory == m)
      return;
  }

  double totalEnergy =
      coulombFactor(ie) * ie.first[EnergyType::CoulombEnergy] +
      polarizationFactor(ie) * ie.first[EnergyType::PolarizationEnergy] +
      dispersionFactor(ie) * ie.first[EnergyType::DispersionEnergy] +
      repulsionFactor(ie) * ie.first[EnergyType::RepulsionEnergy];
  m_interactionEnergies[energyIndex].first[EnergyType::TotalEnergy] =
      totalEnergy;
}

bool DeprecatedCrystal::sameLevelOfTheory(InteractionEnergy energyA,
                                          InteractionEnergy energyB) {
  return sameWavefunctionInJobParameters(energyA.second, energyB.second);
}

bool DeprecatedCrystal::energyForSamePair(InteractionEnergy energyA,
                                          InteractionEnergy energyB) {
  const JobParameters &jobParamsA = energyA.second;
  int fragA1Size = jobParamsA.atomGroups[0];
  QVector<AtomId> fragA1AtomIds = jobParamsA.atoms.mid(0, fragA1Size);
  QVector<AtomId> fragA2AtomIds = jobParamsA.atoms.mid(fragA1Size);

  const JobParameters &jobParamsB = energyB.second;
  int fragB1Size = jobParamsB.atomGroups[0];
  QVector<AtomId> fragB1AtomIds = jobParamsB.atoms.mid(0, fragB1Size);
  QVector<AtomId> fragB2AtomIds = jobParamsB.atoms.mid(fragB1Size);

  return pairsAreEquivalent(fragA1AtomIds, fragA2AtomIds, fragB1AtomIds,
                            fragB2AtomIds);
}

void DeprecatedCrystal::rebuildEnergyTables() {
  for (int i = 0; i < m_interactionEnergies.size(); ++i) {
    updateEnergyTables(i);
  }
}

void DeprecatedCrystal::updateEnergyTables(int interactionEnergyIndex) {
  InteractionEnergy energy = m_interactionEnergies[interactionEnergyIndex];

  bool addedEnergy = false;
  for (int i = 0; i < m_sameTheoryDifferentEnergies.size(); ++i) {
    InteractionEnergy referenceEnergy =
        m_interactionEnergies[m_sameTheoryDifferentEnergies[i][0]];
    if (sameLevelOfTheory(energy, referenceEnergy)) {
      m_sameTheoryDifferentEnergies[i].append(interactionEnergyIndex);
      addedEnergy = true;
      break;
    }
  }
  if (!addedEnergy) {
    m_sameTheoryDifferentEnergies.append(QVector<int>{interactionEnergyIndex});
  }

  addedEnergy = false;
  for (int i = 0; i < m_sameEnergyDifferentTheory.size(); ++i) {
    InteractionEnergy referenceEnergy =
        m_interactionEnergies[m_sameEnergyDifferentTheory[i][0]];
    if (energyForSamePair(energy, referenceEnergy)) {
      m_sameEnergyDifferentTheory[i].append(interactionEnergyIndex);
      addedEnergy = true;
      break;
    }
  }
  if (!addedEnergy) {
    m_sameEnergyDifferentTheory.append(QVector<int>{interactionEnergyIndex});
  }
}

QString DeprecatedCrystal::energyComponentAsString(int index,
                                                   EnergyType energyType) {
  const int ENERGY_WIDTH = 6;
  const int ENERGY_PRECISION = 1;
  double energyValue = m_interactionEnergies[index].first[energyType];
  return QString("%1").arg(energyValue, ENERGY_WIDTH, 'f', ENERGY_PRECISION);
}

void DeprecatedCrystal::clearEnergyInfos() { _energyInfos.clear(); }

double DeprecatedCrystal::energyForEnergyType(int energyIndex,
                                              EnergyType energyType) {
  double result = 0.0;

  InteractionEnergy energy = m_interactionEnergies[energyIndex];
  double factor = 1.0;
  switch (energyType) {
  case EnergyType::CoulombEnergy:
    factor = coulombFactor(energy);
    break;
  case EnergyType::DispersionEnergy:
    factor = dispersionFactor(energy);
    break;
  case EnergyType::PolarizationEnergy:
    factor = polarizationFactor(energy);
    break;
  case EnergyType::RepulsionEnergy:
    factor = repulsionFactor(energy);
    break;
  default:
    break;
  }
  const auto val = energy.first.find(energyType);
  if (val != energy.first.end()) {
    result = val.value() * factor;
  }
  return result;
}

double
DeprecatedCrystal::energyCutoffForEnergyFramework(FrameworkType frameworkType) {
  auto cutoffSettingsKeys = getCutoffSettingsKeys();
  return settings::readSetting(cutoffSettingsKeys[frameworkType]).toFloat();
}

QVector<EnergyTheory> DeprecatedCrystal::energyTheories() {
  QVector<EnergyTheory> theories;

  for (const auto &energyIndices : sameTheoryDifferentEnergies()) {
    Q_ASSERT(energyIndices.size() > 0);

    InteractionEnergy energy = m_interactionEnergies[energyIndices[0]];
    EnergyTheory theory;
    theory.first = energy.second.theory;
    theory.second = energy.second.basisset;
    theories.append(theory);
  }

  return theories;
}

QVector<QString> DeprecatedCrystal::levelsOfTheoriesForLatticeEnergies() {
  QVector<QString> theories;

  for (const auto &energyIndices : sameTheoryDifferentEnergies()) {
    Q_ASSERT(energyIndices.size() > 0);
    InteractionEnergy energy = m_interactionEnergies[energyIndices[0]];
    QString theoryString = Wavefunction::levelOfTheoryString(
        energy.second.theory, energy.second.basisset);
    theories.append(theoryString);
  }

  return theories;
}

QVector<float> DeprecatedCrystal::latticeEnergies() {
  QVector<float> latticeEnergies;

  int keyFrag = keyFragment();
  QVector<int> fragmentIndices = findPairs(keyFrag);

  for (const auto &energyIndices : sameTheoryDifferentEnergies()) {
    const JobParameters &jobParams =
        m_interactionEnergies[energyIndices[0]].second;

    float latticeEnergy = 0.0;

    foreach (int fragmentIndex, fragmentIndices) {
      int energyIndex =
          indexOfInteractionEnergyForFragmentsWithWavefunctionComparison(
              keyFrag, fragmentIndex, jobParams);

      Q_ASSERT(energyIndex != -1);

      float totalEnergy =
          m_interactionEnergies[energyIndex].first[EnergyType::TotalEnergy];
      latticeEnergy += totalEnergy;
    }

    latticeEnergy /= 2.0; // Divide by two to account for double counting
    latticeEnergies.append(latticeEnergy);
  }

  return latticeEnergies;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Equivalent Pairs
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double DeprecatedCrystal::interCentroidDistance(FragmentPair pair) {
  return fabs((centroidOfFragment(pair.first) - centroidOfFragment(pair.second))
                  .length());
}

bool DeprecatedCrystal::pairsAreEquivalent(FragmentPair p1, FragmentPair p2) {
  bool centroidCriteria = fabs(interCentroidDistance(p1) -
                               interCentroidDistance(p2)) < INTER_CENTROID_TOL;
  if (centroidCriteria) {
    QPair<QVector3D, QVector3D> minPositionsP1 =
        positionsOfMinDistanceFragFrag(p1.first, p1.second);
    QPair<QVector3D, QVector3D> minPositionsP2 =
        positionsOfMinDistanceFragFrag(p2.first, p2.second);
    double minDistanceP1 =
        (minPositionsP1.first - minPositionsP1.second).length();
    double minDistanceP2 =
        (minPositionsP2.first - minPositionsP2.second).length();

    return fabs(minDistanceP1 - minDistanceP2) < MIN_DISTANCE_TOL;
  }
  return centroidCriteria;
}

bool DeprecatedCrystal::pairsAreEquivalent(QVector<AtomId> fragA1AtomIds,
                                           QVector<AtomId> fragA2AtomIds,
                                           QVector<AtomId> fragB1AtomIds,
                                           QVector<AtomId> fragB2AtomIds) {
  bool result = false;

  // Test size of fragments
  if (fragA1AtomIds.size() == fragB1AtomIds.size() &&
      fragA2AtomIds.size() == fragB2AtomIds.size()) {

    double interCentroidDistanceA = fabs(
        (centroidOfAtomIds(fragA1AtomIds) - centroidOfAtomIds(fragA2AtomIds))
            .length());
    double interCentroidDistanceB = fabs(
        (centroidOfAtomIds(fragB1AtomIds) - centroidOfAtomIds(fragB2AtomIds))
            .length());

    // Test inter-centroid distances
    if (fabs(interCentroidDistanceA - interCentroidDistanceB) <
        INTER_CENTROID_TOL) {

      QPair<QVector3D, QVector3D> minPositionsA =
          positionsOfMinDistanceAtomIdsAtomIds(fragA1AtomIds, fragA2AtomIds);
      double minDistanceA =
          (minPositionsA.first - minPositionsA.second).length();
      QPair<QVector3D, QVector3D> minPositionsB =
          positionsOfMinDistanceAtomIdsAtomIds(fragB1AtomIds, fragB2AtomIds);
      double minDistanceB =
          (minPositionsB.first - minPositionsB.second).length();

      // Test minimum distance
      if (fabs(minDistanceA - minDistanceB) < MIN_DISTANCE_TOL) {
        result = true;
      }
    }
  }

  return result;
}

QVector<int> DeprecatedCrystal::findPairs(int keyFragment) {
  QVector<int> fragmentIndices;

  for (int fragIndex = 0; fragIndex < numberOfFragments(); fragIndex++) {
    if (fragIndex == keyFragment) {
      continue;
    }
    fragmentIndices.append(fragIndex);
  }

  return fragmentIndices;
}

QVector<int> DeprecatedCrystal::findUniquePairs(int keyFragment) {
  QVector<FragmentPair> pairsToDo;
  QVector<int> fragmentIndices;

  for (int fragIndex = 0; fragIndex < numberOfFragments(); fragIndex++) {
    if (fragIndex == keyFragment) {
      continue;
    }

    FragmentPair fragPair = qMakePair(keyFragment, fragIndex);

    bool foundEquivalent = false;
    foreach (FragmentPair pp, pairsToDo) {
      if (pairsAreEquivalent(fragPair, pp)) {
        foundEquivalent = true;
        break;
      }
    }
    if (!foundEquivalent) {
      pairsToDo.append(fragPair);
      fragmentIndices.append(fragPair.second);
    }
  }

  return fragmentIndices;
}

QVector<int>
DeprecatedCrystal::findUniquePairsInvolvingCompleteFragments(int keyFragment) {
  Q_ASSERT(fragmentIsComplete(keyFragment));

  QVector<int> completeUniquePairs;

  foreach (int fragIndex, findUniquePairs(keyFragment)) {
    if (fragmentIsComplete(fragIndex)) {
      completeUniquePairs.append(fragIndex);
    }
  }
  return completeUniquePairs;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Measurements
//
///////////////////////////////////////////////////////////////////////////////////////////////////

QPair<QVector3D, QVector3D>
DeprecatedCrystal::positionsOfMinDistanceAtomIdsAtomIds(
    QVector<AtomId> atomIds1, QVector<AtomId> atomIds2) const {
  AtomId nearestAtomId1 = atomIds1[0];
  AtomId nearestAtomId2 = atomIds2[0];
  Atom nearestAtom1 = generateAtomFromIndexAndShift(
      nearestAtomId1.unitCellIndex, nearestAtomId1.shift);
  Atom nearestAtom2 = generateAtomFromIndexAndShift(
      nearestAtomId2.unitCellIndex, nearestAtomId2.shift);
  QVector3D nearestAtom1Pos = nearestAtom1.pos();
  QVector3D nearestAtom2Pos = nearestAtom2.pos();
  double minDistance = (nearestAtom1Pos - nearestAtom2Pos).lengthSquared();

  foreach (AtomId atomId1, atomIds1) {
    foreach (AtomId atomId2, atomIds2) {
      Atom atom1 =
          generateAtomFromIndexAndShift(atomId1.unitCellIndex, atomId1.shift);
      Atom atom2 =
          generateAtomFromIndexAndShift(atomId2.unitCellIndex, atomId2.shift);
      QVector3D pos1 = atom1.pos();
      QVector3D pos2 = atom2.pos();
      double distance = (pos1 - pos2).lengthSquared();
      if (distance < minDistance) {
        minDistance = distance;
        nearestAtom1Pos = pos1;
        nearestAtom2Pos = pos2;
      }
    }
  }

  return QPair<QVector3D, QVector3D>(nearestAtom1Pos, nearestAtom2Pos);
}

QPair<QVector3D, QVector3D>
DeprecatedCrystal::positionsOfMinDistanceFragFrag(int frag1, int frag2) const {
  if (m_atomsForFragment.size() > 0) {
    if (m_atomsForFragment[frag1].size() > 0 &&
        m_atomsForFragment[frag2].size() > 0) {
      int nearestAtom1 = m_atomsForFragment[frag1][0];
      int nearestAtom2 = m_atomsForFragment[frag2][0];
      double minDistance =
          (m_atoms[nearestAtom1].pos() - m_atoms[nearestAtom2].pos())
              .lengthSquared();
      foreach (int atom1, m_atomsForFragment[frag1]) {
        foreach (int atom2, m_atomsForFragment[frag2]) {
          double distance =
              (m_atoms[atom1].pos() - m_atoms[atom2].pos()).lengthSquared();
          if (distance < minDistance) {
            minDistance = distance;
            nearestAtom1 = atom1;
            nearestAtom2 = atom2;
          }
        }
      }
      return QPair<QVector3D, QVector3D>(m_atoms[nearestAtom1].pos(),
                                         m_atoms[nearestAtom2].pos());
    }
  }
  return QPair<QVector3D, QVector3D>(QVector3D(0, 0, 0), QVector3D(0, 0, 0));
}

QPair<QVector3D, QVector3D>
DeprecatedCrystal::positionsOfMinDistanceAtomFrag(int atom, int frag) const {
  return positionsOfMinDistancePosFrag(m_atoms[atom].pos(), frag);
}

QPair<QVector3D, QVector3D>
DeprecatedCrystal::positionsOfMinDistancePosFrag(QVector3D pos,
                                                 int frag) const {
  if (m_atomsForFragment.size() > 0) {
    if (m_atomsForFragment[frag].size() > 0) {
      int nearestAtom = m_atomsForFragment[frag][0];
      float minDistance = (pos - m_atoms[nearestAtom].pos()).lengthSquared();
      foreach (int atom2, m_atomsForFragment[frag]) {
        float distance = (pos - m_atoms[atom2].pos()).lengthSquared();
        if (minDistance > distance) {
          minDistance = distance;
          nearestAtom = atom2;
        }
      }
      return QPair<QVector3D, QVector3D>(pos, m_atoms[nearestAtom].pos());
    }
  }
  return QPair<QVector3D, QVector3D>(QVector3D(0, 0, 0), QVector3D(0, 0, 0));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Hydrogens, Hydrogen Bonding, Close Contacts
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void DeprecatedCrystal::makeListOfHydrogenDonors() {
  Q_ASSERT(m_bondedAtomsForAtom.size() > 0);

  _hydrogenDonors.clear();

  for (int i = 0; i < m_atoms.size(); ++i) {
    const Atom &atom = m_atoms[i];
    if (atom.isHydrogen() &&
        !atom.isContactAtom()) { // find non-contact atom hydrogens
      foreach (int donorIndex, m_bondedAtomsForAtom[i]) {
        QString symbol = m_atoms[donorIndex].symbol();
        if (!_hydrogenDonors.contains(symbol)) {
          _hydrogenDonors << symbol;
        }
      }
    }
  }
}

void DeprecatedCrystal::updateCloseContactWithIndex(int contactIndex, QString x,
                                                    QString y,
                                                    double distanceCriteria) {

  Q_ASSERT(contactIndex >= 0 && contactIndex <= CCMAX_INDEX);

  // Due to the way the ComboBoxes are initialised in
  // CloseContactDialog::updateDonorsAndAcceptors
  // by a stepwise clearing and populating of each combobox,
  // this routine does gets called multiple times
  // For some of those calls distanceCriteria does equal 0
  // but it doesn't seem to break the code below
  // because eventually the correct distanceCriteria are
  // passed here.
  // Q_ASSERT(distanceCriteria > 0);

  // Store the criteria for the close contact
  _closeContactsX[contactIndex] = x;
  _closeContactsY[contactIndex] = y;
  m_closeContactsDistanceCriteria[contactIndex] = distanceCriteria;

  // Clear previous close contacts
  _closeContactsTable[contactIndex].clear();

  // Update the close contacts table based on the new criteria
  for (const auto &vdwContact : m_vanDerWaalsContacts) {
    if (vdwContactPresent({vdwContact.from, vdwContact.to}, x, y) &&
        vdwContact.distance < distanceCriteria) {
      _closeContactsTable[contactIndex].push_back(
          {vdwContact.from, vdwContact.to});
    }
  }
}

///  Creates a list of all the hydrogen bonds (D-H•••A) between the atoms of
///  _atomList based on criteria:
///  'acceptor' is the element symbol for the acceptor atoms, can also be 'any'
///  'donor' is the element symbol for the donor atoms, can also be 'any'
///  'distanceCriteria' is based on:
///        Include D—H•••A bonds when the H•••A distance is shorter than the sum
///        of the Van der Waals radii by the 'distanceCriteria'
///  'includeIntraHBonds' determines whether we include intramolecular hydrogen
///  bonds
void DeprecatedCrystal::updateHBondList(QString donor, QString acceptor,
                                        double distanceCriteria,
                                        bool includeIntraHBonds) {
  // Clear previous list of hydrogen bonds
  _hbondList.clear();
  _hbondIntraFlag.clear();

  // Save the hbond parameters
  _hbondAcceptor = acceptor;
  _hbondDonor = donor;
  _hbondDistanceCriteria = distanceCriteria;
  _includeIntraHBonds = includeIntraHBonds;

  _hydrogenList.clear();
  _hydrogenList = hydrogensBondedToDonor(donor);

  // Update the close contacts table based on the new criteria
  for (const auto &vdwContact : m_vanDerWaalsContacts) {
    if (vdwContactPresent({vdwContact.from, vdwContact.to}, acceptor) &&
        (vdwContact.distance < (vdwContact.vdw_sum - distanceCriteria))) {
      _hbondList.push_back({vdwContact.from, vdwContact.to});
      _hbondIntraFlag.push_back(vdwContact.is_intramolecular);
    }
  }
}

QVector<int> DeprecatedCrystal::hydrogensBondedToDonor(QString donorToMatch) {
  Q_ASSERT(m_bondedAtomsForAtom.size() > 0);

  QVector<int> hydrogenList;

  for (int i = 0; i < m_atoms.size(); ++i) {
    Atom atom = m_atoms[i];
    if (atom.isHydrogen() &&
        !atom.isContactAtom()) { // find non-contact atom hydrogens
      foreach (int donorIndex, m_bondedAtomsForAtom[i]) {
        if (symbolsMatch(donorToMatch, donorIndex)) {
          hydrogenList << i;
        }
      }
    }
  }
  return hydrogenList;
}

bool DeprecatedCrystal::hasHydrogens() {
  return listOfElementSymbols().contains("H");
}

void DeprecatedCrystal::calculateCloseContactsTable() {
  for (int i = 0; i <= CCMAX_INDEX; ++i) {
    updateCloseContactWithIndex(i, _closeContactsX[i], _closeContactsY[i],
                                m_closeContactsDistanceCriteria[i]);
  }
}

bool DeprecatedCrystal::vdwContactPresent(QPair<int, int> vdwContact, QString x,
                                          QString y) {
  return (symbolsMatch(x, vdwContact.first) &&
          symbolsMatch(y, vdwContact.second)) ||
         (symbolsMatch(y, vdwContact.first) &&
          symbolsMatch(x, vdwContact.second));
}

bool DeprecatedCrystal::vdwContactPresent(QPair<int, int> vdwContact,
                                          QString acceptorToMatch) {
  // We don't know which way round the donor and acceptor are in the vdwContact
  // so
  // perform the test both ways round. Only one has to match hence the logical
  // or.
  return (matchesDonorCriteria(vdwContact.first) &&
          symbolsMatch(acceptorToMatch, vdwContact.second)) ||
         (matchesDonorCriteria(vdwContact.second) &&
          symbolsMatch(acceptorToMatch, vdwContact.first));
}

/// Gets called whenever the _VdwContactList changes (i.e. when new atoms are
/// added to the structure)
void DeprecatedCrystal::calculateHBondList() {
  updateHBondList(_hbondAcceptor, _hbondDonor, _hbondDistanceCriteria,
                  _includeIntraHBonds);
}

bool DeprecatedCrystal::matchesDonorCriteria(int hydrogenIndex) {
  return _hydrogenList.contains(hydrogenIndex);
}

/// Compares the symbol for the atom given by atomIndex
/// to the parameter 'symbolToMatch'
/// Always returns true if symbolToMatch equals ANY_ITEM i.e. "Any"
bool DeprecatedCrystal::symbolsMatch(QString symbolToMatch, int atomIndex) {
  if (symbolToMatch == ANY_ITEM) {
    return true;
  }
  QString symbol = m_atoms[atomIndex].symbol();
  return (symbolToMatch == symbol);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Formula Sum
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QString DeprecatedCrystal::formulaSumOfFragment(int fragIndex) {
  return formulaSumOfAtoms(atomsForFragment(fragIndex),
                           FORMULA_SUM_PLAIN_NUM_FMT);
}

QString
DeprecatedCrystal::formulaSumOfAtomIdsAsRichText(QVector<AtomId> atomIds) {
  QVector<Atom> atoms = generateAtomsFromAtomIds(atomIds);
  QString result = formulaSumOfAtoms(atoms, FORMULA_SUM_RICH_NUM_FMT);
  return result;
}

QString DeprecatedCrystal::formulaSumOfAtoms(const QVector<Atom> &atoms,
                                             QString numFormat) {
  QString formulaString = "";
  QMap<QString, int> formula;
  for (const auto &atom : atoms) {
    QString elementSymbol = atom.element()->capitalizedSymbol();
    if (formula.contains(elementSymbol)) {
      formula[elementSymbol] += 1;
    } else {
      formula[elementSymbol] = 1;
    }
  }

  int n;
  if (formula.contains("C")) {
    n = formula["C"];
    formula.remove("C");
    if (n == 1) {
      formulaString += "C";
    } else {
      formulaString += QString("C" + numFormat).arg(n);
    }
  }

  if (formula.contains("H")) {
    n = formula["H"];
    formula.remove("H");
    if (n == 1) {
      formulaString += "H";
    } else {
      formulaString += QString("H" + numFormat).arg(n);
    }
  }

  auto keys = formula.keys();
  std::sort(keys.begin(), keys.end());
  foreach (QString key, keys) {
    n = formula[key];
    if (n == 1) {
      formulaString += key;
    } else {
      formulaString += QString(key + numFormat).arg(n);
    }
  }

  return formulaString.trimmed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Fragment Patches
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DeprecatedCrystal::addFragmentPatchProperty(Surface *surface) {
  Q_ASSERT(surface->isParent());
  Q_ASSERT(surface->isHirshfeldBased());

  QVector<Atom> originalAtoms = m_atoms; // save current atoms
  m_atoms.clear(); // Can't call clearAtomList because it deletes the atoms
                   // (see above)

  // Add atoms and store indices for each face
  QVector<int> atomsAdded;
  QVector<int> atomIndexForFace;
  for (int f = 0; f < surface->numberOfFaces(); ++f) {
    int location = atomsAdded.indexOf(surface->outsideAtomIndexForFace(f));
    if (location == -1) {
      AtomId atom = surface->outsideAtomIdForFace(f);
      appendAtom(_unitCellAtomList[atom.unitCellIndex], atom.shift);
      atomsAdded.append(surface->outsideAtomIndexForFace(f));
      atomIndexForFace.append(atomsAdded.size() - 1);
    } else {
      atomIndexForFace.append(location);
    }
  }

  completeAllFragments();
  updateAtomListInfo();
  emit atomsChanged();

  // Get fragment index for each face
  QVector<float> fragmentForFace;
  for (int f = 0; f < surface->numberOfFaces(); ++f) {
    fragmentForFace.append(_fragmentForAtom[atomIndexForFace[f]]);
  }
  surface->addFaceProperty("fragment_patch", fragmentForFace);

  // Replace the atoms used to generate the fragment patch
  // with the original list of atoms
  m_atoms = originalAtoms;
  updateAtomListInfo();
  emit atomsChanged();
}

QVector<QColor> DeprecatedCrystal::fragmentPatchColors(Surface *surface) {
  Q_ASSERT(surface->isHirshfeldBased());

  return surface->colorsOfFragmentPatches();
}

QVector<double> DeprecatedCrystal::fragmentPatchAreas(Surface *surface) {
  Q_ASSERT(surface->isHirshfeldBased());

  return surface->areasOfFragmentPatches();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Charges
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Needs to be refactored
void DeprecatedCrystal::setUncharged() {
  for (int i = 0; i < unitCellAtoms().size(); ++i) {
    _fragmentChargeMultiplicityForUnitCellAtom.push_back({0, 1});
  }

  auto fragments = symmetryUniqueFragments();
  Q_FOREACH (auto fragment, fragments) {
    int multiplicity = guessMultiplicityForFragment(fragment);
    setChargeMultiplicityForFragment(fragment, {0, multiplicity});
    ;
  }
}

QVector<QVector<AtomId>> DeprecatedCrystal::symmetryUniqueFragments() {
  QVector<QVector<AtomId>> result;

  // Save current cluster
  QVector<AtomId> selectedAtomIds = selectedAtomsAsIds();
  QVector<Atom> allAtoms = m_atoms;

  m_atoms.clear();
  addAsymmetricAtomsToAtomList();
  completeAllFragments();
  updateAtomListInfo();

  QVector<int> fragmentIds(numberOfFragments());
  std::iota(fragmentIds.begin(), fragmentIds.end(), 0);
  result = atomIdsForFragments(fragmentIds);

  m_atoms = allAtoms;
  selectAtomsWithEquivalentAtomIds(selectedAtomIds);
  updateAtomListInfo();

  return result;
}

ChargeMultiplicityPair DeprecatedCrystal::chargeMultiplicityForFragment(
    const QVector<AtomId> &fragment) const {
  Q_ASSERT(fragment.size() > 0);

  // charge should be same for all atoms of fragment
  // therefore just take the first
  AtomId firstAtom = fragment.first();
  return _fragmentChargeMultiplicityForUnitCellAtom[firstAtom.unitCellIndex];
}

std::vector<ChargeMultiplicityPair>
DeprecatedCrystal::chargeMultiplicityForFragments(
    const QVector<QVector<AtomId>> &fragments) const {
  std::vector<ChargeMultiplicityPair> cm;

  if (noChargeMultiplicityInformation()) { // no existing charges therefore set
                                           // them all to
                                           // zero
    for (int i = 0; i < fragments.size(); ++i) {
      cm.push_back(
          {0, guessMultiplicityForFragment(
                  fragments[i])}); // TODO FIGURE OUT BETTER MULTIPLICITY keys
    }
  } else {
    foreach (QVector<AtomId> fragment, fragments) {
      cm.push_back(chargeMultiplicityForFragment(fragment));
    }
  }
  return cm;
}

void DeprecatedCrystal::setChargeMultiplicityForFragment(
    const QVector<AtomId> &fragment, const ChargeMultiplicityPair &cm) {
  foreach (AtomId atomId, fragment) {
    foreach (int unitCellIndex, symmetryRelatedUnitCellAtomsForUnitCellAtom(
                                    atomId.unitCellIndex)) {
      _fragmentChargeMultiplicityForUnitCellAtom[unitCellIndex] = cm;
    }
  }
}

int DeprecatedCrystal::guessMultiplicityForFragment(
    const QVector<AtomId> &fragment) const {
  // here we just guess the multiplicity based on the number of electrons
  int n_electrons = 0, multiplicity = 1;
  auto atoms = generateAtomsFromAtomIds(fragment);
  for (const auto &atom : atoms) {
    n_electrons += atom.element()->number();
  }
  // if we have an odd number of electrons, set multiplicity to default to 2
  if (n_electrons % 2 == 1)
    multiplicity = 2;
  return multiplicity;
}

void DeprecatedCrystal::setChargesMultiplicitiesForFragments(
    const QVector<QVector<AtomId>> &fragments,
    const std::vector<ChargeMultiplicityPair> &cm) {
  const int CHARGE_GUARD = -99;
  const int MULTIPLICITY_GUARD = -99;
  Q_ASSERT(fragments.size() == cm.size());

  // What if _fragmentChargeForUnitCellAtom is uninitialized
  int numUnitCellAtoms = unitCellAtoms().size();
  if (_fragmentChargeMultiplicityForUnitCellAtom.size() != numUnitCellAtoms) {
    _fragmentChargeMultiplicityForUnitCellAtom.clear();
    for (int i = 0; i < numUnitCellAtoms; ++i) {
      _fragmentChargeMultiplicityForUnitCellAtom.push_back(
          {CHARGE_GUARD, MULTIPLICITY_GUARD});
    }
  }

  for (int i = 0; i < fragments.size(); ++i) {
    setChargeMultiplicityForFragment(fragments[i], cm[i]);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// We only write out the minimum number of data members possible which keeps the
// amount of data as small as possible.
// When reading in we must therefore reinitialize various tables and lists. By
// using the initialization routines
// we can provide sufficient checks to ensure the Crystal is consistent and has
// everything it needs.
//
// The following are not written out:
//
// 1. Data that should be reinitialized e.g. connection tables
// 2. Wavefunction (because the checkpoint files associated with the
// wavefunction may no longer exist when the user reloads a project)
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QDataStream &operator<<(QDataStream &ds, const InteractionEnergy &energy) {
  QMap<EnergyType, double> energyMap = energy.first;
  QMap<int, double> outputtableEnergyMap;
  foreach (EnergyType e, energyMap.keys()) {
    outputtableEnergyMap.insert(static_cast<int>(e), energyMap[e]);
  }
  ds << outputtableEnergyMap;
  ds << energy.second;

  return ds;
}

QDataStream &operator>>(QDataStream &ds, InteractionEnergy &energy) {
  QMap<int, double> inputtableEnergyMap;
  ds >> inputtableEnergyMap;

  QMap<EnergyType, double> energyMap;
  foreach (int e, inputtableEnergyMap.keys()) {
    energyMap.insert(static_cast<EnergyType>(e), inputtableEnergyMap[e]);
  }
  energy.first = energyMap;
  ds >> energy.second;

  return ds;
}

QDataStream &operator<<(QDataStream &ds, const DeprecatedCrystal &crystal) {
  ds << crystal._unitCell;

  ds << crystal._unitCellAtomList;
  ds << crystal.m_atoms;

  ds << crystal._wavefunctions;

  ds << crystal._formula;
  ds << crystal._spaceGroup;
  ds << crystal._crystalName;
  ds << crystal._cifFilename;

  ds << crystal._origin[0];
  ds << crystal._origin[1];
  ds << crystal._origin[2];

  ds << crystal._asymmetricUnitIndicesAndShifts;

  ds << crystal._disorderGroups;

  write_stl_container(ds, crystal._symopsForUnitCellAtoms);

  ds << crystal.m_doNotBondList;
  ds << crystal.m_doBondList;

  ds << crystal.m_interactionEnergies;

  write_stl_container(ds, crystal._fragmentChargeMultiplicityForUnitCellAtom);
  return ds;
}

QDataStream &operator>>(QDataStream &ds, DeprecatedCrystal &crystal) {
  ds >> crystal._unitCell;

  ds >> crystal._unitCellAtomList;
  ds >> crystal.m_atoms;
  ds >> crystal._wavefunctions;

  ds >> crystal._formula;
  ds >> crystal._spaceGroup;
  ds >> crystal._crystalName;
  ds >> crystal._cifFilename;

  ds >> crystal._origin[0];
  ds >> crystal._origin[1];
  ds >> crystal._origin[2];

  ds >> crystal._asymmetricUnitIndicesAndShifts;

  ds >> crystal._disorderGroups;

  read_stl_container(ds, crystal._symopsForUnitCellAtoms);

  ds >> crystal.m_doNotBondList;
  ds >> crystal.m_doBondList;

  ds >> crystal.m_interactionEnergies;

  read_stl_container(ds, crystal._fragmentChargeMultiplicityForUnitCellAtom);

  // final calculations
  // TODO MOVE THIS TO SCENE IMPORT
  // crystal.rebuildSurfaceParentCloneRelationship();
  crystal.removeVdwContactAtoms();

  crystal.makeListOfElementSymbols(); // _elementSymbols
  crystal.makeSymopMappingTable();    // _symopMappingTable
  crystal.makeConnectionTables();     // _unitCellConnectionTable,
                                      // _vdwCellConnectionTable
  crystal.updateAtomListInfo();       // _atomsForBond, _fragmentForAtom,
                                      // _bondedAtomsForAtom, _bondsForAtom,
                                      // _atomsForFragment,
  // _vdwContactDistance, _vdwContactVdwSum, _vdwContactList,
  // _vdwContactIntraFlag,
  // _hbondList, _hbondIntraFlag, _hydrogenList, _closeContactsTable
  // _drawingQuality
  crystal.calculateRadius();          // _radius
  crystal.makeListOfHydrogenDonors(); // hydrogen donors

  crystal.rebuildEnergyTables(); // _sameTheoryDifferentEnergies,
                                 // _sameEnergyDifferentTheory
  // crystal.updateEnergyInfo(); // _energyInfos

  crystal.emit atomsChanged();

  return ds;
}
