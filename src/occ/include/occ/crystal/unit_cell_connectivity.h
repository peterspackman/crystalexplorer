#pragma once
#include <occ/core/kdtree.h>
#include <occ/crystal/crystal.h>

namespace occ::crystal {

using BondOverrides =
    ankerl::unordered_dense::map<occ::core::graph::PBCEdge,
                                 occ::core::graph::PeriodicEdge::Connection,
                                 occ::core::graph::PBCEdgeHash>;
using PBCEdgeSet = ankerl::unordered_dense::set<occ::core::graph::PBCEdge,
                                                occ::core::graph::PBCEdgeHash>;

class UnitCellConnectivityBuilder {
public:
  UnitCellConnectivityBuilder(const Crystal &crystal);
  PeriodicBondGraph build(const BondOverrides &overrides);

private:
  void finalize_unimplemented_connections(PeriodicBondGraph &graph,
                                          const BondOverrides &overrides);
  void initialize_vertices(PeriodicBondGraph &graph);

  void build_kdtree(const CrystalAtomRegion &slab);

  void detect_connections(PeriodicBondGraph &graph,
                          const CrystalAtomRegion &slab,
                          BondOverrides &overrides);

  void detect_atom_connections(PeriodicBondGraph &graph,
                               const CrystalAtomRegion &slab,
                               BondOverrides &overrides,
                               core::KdRadiusResultSet &results,
                               size_t uc_idx_l);

  const CrystalAtomRegion m_slab;
  const CrystalAtomRegion m_unit_cell_atoms;
  const Vec m_covalent_radii;
  const Vec m_vdw_radii;
  const IVec m_elements;
  std::vector<PeriodicBondGraph::VertexDescriptor> m_vertices;
  std::unique_ptr<occ::core::KDTree<double>> m_tree;
};

} // namespace occ::crystal
