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
  occ::Mat3N convertCoordinates(const occ::Mat3N &pos, ChemicalStructure::CoordinateConversion) const override;

  void deleteFragmentContainingAtomIndex(int atomIndex) override;
  void deleteIncompleteFragments() override;
  void deleteAtoms(const std::vector<GenericAtomIndex> &) override;

  FragmentPairs
  findFragmentPairs(FragmentPairSettings settings = {}) const override;

  int genericIndexToIndex(const GenericAtomIndex &) const override;
  GenericAtomIndex indexToGenericIndex(int) const override;

  void updateBondGraph() override;

  void resetAtomsAndBonds(bool toSelection = false) override;

  void setShowContacts(const ContactSettings &) override;

  void completeFragmentContaining(int) override;
  void completeFragmentContaining(GenericAtomIndex) override;

  bool hasIncompleteFragments() const override;
  bool hasIncompleteSelectedFragments() const override;
  void completeAllFragments() override;

  void buildSlab(SlabGenerationOptions) override;

  std::vector<FragmentIndex> completedFragments() const override;
  std::vector<FragmentIndex> selectedFragments() const override;

  QColor getFragmentColor(FragmentIndex) const override;
  void setFragmentColor(FragmentIndex, const QColor &) override;
  void setAllFragmentColors(const FragmentColorSettings &) override;

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

  [[nodiscard]] QString getTransformationString(const Eigen::Isometry3d &result) const;

  [[nodiscard]] nlohmann::json toJson() const override;
  bool fromJson(const nlohmann::json&) override;

private:
  Fragment makeFragmentFromFragmentIndex(FragmentIndex) const;
  FragmentIndex findUnitCellFragment(const Fragment &frag) const;
  Fragment makeFragmentFromOccMolecule(const occ::core::Molecule &mol, FragmentIndex);

  void addAtomsByCrystalIndex(const std::vector<GenericAtomIndex> &,
                              const AtomFlags &flags = AtomFlag::NoFlag);
  void addVanDerWaalsContactAtoms();
  void removeVanDerWaalsContactAtoms();
  void deleteAtomsByOffset(const std::vector<int> &atomIndices);

  void buildDimerMappingTable(double maxRadius = 12.0);

  OccCrystal m_crystal;

  std::vector<GenericAtomIndex> m_unitCellOffsets;
  ankerl::unordered_dense::map<GenericAtomIndex, int, GenericAtomIndexHash>
      m_atomMap;
  ankerl::unordered_dense::map<int, AtomicDisplacementParameters>
      m_unitCellAdps;
  ankerl::unordered_dense::map<int, FragmentIndex> m_unitCellAtomFragments;

  FragmentMap m_unitCellFragments;

  occ::crystal::CrystalDimers m_unitCellDimers;
  occ::crystal::DimerMappingTable m_dimerMappingTable;
  occ::crystal::DimerMappingTable m_dimerMappingTableNoInv;
  ContactSettings m_contactSettings;
};
