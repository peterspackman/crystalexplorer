#pragma once
#include "chemicalstructure.h"
#include "crystalplane.h"
#include <QHash>
#include <ankerl/unordered_dense.h>
#include <occ/crystal/crystal.h>

using OccCrystal = occ::crystal::Crystal;

struct CrystalIndex {
  int unitCellOffset{0};
  MillerIndex hkl;

  inline bool operator==(const CrystalIndex &other) const {
    return std::tie(unitCellOffset, hkl.h, hkl.k, hkl.l) ==
           std::tie(other.unitCellOffset, other.hkl.h, other.hkl.k,
                    other.hkl.l);
  }
  inline bool operator<(const CrystalIndex &other) const {
    return std::tie(unitCellOffset, hkl.h, hkl.k, hkl.l) <
           std::tie(other.unitCellOffset, other.hkl.h, other.hkl.k,
                    other.hkl.l);
  }
  inline bool operator>(const CrystalIndex &other) const {
    return std::tie(unitCellOffset, hkl.h, hkl.k, hkl.l) >
           std::tie(other.unitCellOffset, other.hkl.h, other.hkl.k,
                    other.hkl.l);
  }

};

struct CrystalIndexHash {
    using is_avalanching = void;
    [[nodiscard]] auto operator()(CrystalIndex const &idx) const noexcept -> uint64_t {
	static_assert(std::has_unique_object_representations_v<CrystalIndex>);
	return ankerl::unordered_dense::detail::wyhash::hash(&idx, sizeof(idx));
    }
};

using CrystalIndexSet = ankerl::unordered_dense::set<CrystalIndex, CrystalIndexHash>;

class CrystalStructure : public ChemicalStructure {
  Q_OBJECT;

public:
  CrystalStructure(QObject *parent = nullptr);

  // setters
  void setOccCrystal(const OccCrystal &);
  inline const auto &occCrystal() const { return m_crystal; }

  inline void setName(const QString &name) { m_name = name; }
  inline const auto &name() const { return m_name; }

  inline void setFilename(const QString &filename) { m_filename = filename; }
  inline const auto &filename() const { return m_name; }

  QString chemicalFormula() const;

  virtual inline StructureType structureType() const override {
    return StructureType::Crystal;
  }
  virtual inline occ::Mat3 cellVectors() const override {
    return m_crystal.unit_cell().direct();
  }

  virtual int fragmentIndexForAtom(int) const override;
  virtual void deleteFragmentContainingAtomIndex(int atomIndex) override;
  virtual void deleteIncompleteFragments() override;



  virtual const std::vector<int> &atomsForFragment(int) const override;
  virtual std::vector<GenericAtomIndex> atomIndicesForFragment(int) const override;

  virtual const std::pair<int, int> &atomsForBond(int) const override;
  virtual const std::vector<std::pair<int, int>> &
  hydrogenBonds() const override;
  virtual const std::vector<std::pair<int, int>> &
  covalentBonds() const override;
  virtual const std::vector<std::pair<int, int>> &vdwContacts() const override;

  virtual void updateBondGraph() override;

  virtual void resetAtomsAndBonds(bool toSelection = false) override;

  virtual void setShowVanDerWaalsContactAtoms(bool state) override;
  virtual void completeFragmentContaining(int) override;
  virtual bool hasIncompleteFragments() const override;
  virtual void completeAllFragments() override;
  virtual void packUnitCells(const QPair<QVector3D, QVector3D> &) override;

  virtual std::vector<int> completedFragments() const override;

  virtual FragmentState getSymmetryUniqueFragmentState(int) const override;
  virtual void setSymmetryUniqueFragmentState(int, FragmentState) override;

  virtual const std::vector<std::vector<GenericAtomIndex>> &symmetryUniqueFragments() const override;
  virtual const std::vector<FragmentState> &symmetryUniqueFragmentStates() const override;

  virtual void expandAtomsWithinRadius(float radius, bool selected) override;

  [[nodiscard]] virtual std::vector<GenericAtomIndex> atomsWithFlags(const AtomFlags &flags) const override;
  [[nodiscard]] virtual std::vector<GenericAtomIndex> atomsSurroundingAtomsWithFlags(const AtomFlags &flags, float radius) const override;


  [[nodiscard]] virtual occ::IVec atomicNumbersForIndices(const std::vector<GenericAtomIndex> &) const override;

  [[nodiscard]] virtual occ::Mat3N atomicPositionsForIndices(const std::vector<GenericAtomIndex> &) const override;

private:
  void addAtomsByCrystalIndex(std::vector<CrystalIndex> &,
                              const AtomFlags &flags = AtomFlag::NoFlag);
  void addVanDerWaalsContactAtoms();
  void removeVanDerWaalsContactAtoms();
  void deleteAtoms(const std::vector<int> &atomIndices);

  QString m_name;
  QString m_filename;
  OccCrystal m_crystal;

  std::vector<CrystalIndex> m_unitCellOffsets;
  ankerl::unordered_dense::map<CrystalIndex, int, CrystalIndexHash> m_atomMap;
  std::vector<std::vector<int>> m_fragments;
  std::vector<CrystalIndex> m_fragmentUnitCellMolecules;
  std::vector<int> m_fragmentForAtom;
  std::vector<std::vector<GenericAtomIndex>> m_symmetryUniqueFragments;
  std::vector<ChemicalStructure::FragmentState> m_symmetryUniqueFragmentStates;

  std::vector<std::pair<int, int>> m_covalentBonds;
  std::vector<std::pair<int, int>> m_vdwContacts;
  std::vector<std::pair<int, int>> m_hydrogenBonds;
};
