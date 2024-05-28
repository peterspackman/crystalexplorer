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

  // assumed to be CIF for now
  inline void setFileContents(const QByteArray &contents) { m_fileContents = contents; }
  inline const QByteArray &fileContents() const { return m_fileContents; }

  QString chemicalFormula() const;

  virtual inline StructureType structureType() const override {
    return StructureType::Crystal;
  }
  virtual inline occ::Mat3 cellVectors() const override {
    return m_crystal.unit_cell().direct();
  }

  virtual int fragmentIndexForAtom(int) const override;
  virtual void deleteFragmentContainingAtomIndex(int atomIndex) override;
 
  virtual const std::vector<Fragment>& getFragments() const override;
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
  virtual std::vector<int> selectedFragments() const override;

  virtual FragmentState getSymmetryUniqueFragmentState(int) const override;
  virtual void setSymmetryUniqueFragmentState(int, FragmentState) override;

  virtual const std::vector<Fragment> &symmetryUniqueFragments() const override;
  virtual const std::vector<FragmentState> &symmetryUniqueFragmentStates() const override;

  virtual void expandAtomsWithinRadius(float radius, bool selected) override;
  virtual Fragment makeFragment(const std::vector<GenericAtomIndex> &idxs) const override;

  [[nodiscard]] virtual std::vector<GenericAtomIndex> atomsWithFlags(const AtomFlags &flags) const override;
  [[nodiscard]] virtual std::vector<GenericAtomIndex> atomsSurroundingAtomsWithFlags(const AtomFlags &flags, float radius) const override;


  [[nodiscard]] virtual occ::IVec atomicNumbersForIndices(const std::vector<GenericAtomIndex> &) const override;

  [[nodiscard]] virtual occ::Mat3N atomicPositionsForIndices(const std::vector<GenericAtomIndex> &) const override;


private:
  Fragment makeAsymFragment(const std::vector<GenericAtomIndex> &idxs) const;

  void addAtomsByCrystalIndex(std::vector<GenericAtomIndex> &,
                              const AtomFlags &flags = AtomFlag::NoFlag);
  void addVanDerWaalsContactAtoms();
  void removeVanDerWaalsContactAtoms();
  void deleteAtoms(const std::vector<int> &atomIndices);

  QString m_filename;
  QByteArray m_fileContents;
  OccCrystal m_crystal;

  std::vector<GenericAtomIndex> m_unitCellOffsets;
  ankerl::unordered_dense::map<GenericAtomIndex, int, GenericAtomIndexHash> m_atomMap;
  std::vector<Fragment> m_fragments;
  std::vector<GenericAtomIndex> m_fragmentUnitCellMolecules;
  std::vector<int> m_fragmentForAtom;
  std::vector<Fragment> m_symmetryUniqueFragments;
  std::vector<ChemicalStructure::FragmentState> m_symmetryUniqueFragmentStates;

  std::vector<std::pair<int, int>> m_covalentBonds;
  std::vector<std::pair<int, int>> m_vdwContacts;
  std::vector<std::pair<int, int>> m_hydrogenBonds;
};
