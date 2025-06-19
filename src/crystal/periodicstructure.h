#pragma once
#include "chemicalstructure.h"
#include "crystalplane.h"
#include "dimer_graph.h"
#include "fragment_index.h"
#include <QHash>
#include <ankerl/unordered_dense.h>
#include <occ/crystal/crystal.h>
#include <occ/crystal/dimer_mapping_table.h>
#include <occ/core/graph.h>

using OccCrystal = occ::crystal::Crystal;

// Abstract base class for structures with periodic boundary conditions
// Provides common functionality for both 3D crystals and 2D slabs
class PeriodicStructure : public ChemicalStructure {
  Q_OBJECT;

public:
  PeriodicStructure(QObject *parent = nullptr);

  // Common periodic structure interface
  virtual bool isPeriodic(int dimension) const = 0; // 0=x, 1=y, 2=z
  virtual int periodicDimensions() const = 0;       // Number of periodic dimensions (2 or 3)

  // Index conversion methods (common to both 2D and 3D)
  int genericIndexToIndex(const GenericAtomIndex &) const override;
  GenericAtomIndex indexToGenericIndex(int) const override;

  // Fragment management (common patterns)
  FragmentIndex fragmentIndexForGeneralAtom(GenericAtomIndex) const override;
  void deleteFragmentContainingAtomIndex(int atomIndex) override;
  void deleteIncompleteFragments() override;
  void deleteAtoms(const std::vector<GenericAtomIndex> &) override;

  // Bond and atom management
  void updateBondGraph() override;
  void resetAtomsAndBonds(bool toSelection = false) override;

  // Fragment operations (overridable for 2D vs 3D behavior)
  virtual void completeFragmentContaining(int) override;
  virtual void completeFragmentContaining(GenericAtomIndex) override;
  virtual bool hasIncompleteFragments() const override;
  virtual bool hasIncompleteSelectedFragments() const override;
  virtual void completeAllFragments() override;
  
  // Selection propagation through bond graph
  void propagateAtomFlagViaConnectivity(GenericAtomIndex startAtom, 
                                       AtomFlag flag, 
                                       bool set = true);
  
  // Override selection to use bond graph connectivity
  void selectFragmentContaining(int atom) override;
  void selectFragmentContaining(GenericAtomIndex atom) override;

  // Atom expansion (dimension-aware)
  virtual void expandAtomsWithinRadius(float radius, bool selected) override;

  // Contact handling
  virtual void setShowContacts(const ContactSettings &) override;

  // Fragment utilities
  std::vector<FragmentIndex> completedFragments() const override;
  std::vector<FragmentIndex> selectedFragments() const override;
  Fragment makeFragment(const std::vector<GenericAtomIndex> &idxs) const override;

  // Atom querying
  std::vector<GenericAtomIndex>
  atomsWithFlags(const AtomFlags &flags, bool set = true) const override;
  virtual std::vector<GenericAtomIndex>
  atomsSurroundingAtoms(const std::vector<GenericAtomIndex> &,
                        float radius) const override;
  virtual std::vector<GenericAtomIndex>
  atomsSurroundingAtomsWithFlags(const AtomFlags &flags,
                                 float radius) const override;

  // Coordinate utilities
  occ::IVec atomicNumbersForIndices(const std::vector<GenericAtomIndex> &) const override;
  std::vector<QString> labelsForIndices(const std::vector<GenericAtomIndex> &) const override;
  occ::Mat3N atomicPositionsForIndices(const std::vector<GenericAtomIndex> &) const override;
  
  // Override ChemicalStructure fragment interface for proper labeling
  const FragmentMap &symmetryUniqueFragments() const override;

  // Transformation utilities
  std::vector<GenericAtomIndex> getAtomIndicesUnderTransformation(
      const std::vector<GenericAtomIndex> &idxs,
      const Eigen::Isometry3d &result) const override;
  bool getTransformation(const std::vector<GenericAtomIndex> &from_orig,
                         const std::vector<GenericAtomIndex> &to_orig,
                         Eigen::Isometry3d &result) const override;

protected:
  // Dimension-aware methods for subclasses to override
  virtual void addPeriodicAtoms(const std::vector<GenericAtomIndex> &indices,
                               const AtomFlags &flags = AtomFlag::NoFlag) = 0;
  virtual void addPeriodicContactAtoms();
  virtual void removePeriodicContactAtoms() = 0;
  virtual void deleteAtomsByOffset(const std::vector<int> &atomIndices) = 0;

  // Unit cell connectivity - subclasses provide their connectivity graph
  virtual const occ::core::graph::PeriodicBondGraph& getUnitCellConnectivity() const = 0;

  // Dimension-aware radius expansion
  virtual std::vector<GenericAtomIndex>
  findAtomsWithinRadius(const std::vector<GenericAtomIndex> &centerAtoms,
                       float radius) const = 0;

  // Common data members (mirroring CrystalStructure pattern)
  std::vector<GenericAtomIndex> m_periodicAtomOffsets;
  ankerl::unordered_dense::map<GenericAtomIndex, int, GenericAtomIndexHash> m_periodicAtomMap;
  ContactSettings m_contactSettings;

  // Fragment management (shared between 2D and 3D)
  FragmentMap m_periodicFragments;
  
  // Unit cell fragment mapping (like CrystalStructure)
  FragmentMap m_unitCellFragments;
  ankerl::unordered_dense::map<int, FragmentIndex> m_unitCellAtomFragments;

protected:
  // Common utilities for subclasses
  void updateFragmentMapping();
  void buildUnitCellFragments();
  void deleteAtomsByOffsetCommon(const std::vector<int> &atomIndices);

private:
};