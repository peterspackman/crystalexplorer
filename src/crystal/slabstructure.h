#pragma once
#include "periodicstructure.h"
#include <occ/crystal/crystal.h>
#include <occ/crystal/hkl.h>
#include <occ/crystal/surface.h>

using OccCrystal = occ::crystal::Crystal;
using OccSurface = occ::crystal::Surface;

class CrystalStructure;

struct CrystalSurfaceCutOptions {
  occ::crystal::HKL millerPlane{1, 0, 0};  // Miller indices for cut direction
  double cutOffset{0.0};                   // Offset along normal (fractional)
  double thickness{0.0};                   // Slab thickness in Angstroms (0 = monolayer)
  bool preserveMolecules{true};            // Keep whole molecules vs cutting atoms
  QString termination{"auto"};             // Surface termination identifier
};

class SlabStructure : public PeriodicStructure {
  Q_OBJECT

public:
  explicit SlabStructure(QObject *parent = nullptr);

  // Core structure identification
  StructureType structureType() const override {
    return StructureType::Surface;
  }

  // PeriodicStructure interface implementation for 2D slabs
  bool isPeriodic(int dimension) const override { 
    return dimension < 2; // Only periodic in x,y (not z)
  }
  int periodicDimensions() const override { return 2; }

  // Build slab from crystal structure
  void buildFromCrystal(const CrystalStructure &crystal,
                       const CrystalSurfaceCutOptions &options);

  // Cell properties for 2D + vacuum structure
  occ::Mat3 cellVectors() const override;
  occ::Vec3 cellAngles() const override;
  occ::Vec3 cellLengths() const override;

  // Coordinate conversions
  occ::Mat3N convertCoordinates(const occ::Mat3N &pos,
                               CoordinateConversion conversion) const override;

  // Fragment management - using base class implementations
  FragmentIndex fragmentIndexForGeneralAtom(GenericAtomIndex) const override;

  // Overridden ChemicalStructure methods for 2D periodicity
  void buildSlab(SlabGenerationOptions) override;
  CellIndexSet occupiedCells() const override;
  
  // Override reset to go back to initial slab molecules
  void resetAtomsAndBonds(bool toSelection = false) override;

  // Slab-specific properties
  void setSlabThickness(double thickness);
  double slabThickness() const { return m_slabThickness; }

  void setMillerPlane(const occ::crystal::HKL &hkl);
  occ::crystal::HKL millerPlane() const { return m_millerPlane; }

  void setTermination(const QString &termination);
  QString termination() const { return m_termination; }

  void setCutOffset(double offset);
  double cutOffset() const { return m_cutOffset; }

  // Serialization
  nlohmann::json toJson() const override;
  bool fromJson(const nlohmann::json &json) override;

protected:
  // PeriodicStructure implementation for 2D slabs
  void addPeriodicAtoms(const std::vector<GenericAtomIndex> &indices,
                       const AtomFlags &flags = AtomFlag::NoFlag) override;
  void removePeriodicContactAtoms() override;
  void deleteAtomsByOffset(const std::vector<int> &atomIndices) override;

  // Unit cell connectivity for slab structure
  const occ::core::graph::PeriodicBondGraph& getUnitCellConnectivity() const override;

  std::vector<GenericAtomIndex>
  findAtomsWithinRadius(const std::vector<GenericAtomIndex> &centerAtoms,
                       float radius) const override;

private:
  void calculateSurfaceVectors(const OccCrystal &crystal);
  void updateSlabFragments();
  void buildSlabConnectivity() const;
  
  // Helper methods for slab-specific atom management
  Fragment makeSlabFragmentFromFragmentIndex(FragmentIndex idx) const;

  // Slab properties (use m_ prefix to match base class style)
  occ::Mat3 m_surfaceVectors;              // Surface coordinate system
  double m_slabThickness{0.0};             // Slab depth in Angstroms (0 = monolayer)
  double m_cutOffset{0.0};                 // Cut offset along normal (fractional)
  occ::crystal::HKL m_millerPlane{1, 0, 0}; // Miller indices for cut direction
  QString m_termination{"auto"};           // Surface termination identifier

  // Reference to parent crystal (optional, for regeneration)
  OccCrystal m_parentCrystal;
  CrystalSurfaceCutOptions m_lastOptions;
  
  // Store base slab atom data for periodic expansion
  occ::Mat3N m_baseSlabPositions;          // Base atom positions in slab
  occ::IVec m_baseSlabNumbers;             // Atomic numbers for base atoms
  std::vector<QString> m_baseSlabLabels;   // Labels for base atoms
  
  // Bond connectivity for 2D slab
  mutable occ::core::graph::PeriodicBondGraph m_slabConnectivity;
};