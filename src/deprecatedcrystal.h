#pragma once
#include <QColor>
#include <QList>
#include <QPair>
#include <QString>
#include <QVector3D>
#include <optional>

#include "atom.h"
#include "chargemultiplicitypair.h"
#include "crystalsurfacehandler.h"
#include "energydata.h"
#include "fragmentpairinfo.h"
#include "frameworkdescription.h"
#include "jobparameters.h"
#include "packingdialog.h" // access to enum UnitCellPackingCriteria
#include "qeigen.h"
#include "spacegroup.h"
#include "surface.h"
#include "transformablewavefunction.h"
#include "unitcell.h"
#include "vanderwaalscontact.h"
#include "wavefunction.h"
#include <optional>

// Same bonding criteria as used by the CCDC
// http://www.ccdc.cam.ac.uk/products/csd/radii/
// sum of cov radii - BONDING_TOLERANCE < bond length < sum of cov. radii +
// BONDING_TOLERANCE
const float BONDING_TOLERANCE = 0.4f; // angstroms

const int GLOBAL_MIN_NUM_BONDS_FOR_INTRA = 3;

enum DrawingQuality { veryLow, low, medium, high };

const int SMALL_CRYSTAL_LIMIT = 100;
const int MEDIUM_CRYSTAL_LIMIT = 300;
const int LARGE_CRYSTAL_LIMIT = 600;
const int HUGE_CRYSTAL_LIMIT = 900;

// Small amount added on to cell to ensure atoms on special positions
// (corners/edges of cell)
// are generated when completing a cell.
const double CELL_DELTA = 0.00001;

// When generating a cluster for the void we start with the unit cell and added
// some
// padding around the outside to ensure the void is correct inside the cell.
const double VOID_UNITCELL_PADDING = 5.0; // angstrom

// Used to decide whether two inter-centroid distance (between two pairs of
// fragments)
// are the same.
const double INTER_CENTROID_TOL = 0.0001;
const double MIN_DISTANCE_TOL = 0.0001;

const QString INFO_HORIZONTAL_RULE = QString(80, '-') + "\n";

const QString FORMULA_SUM_PLAIN_NUM_FMT = "%1 ";
const QString FORMULA_SUM_RICH_NUM_FMT = "<sub>%1</sub>";

enum ChargeStatus { unknown, uncharged, charged };

typedef QMultiMap<int, Shift> Connection;
typedef QVector<Connection> ConnectionTable;
typedef QPair<Shift, QVector<float>> ShiftAndLimits;
using CrystalSymops = QMap<SymopId, Vector3q>;
typedef QPair<QMap<EnergyType, double>, JobParameters> InteractionEnergy;
typedef QPair<int, int> FragmentPair;
typedef QPair<QVector<AtomId>, int> ChargedFragment;

using ContactsList = QVector<QPair<int, int>>;

class DeprecatedCrystal : public QObject {
  Q_OBJECT

  friend QDataStream &operator<<(QDataStream &ds,
                                 const DeprecatedCrystal &crystal);
  friend QDataStream &operator>>(QDataStream &, DeprecatedCrystal &);
  friend class CrystalSurfaceHandler;

public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  DeprecatedCrystal();
  ~DeprecatedCrystal();

  // Initialization
  void setCrystalCell(QString, QString, float, float, float, float, float,
                      float);
  void setUnitCellAtoms(const QVector<Atom> &);
  void setSymopsForUnitCellAtoms(const std::vector<int> &);
  void setAsymmetricUnitIndicesAndShifts(const QMap<int, Shift> &);
  void setCrystalName(QString);
  void setCifFilename(QString cifFilename) { _cifFilename = cifFilename; }
  void postReadingInit();
  void calculateUnitCellCartesianCoordinates();

  // General Crystal Information
  inline const QString &crystalName() const { return _crystalName; }
  inline const QString &cifFilename() const { return _cifFilename; }
  inline const QString &formula() const { return _formula; }

  // Atoms, Bonding, Packing and Cluster Generation
  void expandAtomsBondedToAtom(const Atom &);
  void addAsymmetricAtomsToAtomList();
  void resetToAsymmetricUnit();
  void expandAtomsWithinRadius(float);
  void expandAtomsWithinRadiusOfSelectedAtoms(float);
  void packUnitCells(const QVector3D &);
  void packMultipleCells(const QVector3D &, const QVector3D &);
  void appendAtom(const Atom &, const Shift &);
  void setAtomListToBufferedUnitCellAtomList(const QVector3D &,
                                             const QVector3D &);
  void updateForChangeInAtomConnectivity();

  QPair<QVector3D, QVector3D> fractionalPackingLimitsFromPadding(float);

  const QVector<Atom> &atoms() const { return m_atoms; }
  QVector<Atom> &atoms() { return m_atoms; }
  const QVector<Atom> &unitCellAtoms() const { return _unitCellAtomList; }
  QVector<Atom> &unitCellAtoms() { return _unitCellAtomList; }

  QVector<Atom> bufferedUnitCellAtomList(const QPair<QVector3D, QVector3D> &);
  QVector<Atom> bufferedAtomList(const QVector<Atom> &,
                                 const QPair<QVector3D, QVector3D> &boxLimits,
                                 bool withinBox = true,
                                 bool includeCellBoundaryAtoms = true);
  QVector<Atom> generateAtomsFromAtomIds(const QVector<AtomId> &) const;
  QVector<Atom> packedUnitCellsAtomList(const QPair<QVector3D, QVector3D> &,
                                        bool calcConnectivity = true);
  QVector<AtomId> voidCluster(const float);

  bool areCovalentBondedAtoms(const Atom &, const Atom &);
  bool areCovalentBondedAtomsByDistanceCriteria(const Atom &, const Atom &);
  bool
  positionIsWithinBoxCentredAtZero(QVector3D pos,
                                   const QPair<QVector3D, QVector3D> &boxLimits,
                                   QVector3D zero = QVector3D(0, 0, 0),
                                   bool includeBoxBoundaryPositions = true);
  bool anyAtomHasAdp() const;
  bool isCompleteCellFromLimits(QVector<float>);
  bool atomLocatedInPartialCell(Atom *, QVector<float>);

  void colorSelectedAtoms(QColor);
  void resetAllAtomColors();
  bool hasAtomsWithCustomColor() const;
  void translateOrigin(const QVector3D &);

  // Fragments
  void toggleFragmentColors();
  void colorFragmentsByEnergyPair();
  void clearFragmentColors();
  void completeFragmentContainingAtomIndex(int);
  void deleteFragmentContainingAtomIndex(int);
  void discardIncompleteFragments();
  void completeAllFragments();
  void completeSelectedFragments();

  bool hasIncompleteFragments();
  bool hasIncompleteSelectedFragments();
  bool fragmentIsComplete(int);
  bool fragmentAtomsAreSymmetryRelated(const QVector<AtomId> &,
                                       const QVector<AtomId> &) const;
  bool fragmentsAreSymmetryRelated(int, int);
  bool moreThanOneSymmetryUniqueFragment();

  QVector3D centroidOfFragment(int);
  QVector3D centroidOfAtomIds(QVector<AtomId>);
  QVector3D centerOfMassOfAtomIds(QVector<AtomId>);
  QVector<QVector3D> centroidsOfFragments();

  int numberOfFragments() const;
  int fragmentIndexOfFirstSelectedFragment();
  int fragmentIndexOfSecondSelectedFragment();
  int keyFragment();
  int numberOfCompleteFragments();
  inline int fragmentIndexForAtomIndex(int atomIndex) const {
    return _fragmentForAtom[atomIndex];
  }

  QVector<Atom> atomsForFragment(int, bool skipContactAtoms = true) const;
  QVector<AtomId> atomIdsForFragment(int, bool skipContactAtoms = true) const;
  QVector<QVector<AtomId>>
  atomIdsForFragments(const QVector<int> &, bool skipContactAtoms = true) const;
  QVector<int> fragmentIndices() const {
    QVector<int> vals(numberOfFragments());
    std::iota(vals.begin(), vals.end(), 0);
    return vals;
  }

  QVector<int> fragmentIndicesOfCompleteFragments();

  // Bond Modification
  void bondSelectedAtoms();
  void unbondSelectedAtoms();
  void resetBondingModifications();

  // Disorder
  bool isDisordered();

  // Visibility
  bool hasVisibleAtoms() const;
  bool hasHiddenAtoms() const;
  QVector<Atom> visibleAtoms();

  // Selections
  void setSelectStatusForAllAtoms(bool);
  void discardSelectedAtoms();
  void selectAllAtoms();
  void unselectAllAtoms();
  void selectAllSuppressedAtoms();
  void selectAtomsInsideSurface(Surface *);
  void selectAtomsOutsideSurface(Surface *);
  void selectFragmentContaining(int);
  void selectAtomsWithEquivalentAtomIds(QVector<AtomId>);

  QVector<int> selectedAtomIndices();
  QVector<AtomId> selectedAtomsAsIds();
  QVector<AtomId> selectedAtomsAsIdsOrderedByFragment();

  bool hasSelectedAtoms() const;
  bool hasAllAtomsSelected();
  bool fragmentContainingAtomIndexIsSelected(int);
  bool fragmentIsSelected(int);
  int numberOfSelectedFragments();
  void invertSelection();

  // Symmetry
  SymopId symopIdForFragment(int);
  Vector3q cellTranslation(Shift);
  QString calculateFragmentPairIdentifier(QVector<AtomId>, QVector<AtomId>);

  // Elements
  void makeListOfElementSymbols();
  QStringList listOfElementSymbols();

  // Unitcell, Radius and Origin
  void resetOrigin();
  void setOrigin(Vector3q);
  void calculateRadius();
  const UnitCell &unitCell() const { return _unitCell; }
  float radius() const { return _radius; }
  Vector3q origin() const { return _origin; }
  Vector3q aAxis() const { return _unitCell.aAxis(); }
  Vector3q bAxis() const { return _unitCell.bAxis(); }
  Vector3q cAxis() const { return _unitCell.cAxis(); }

  // Contact Atoms
  void showVdwContactAtoms(bool);
  void removeVdwContactAtoms();
  bool hasAnyVdwContactAtoms() const;

  // Atom Suppression
  bool hasSuppressedAtoms() const;
  void suppressSelectedAtoms();
  void unsuppressSelectedAtoms();
  void unsuppressAllAtoms();
  QVector<int> suppressedAtomsAsUnitCellAtomIndices();
  QSet<int> symmetryRelatedAtomsForUnitCellAtomIndices(QSet<int>);

  // Wavefunctions
  void addWavefunction(const Wavefunction &);
  void addMonomerEnergy(const MonomerEnergy &);
  void replaceExistingWavefunction(const Wavefunction &);
  QVector<TransformableWavefunction>
  transformableWavefunctionForCurrentSelection();
  QVector<TransformableWavefunction> transformableWavefunctionsForFragment(int);
  QVector<TransformableWavefunction>
  transformableWavefunctionsForAtoms(const QVector<AtomId> &);
  QVector<QPair<TransformableWavefunction, TransformableWavefunction>>
  transformableWavefunctionsForFragmentAtoms(const QVector<AtomId> &,
                                             const QVector<AtomId> &);
  std::optional<Wavefunction>
  wavefunctionMatchingParameters(const JobParameters &);
  std::optional<MonomerEnergy>
  monomerEnergyMatchingParameters(const JobParameters &);

  inline int numberOfAtoms() const { return m_atoms.size(); }
  inline int numberOfBonds() const { return _atomsForBond.size(); }

  const Wavefunction &lastWavefunction() {
    Q_ASSERT(_wavefunctions.size() > 0);
    return _wavefunctions.back();
  }

  const auto &wavefunctions() const { return _wavefunctions; }

  // Interaction Energies
  inline const auto &interactionEnergies() const {
    return m_interactionEnergies;
  }
  inline const auto &sameEnergyDifferentTheory() const {
    return m_sameEnergyDifferentTheory;
  }
  inline const auto &sameTheoryDifferentEnergies() const {
    return m_sameTheoryDifferentEnergies;
  }

  void addInteractionEnergyData(QMap<EnergyType, double>,
                                const JobParameters &);
  bool hasInteractionEnergies();
  bool haveInteractionEnergyForPairInJobParameters(const JobParameters &);
  int indexOfInteractionEnergyForFragments(int, int);
  int indexOfInteractionEnergyForAtomIds(QVector<AtomId>, QVector<AtomId>);
  int indexOfInteractionEnergyForFragmentsWithEnergyTheoryComparison(
      int, int, EnergyTheory);
  int indexOfInteractionEnergyForAtomIdsWithEnergyTheoryComparison(
      QVector<AtomId>, QVector<AtomId>, EnergyTheory);
  int indexOfInteractionEnergyForFragmentsWithWavefunctionComparison(
      int, int, const JobParameters &);
  int indexOfInteractionEnergyForAtomIdsWithWavefunctionComparison(
      QVector<AtomId>, QVector<AtomId>, const JobParameters &);
  QVector<QColor> interactionEnergyColors();
  QColor energyColorForPair(int, int);
  QVector<SymopId> interactionEnergySymops();
  QVector<double> interactionEnergyDistances();
  void updateEnergyInfo(FrameworkType);
  EnergyModel energyModelFromJobParameters(const JobParameters &);
  float coulombFactor(InteractionEnergy);
  float polarizationFactor(InteractionEnergy);
  float dispersionFactor(InteractionEnergy);
  float repulsionFactor(InteractionEnergy);
  void updateTotalEnergy(int);
  QVector<QString> levelsOfTheoriesForLatticeEnergies();
  QVector<float> latticeEnergies();
  bool energyIsBenchmarked(InteractionEnergy);
  QVector<bool> interactionEnergyBenchmarkedEnergyStatuses();
  QVector<EnergyTheory> energyTheories();
  QMap<int, int> interactionEnergyFragmentCount();
  void setEnergyTheoryForEnergyFramework(EnergyTheory theory,
                                         FrameworkType framework) {
    m_energyTheory = theory;
    updateEnergyInfo(framework);
  }

  // Equivalent Pairs
  double interCentroidDistance(FragmentPair);
  bool pairsAreEquivalent(FragmentPair, FragmentPair);
  bool pairsAreEquivalent(QVector<AtomId>, QVector<AtomId>, QVector<AtomId>,
                          QVector<AtomId>);
  QVector<int> findPairs(int);
  QVector<int> findUniquePairs(int);
  QVector<int> findUniquePairsInvolvingCompleteFragments(int);

  QPair<QVector3D, QVector3D>
      positionsOfMinDistanceAtomIdsAtomIds(QVector<AtomId>,
                                           QVector<AtomId>) const;
  QPair<QVector3D, QVector3D> positionsOfMinDistanceFragFrag(int frag1,
                                                             int frag2) const;
  QPair<QVector3D, QVector3D> positionsOfMinDistanceAtomFrag(int atom,
                                                             int frag) const;
  QPair<QVector3D, QVector3D> positionsOfMinDistancePosFrag(QVector3D pos,
                                                            int frag) const;

  // Spacegroup
  const SpaceGroup &spaceGroup() const { return _spaceGroup; }
  SpaceGroup &spaceGroup() { return _spaceGroup; }

  // See note in DrawableCrystal "Surface Picking Names" as to why we have to
  // regenerate all the lists.

  // Hydrogens, Hydrogen Bonds, Close Contacts
  void makeListOfHydrogenDonors();
  void updateCloseContactWithIndex(int, QString, QString, double);
  void updateHBondList(QString, QString, double, bool);
  QVector<int> hydrogensBondedToDonor(QString);
  QStringList listOfHydrogenDonors() { return _hydrogenDonors; }
  bool hasHydrogens();

  // Formula Sum
  QString formulaSumOfFragment(int);
  QString formulaSumOfAtomIdsAsRichText(QVector<AtomId>);

  // Colors
  ColorScheme _selectedFragmentColorScheme = ColorScheme::Viridis;
  void colorAllAtoms(QColor);
  void colorAtomsByFragment(QVector<int>);

  // Fragment Patches
  void addFragmentPatchProperty(Surface *);
  QVector<QColor> fragmentPatchColors(Surface *);
  QVector<double> fragmentPatchAreas(Surface *);

  // Charges
  bool noChargeMultiplicityInformation() const {
    return _fragmentChargeMultiplicityForUnitCellAtom.size() == 0;
  }
  bool hasChargeMultiplicityInformation() {
    return !noChargeMultiplicityInformation();
  }
  void setUncharged();
  int guessMultiplicityForFragment(const QVector<AtomId> &) const;
  QVector<QVector<AtomId>> symmetryUniqueFragments();
  ChargeMultiplicityPair
  chargeMultiplicityForFragment(const QVector<AtomId> &) const;
  std::vector<ChargeMultiplicityPair> chargeMultiplicityForFragments(
      const QVector<QVector<AtomId>> &fragments) const;
  void setChargeMultiplicityForFragment(const QVector<AtomId> &,
                                        const ChargeMultiplicityPair &);
  void setChargesMultiplicitiesForFragments(
      const QVector<QVector<AtomId>> &,
      const std::vector<ChargeMultiplicityPair> &);

  // TODO sort these out rather than invasive requests
  inline auto &energyInfos() { return _energyInfos; }
  inline auto &atomIdsForFragmentsRef() { return m_atomsForFragment; }
  inline auto &hbondList() { return _hbondList; }
  inline auto &intramolecularHbondFlags() { return _hbondIntraFlag; }
  inline auto &fragmentForAtom() { return _fragmentForAtom; }
  inline auto &closeContactsTable() { return _closeContactsTable; }
  inline const auto &atomsForBond() const { return _atomsForBond; }
  inline auto &atomsForBond() { return _atomsForBond; }
  inline const auto &disorderGroups() const { return _disorderGroups; }
  inline bool includeIntramolecularHBonds() const {
    return _includeIntraHBonds;
  }

  // Selections
  void setSelectStatusForFragment(int, bool);
  Atom generateAtomFromIndexAndShift(int, const Shift &) const;

  // utility
  CrystalSymops calculateCrystalSymops(const Surface *, int) const;
  void removeLastAtoms(int);

signals:
  void atomsChanged();
  void surfacesChanged();

protected:
  // Atoms, Bonding, Packing and Cluster Generation
  bool hasCovalentBondedAtoms(int, int);

  // Although _drawingQualtity is "drawing" parameter and therefore should be in
  // DrawableCrystal
  // if we want to set it immediately upon reading crystaldata
  // (setDrawingQualityFromClusterSize)
  // it must be defined here
  DrawingQuality _drawingQuality;

  UnitCell _unitCell;
  QVector<Atom> _unitCellAtomList;
  QVector<Atom> m_atoms;
  QVector<QPair<int, int>> _atomsForBond;
  QVector<int> _fragmentForAtom;
  QVector<QVector<int>> m_atomsForFragment;
  QVector<FragmentPairInfo> _energyInfos;

  QVector<int> _disorderGroups;

  bool _includeIntraHBonds;
  QVector<QPair<int, int>> _hbondList;
  QVector<bool> _hbondIntraFlag;
  QVector<ContactsList> _closeContactsTable;

private:
  // Initalization
  void init();
  void makeListOfDisorderGroups();
  void makeSymopMappingTable();
  void updateAtomListInfo();
  void clearUnitCellAtomList();

  // Atom List Connectivity
  void updateConnectivityInfo();
  void calculateConnectivityInfo(const QVector<Atom> &);
  void calculateCovalentBondInfo(const QVector<Atom> &);
  void calculateFragmentInfo(const QVector<Atom> &);

  // Connection Tables
  void makeConnectionTables();
  void makeUnitCellConnectionTableAlt();
  void makeVdwCellConnectionTableAlt();
  void makeUnitCellConnectionTable();
  void makeVdwCellConnectionTable();
  bool areVdwBondedAtoms(const Atom &, const Atom &,
                         const float closeContactTolerance = 0.0f);

  // Atoms, Bonding, Packing and Cluster Generation
  int numberOfCovalentBondedAtomsBetweenAtoms(int i, int j);
  void clearAtomList();
  void appendUniqueAtomsOnly(const QVector<Atom> &);
  bool hasAllAtomsBondedToAtom(const Atom &);
  void appendConnectionsToAtom(const Atom &, ConnectionTable, QVector<Atom> &,
                               QVector<int> shiftLimits = QVector<int>(),
                               bool addContactAtoms = false);
  bool hasAtom(const Atom &);
  QVector<int> shiftLimits(const QVector<Atom> &);
  QVector<int> shiftLimitsForFragmentContainingAtom(const Atom &);
  bool shiftWithinPlusOneOfLimits(const Shift &, const QVector<int> &) const;
  bool isZeroShift(const Shift &) const;
  bool shiftListContainsShift(const QVector<Shift> &, const Shift &) const;
  QVector<Atom> removeSelectedAtoms();
  QVector<Atom> generateAtomsFromShifts(QVector<Shift>);
  void keepClusterAtomsWithinRadiusOfSelectedAtoms(const QVector<Atom> &,
                                                   const QVector<Atom> &,
                                                   float);
  void reinstateAtoms(const QVector<Atom> &);
  int unitCellAtomIndexOf(AtomId, SymopId, Vector3q);

  // Fragments
  int generateFragmentFromAtomIdAssociatedWithASurface(Surface *, const AtomId);
  QColor fragmentColor(int, bool skipFragment0 = false);
  int firstFragmentWithSelectedAtom();
  QVector<int> fragmentIndicesOfSelection();
  bool fragmentWithIndexExists(int) const;
  int fragmentIndexOfSelectedFragmentAtOrdinal(int);
  bool hasSameFragmentAtoms(int, int);

  // Bond Modification
  bool doNotBond(const Atom &, const Atom &, bool);
  bool doBond(const Atom &, const Atom &, bool);
  bool removeFromBondingList(int, int);
  bool removeFromNonBondingList(int, int);
  void addUniqueAtomPairsToBondingList(const QVector<int> &atomIds);
  void addUniqueAtomPairsToNonBondingList(const QVector<int> &atomIds);
  bool pairInList(int atom_i_index, int atom_j_index,
                  const QVector<QPair<int, int>> &listOfPairs);

  // Symmetry
  QVector<int> symmetryRelatedUnitCellAtomsForUnitCellAtom(int);
  CrystalSymops calculateCrystalSymops(int, int) const;
  CrystalSymops calculateCrystalSymops(const QVector<AtomId> &,
                                       const QVector<AtomId> &) const;
  SymopId symopIdForUnitCellAtoms(int, int) const;
  Vector3q calculateShift(const AtomId &, const AtomId &,
                          const SymopId &) const;
  Vector3q vectorFromShift(const Shift &) const;
  bool isSameShift(const Vector3q &, const Vector3q &) const;
  QVector<Vector3q> cellShiftsFromCellLimits(const QVector3D &) const;
  QVector<Shift> getCellShifts(const QVector<Atom> &, float) const;
  QVector<Shift> shiftsWithinRadiusOfReferenceShift(const Shift &, float) const;

  // Contact Atoms
  void calculateVdwContactInfo();
  bool isSuitableIntraCloseContact(int, int);
  void clearVdwContactInfo();
  void addVdwContact(const VanDerWaalsContact &contact);
  void appendVdwContactAtoms();

  // Wavefunctions
  std::optional<TransformableWavefunction>
  transformableWavefunctionForAtomsFromWavefunction(const Wavefunction &,
                                                    const QVector<AtomId> &);
  int indexOfWavefunctionMatchingParameters(const JobParameters &);
  bool sameWavefunctionInJobParameters(const JobParameters &,
                                       const JobParameters &);

  // Wavefunctions
  std::optional<MonomerEnergy>
  monomerEnergyForAtomsFromMonomerEnergy(const MonomerEnergy &,
                                         const QVector<AtomId> &);
  int indexOfMonomerEnergymatchingParams(const JobParameters &);

  // Interaction Energies
  bool sameLevelOfTheory(InteractionEnergy, InteractionEnergy);
  bool energyForSamePair(InteractionEnergy, InteractionEnergy);
  void rebuildEnergyTables();
  void updateEnergyTables(int);
  QString energyComponentAsString(int, EnergyType);
  void clearEnergyInfos();
  double energyForEnergyType(int, EnergyType);
  double energyCutoffForEnergyFramework(FrameworkType);

  // Hydrogens, Hydrogen Bonding, Close Contacts
  void calculateCloseContactsTable();
  bool vdwContactPresent(QPair<int, int>, QString, QString);
  bool vdwContactPresent(QPair<int, int>, QString);
  void calculateHBondList();
  bool matchesDonorCriteria(int);
  bool symbolsMatch(QString, int);
  void update_unit_cell_connectivity();

  // Formula Sum
  QString formulaSumOfAtoms(const QVector<Atom> &, QString);

  QString _formula;
  SpaceGroup _spaceGroup;
  QString _crystalName;
  QString _cifFilename;
  Vector3q _origin;
  float _radius;
  bool m_isPeriodic{true};

  /// An atom index (key) mapped to a list of atom indices which are
  /// bonded to that atom index (value)
  std::vector<std::vector<int>> m_bondedAtomsForAtom;

  std::vector<std::vector<int>> m_bondsForAtom;

  /// The unit cell atom index for an asymmetric atom (key) mapped to a
  /// corresponding shift in fractional coordinates for that
  /// asymmetric unit atom (value). Note: currently, all symmetry
  /// related atoms in the unit cell are consecutively listed after
  /// the unique asymmetric unit atom.
  QMultiMap<int, Shift> _asymmetricUnitIndicesAndShifts;

  /// Each element in the list is a "connection" from a unit cell
  /// atom "u" to a map describing the corresponding atoms in the unit cell
  /// "u1", "u2" ... connected to "u" (keys), with their corresponding
  /// fraction atom shifts "s1", s2" ... (values). Van der waals connections
  /// do not include connections which are covalently bonded.
  ConnectionTable _unitCellConnectionTable;
  ConnectionTable _vdwCellConnectionTable;

  std::vector<int> _symopsForUnitCellAtoms;
  // Returns the symop that maps
  // unit cell atom i to unit cell atom j.
  MatrixXq m_symopMappingTable;

  QStringList _elementSymbols;
  QStringList _hydrogenDonors;

  // Wavefunction
  QVector<Wavefunction> _wavefunctions;
  QVector<MonomerEnergy> m_monomerEnergies;

  float _covalentCutOff;
  float _vdwCutOff;

  std::vector<VanDerWaalsContact> m_vanDerWaalsContacts;
  QVector<int> _hydrogenList;
  QString _hbondDonor;
  QString _hbondAcceptor;
  double _hbondDistanceCriteria;
  QStringList _closeContactsX;
  QStringList _closeContactsY;
  QVector<double> m_closeContactsDistanceCriteria{GLOBAL_CC_DISTANCE_CRITERIA,
                                                  GLOBAL_CC_DISTANCE_CRITERIA,
                                                  GLOBAL_CC_DISTANCE_CRITERIA};

  QVector<QPair<int, int>> m_doNotBondList;
  QVector<QPair<int, int>> m_doBondList;

  QVector<InteractionEnergy> m_interactionEnergies;
  QVector<QVector<int>> m_sameTheoryDifferentEnergies;
  QVector<QVector<int>> m_sameEnergyDifferentTheory;

  EnergyTheory m_energyTheory; // Used by Energy Frameworks

  // Charges
  std::vector<ChargeMultiplicityPair>
      _fragmentChargeMultiplicityForUnitCellAtom;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &, const InteractionEnergy &);
QDataStream &operator>>(QDataStream &, InteractionEnergy &);
QDataStream &operator<<(QDataStream &, const DeprecatedCrystal &);
QDataStream &operator>>(QDataStream &, DeprecatedCrystal &);
