#pragma once
#include "chemicalstructure.h"
#include "crystalplane.h"
#include <QHash>
#include <ankerl/unordered_dense.h>
#include <occ/crystal/crystal.h>

using OccCrystal = occ::crystal::Crystal;

class CrystalStructure : public ChemicalStructure {
  Q_OBJECT;

public:
  CrystalStructure(QObject *parent = nullptr);

  // setters
  void setOccCrystal(const OccCrystal &);
  inline const auto &occCrystal() const { return m_crystal; }

  void setPairInteractionsFromDimerAtoms(const QList<QList<PairInteraction*>> &, const QList<QList<DimerAtoms>> &);

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

  int fragmentIndexForAtom(int) const override;
  int fragmentIndexForAtom(GenericAtomIndex) const override;
  void deleteFragmentContainingAtomIndex(int atomIndex) override;

  const std::vector<Fragment> &getFragments() const override;
  void deleteIncompleteFragments() override;
  void deleteAtoms(const std::vector<GenericAtomIndex> &) override;

  const std::vector<int> &atomsForFragment(int) const override;
  std::vector<GenericAtomIndex> atomIndicesForFragment(int) const override;

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

  std::vector<int> completedFragments() const override;
  std::vector<int> selectedFragments() const override;

  FragmentState getSymmetryUniqueFragmentState(int) const override;
  void setSymmetryUniqueFragmentState(int, FragmentState) override;

  const std::vector<Fragment> &symmetryUniqueFragments() const override;
  const std::vector<FragmentState> &
  symmetryUniqueFragmentStates() const override;


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

  bool getTransformation(
      const std::vector<GenericAtomIndex> &from_orig,
      const std::vector<GenericAtomIndex> &to_orig,
      Eigen::Isometry3d &result) const override;

  [[nodiscard]] std::vector<AtomicDisplacementParameters> atomicDisplacementParametersForAtoms(const std::vector<GenericAtomIndex> &) const override;
  [[nodiscard]] AtomicDisplacementParameters atomicDisplacementParameters(GenericAtomIndex) const override;


private:
  Fragment makeAsymFragment(const std::vector<GenericAtomIndex> &idxs) const;

  void addAtomsByCrystalIndex(std::vector<GenericAtomIndex> &,
                              const AtomFlags &flags = AtomFlag::NoFlag);
  void addVanDerWaalsContactAtoms();
  void removeVanDerWaalsContactAtoms();
  void deleteAtomsByOffset(const std::vector<int> &atomIndices);

  OccCrystal m_crystal;

  std::vector<GenericAtomIndex> m_unitCellOffsets;
  ankerl::unordered_dense::map<GenericAtomIndex, int, GenericAtomIndexHash>
      m_atomMap;
  ankerl::unordered_dense::map<int, AtomicDisplacementParameters> m_unitCellAdps;
  std::vector<Fragment> m_fragments;
  std::vector<GenericAtomIndex> m_fragmentUnitCellMolecules;
  std::vector<int> m_fragmentForAtom;
  std::vector<Fragment> m_symmetryUniqueFragments;
  std::vector<ChemicalStructure::FragmentState> m_symmetryUniqueFragmentStates;

  std::vector<std::pair<int, int>> m_covalentBonds;
  std::vector<std::pair<int, int>> m_vdwContacts;
  std::vector<std::pair<int, int>> m_hydrogenBonds;
};
