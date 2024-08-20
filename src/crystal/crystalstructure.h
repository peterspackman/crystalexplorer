#pragma once
#include "chemicalstructure.h"
#include "crystalplane.h"
#include "dimer_graph.h"
#include "fragment_index.h"
#include <QHash>
#include <ankerl/unordered_dense.h>
#include <occ/crystal/crystal.h>
#include <occ/crystal/dimer_mapping_table.h>

using OccCrystal = occ::crystal::Crystal;

class CrystalStructure : public ChemicalStructure {
  Q_OBJECT;

public:
  CrystalStructure(QObject *parent = nullptr);

  // setters
  void setOccCrystal(const OccCrystal &);
  inline const auto &occCrystal() const { return m_crystal; }

  void
  setPairInteractionsFromDimerAtoms(const QList<QList<PairInteraction *>> &,
                                    const QList<QList<DimerAtoms>> &);

  virtual inline StructureType structureType() const override {
    return StructureType::Crystal;
  }

  inline occ::Mat3 cellVectors() const override {
    return m_crystal.unit_cell().direct();
  }

  inline occ::Vec3 cellAngles() const override {
    return m_crystal.unit_cell().angles();
  }

  inline occ::Vec3 cellLengths() const override {
    return m_crystal.unit_cell().lengths();
  }

  inline const auto &spaceGroup() const { return m_crystal.space_group(); }

  FragmentIndex fragmentIndexForAtom(int) const override;
  FragmentIndex fragmentIndexForAtom(GenericAtomIndex) const override;
  void deleteFragmentContainingAtomIndex(int atomIndex) override;

  const FragmentMap &getFragments() const override;
  void deleteIncompleteFragments() override;
  void deleteAtoms(const std::vector<GenericAtomIndex> &) override;

  std::vector<GenericAtomIndex>
      atomIndicesForFragment(FragmentIndex) const override;

  FragmentPairs
  findFragmentPairs(FragmentPairSettings settings = {}) const override;


  const std::pair<int, int> &atomsForBond(int) const override;
  std::vector<HBondTriple>
  hydrogenBonds(const HBondCriteria & = {}) const override;
  const std::vector<std::pair<int, int>> &covalentBonds() const override;
  std::vector<CloseContactPair>
  closeContacts(const CloseContactCriteria & = {}) const override;

  int genericIndexToIndex(const GenericAtomIndex &) const override;
  GenericAtomIndex indexToGenericIndex(int) const override;

  void updateBondGraph() override;

  void resetAtomsAndBonds(bool toSelection = false) override;

  void setShowVanDerWaalsContactAtoms(bool state) override;

  void completeFragmentContaining(int) override;
  void completeFragmentContaining(GenericAtomIndex) override;

  bool hasIncompleteFragments() const override;
  bool hasIncompleteSelectedFragments() const override;
  void completeAllFragments() override;
  void packUnitCells(const QPair<QVector3D, QVector3D> &) override;

  std::vector<FragmentIndex> completedFragments() const override;
  std::vector<FragmentIndex> selectedFragments() const override;

  QColor getFragmentColor(FragmentIndex) const override;
  void setFragmentColor(FragmentIndex, const QColor &) override;
  void setAllFragmentColors(const QColor &color) override;

  Fragment::State getSymmetryUniqueFragmentState(FragmentIndex) const override;
  void setSymmetryUniqueFragmentState(FragmentIndex, Fragment::State) override;

  const FragmentMap &symmetryUniqueFragments() const override;

  CellIndexSet occupiedCells() const override;

  void expandAtomsWithinRadius(float radius, bool selected) override;
  Fragment
  makeFragment(const std::vector<GenericAtomIndex> &idxs) const override;

  [[nodiscard]] std::vector<GenericAtomIndex>
  atomsWithFlags(const AtomFlags &flags, bool set = true) const override;
  [[nodiscard]] std::vector<GenericAtomIndex>
  atomsSurroundingAtoms(const std::vector<GenericAtomIndex> &,
                        float radius) const override;
  [[nodiscard]] std::vector<GenericAtomIndex>
  atomsSurroundingAtomsWithFlags(const AtomFlags &flags,
                                 float radius) const override;

  [[nodiscard]] occ::IVec
  atomicNumbersForIndices(const std::vector<GenericAtomIndex> &) const override;

  [[nodiscard]] std::vector<QString>
  labelsForIndices(const std::vector<GenericAtomIndex> &) const override;

  [[nodiscard]] occ::Mat3N atomicPositionsForIndices(
      const std::vector<GenericAtomIndex> &) const override;

  [[nodiscard]] QString chemicalFormula(bool richText = true) const override;

  std::vector<GenericAtomIndex> getAtomIndicesUnderTransformation(
      const std::vector<GenericAtomIndex> &idxs,
      const Eigen::Isometry3d &result) const override;

  bool getTransformation(const std::vector<GenericAtomIndex> &from_orig,
                         const std::vector<GenericAtomIndex> &to_orig,
                         Eigen::Isometry3d &result) const override;

  [[nodiscard]] std::vector<AtomicDisplacementParameters>
  atomicDisplacementParametersForAtoms(
      const std::vector<GenericAtomIndex> &) const override;
  [[nodiscard]] AtomicDisplacementParameters
      atomicDisplacementParameters(GenericAtomIndex) const override;

private:
  Fragment makeFragmentFromFragmentIndex(FragmentIndex);
  FragmentIndex findUnitCellFragment(const Fragment &frag) const;
  Fragment makeFragmentFromOccMolecule(const occ::core::Molecule &mol) const;

  void addAtomsByCrystalIndex(const std::vector<GenericAtomIndex> &,
                              const AtomFlags &flags = AtomFlag::NoFlag);
  void addVanDerWaalsContactAtoms();
  void removeVanDerWaalsContactAtoms();
  void deleteAtomsByOffset(const std::vector<int> &atomIndices);

  void buildDimerMappingTable(double maxRadius = 30.0);

  OccCrystal m_crystal;

  std::vector<GenericAtomIndex> m_unitCellOffsets;
  ankerl::unordered_dense::map<GenericAtomIndex, int, GenericAtomIndexHash>
      m_atomMap;
  ankerl::unordered_dense::map<int, AtomicDisplacementParameters>
      m_unitCellAdps;
  ankerl::unordered_dense::map<int, FragmentIndex> m_unitCellAtomFragments;

  FragmentMap m_fragments;
  std::vector<FragmentIndex> m_fragmentForAtom;
  FragmentMap m_symmetryUniqueFragments;
  FragmentMap m_unitCellFragments;

  std::vector<std::pair<int, int>> m_covalentBonds;
  std::vector<std::pair<int, int>> m_vdwContacts;
  std::vector<std::pair<int, int>> m_hydrogenBonds;

  occ::crystal::CrystalDimers m_unitCellDimers;
  occ::crystal::DimerMappingTable m_dimerMappingTable;
  occ::crystal::DimerMappingTable m_dimerMappingTableNoInv;
};
