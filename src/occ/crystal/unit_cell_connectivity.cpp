#include <occ/crystal/unit_cell_connectivity.h>

using occ::core::graph::PBCEdge;
using occ::core::graph::PeriodicEdge;

namespace occ::crystal {

UnitCellConnectivityBuilder::UnitCellConnectivityBuilder(const Crystal &crystal)
    : m_unit_cell_atoms(crystal.unit_cell_atoms()),
      m_slab(crystal.slab({-2, -2, -2}, {2, 2, 2})),
      m_covalent_radii(crystal.asymmetric_unit().covalent_radii()),
      m_vdw_radii(crystal.asymmetric_unit().vdw_radii()),
      m_elements(crystal.asymmetric_unit().atomic_numbers) {}

PeriodicBondGraph
UnitCellConnectivityBuilder::build(const BondOverrides &overrides) {
  PeriodicBondGraph graph;
  initialize_vertices(graph);
  build_kdtree(m_slab);

  BondOverrides overrides_copy = overrides;
  // copy so that we can modify it to remove overrides that have been done
  detect_connections(graph, m_slab, overrides_copy);
  // implement those that weren't detected as nearby
  finalize_unimplemented_connections(graph, overrides_copy);

  return graph;
}

void UnitCellConnectivityBuilder::initialize_vertices(
    PeriodicBondGraph &graph) {
  m_vertices.clear();
  for (size_t i = 0; i < m_unit_cell_atoms.size(); i++) {
    m_vertices.push_back(graph.add_vertex(occ::core::graph::PeriodicVertex{i}));
  }
}

void UnitCellConnectivityBuilder::build_kdtree(const CrystalAtomRegion &slab) {
  m_tree = std::make_unique<occ::core::KDTree<double>>(
      slab.cart_pos.rows(), slab.cart_pos, occ::core::max_leaf);
  m_tree->index->buildIndex();
}

void UnitCellConnectivityBuilder::detect_connections(
    PeriodicBondGraph &graph, const CrystalAtomRegion &slab,
    BondOverrides &overrides) {
  using Connection = PeriodicEdge::Connection;
  double max_vdw = m_vdw_radii.maxCoeff();
  double max_dist = (max_vdw * 2 + 0.6) * (max_vdw * 2 + 0.6);

  core::KdResultSet idxs_dists;
  core::KdRadiusResultSet results(max_dist, idxs_dists);

  for (size_t uc_idx_l = 0; uc_idx_l < m_unit_cell_atoms.size(); uc_idx_l++) {
    detect_atom_connections(graph, slab, overrides, results, uc_idx_l);
    results.clear();
  }
}

inline bool can_hbond(int a, int b) {
  if (a == 1) {
    if (b == 7 || b == 8 || b == 9)
      return true;
  } else if (b == 1) {
    if (a == 7 || a == 8 || a == 9)
      return true;
  }
  return false;
}

void UnitCellConnectivityBuilder::detect_atom_connections(
    PeriodicBondGraph &graph, const CrystalAtomRegion &slab,
    BondOverrides &overrides, core::KdRadiusResultSet &results,
    size_t uc_idx_l) {
  using Connection = PeriodicEdge::Connection;

  size_t asym_idx_l = m_unit_cell_atoms.asym_idx(uc_idx_l);
  double cov_a = m_covalent_radii(asym_idx_l);
  double vdw_a = m_vdw_radii(asym_idx_l);
  int el_a = m_elements(asym_idx_l);

  const double *q = slab.cart_pos.col(uc_idx_l).data();
  m_tree->index->findNeighbors(results, q, nanoflann::SearchParams());

  PBCEdgeSet implemented_overrides;
  for (const auto &[idx, dist2] : results.m_indices_dists) {
    if (idx == uc_idx_l)
      continue;

    size_t uc_idx_r = idx % m_unit_cell_atoms.size();
    if (uc_idx_r < uc_idx_l)
      continue;

    const int h = slab.hkl(0, idx);
    const int k = slab.hkl(1, idx);
    const int l = slab.hkl(2, idx);
    size_t asym_idx_r = m_unit_cell_atoms.asym_idx(uc_idx_r);
    double cov_b = m_covalent_radii(asym_idx_r);
    double vdw_b = m_vdw_radii(asym_idx_r);
    int el_b = m_elements(asym_idx_r);

    double threshold = (cov_a + cov_b + 0.4) * (cov_a + cov_b + 0.4);
    double threshold_vdw = (vdw_a + vdw_b + 0.6) * (vdw_a + vdw_b + 0.6);

    PBCEdge candidate{static_cast<int>(uc_idx_l), static_cast<int>(uc_idx_r), h,
                      k, l};

    auto add_edge = [&](Connection connectionType, double dist2) {
      const double dist = std::sqrt(dist2);
      PeriodicEdge left_right{dist,       uc_idx_l,   uc_idx_r,
                              asym_idx_l, asym_idx_r, h,
                              k,          l,          connectionType};
      PeriodicEdge right_left{dist,       uc_idx_r,   uc_idx_l,
                              asym_idx_r, asym_idx_l, -h,
                              -k,         -l,         connectionType};
      graph.add_edge(m_vertices[uc_idx_l], m_vertices[uc_idx_r], left_right);
      graph.add_edge(m_vertices[uc_idx_r], m_vertices[uc_idx_l], right_left);
    };

    Connection conn = Connection::DontBond;
    const auto kv = overrides.find(candidate);
    if (kv == overrides.end()) {
      if (dist2 < threshold) {
        conn = Connection::CovalentBond;
      } else if (dist2 < threshold_vdw) {
        conn = Connection::CloseContact;
      }
    } else {
      conn = kv->second;
      implemented_overrides.insert(kv->first);
    }
    if (conn != Connection::DontBond) {
      add_edge(conn, dist2);
      if (conn == Connection::CloseContact && can_hbond(el_a, el_b)) {
        add_edge(Connection::HydrogenBond, dist2);
      }
    }
  }
  for (const auto &edge : implemented_overrides) {
    overrides.erase(edge);
    PBCEdge back{edge.target, edge.source, -edge.h, -edge.k, -edge.l};
    overrides.erase(back);
  }
}

void UnitCellConnectivityBuilder::finalize_unimplemented_connections(
    PeriodicBondGraph &graph, const BondOverrides &overrides) {
  using Connection = PeriodicEdge::Connection;
  for (const auto &[edge, conn] : overrides) {
    if (conn == Connection::DontBond)
      continue;
    if (edge.source > edge.target)
      continue;

    size_t uc_idx_l = edge.source;
    size_t uc_idx_r = edge.target;
    size_t asym_idx_l = m_unit_cell_atoms.asym_idx(uc_idx_l);
    size_t asym_idx_r = m_unit_cell_atoms.asym_idx(uc_idx_r);
    int el_a = m_elements(asym_idx_l);
    int el_b = m_elements(asym_idx_r);
    int h = edge.h;
    int k = edge.k;
    int l = edge.l;

    auto add_edge = [&](Connection c, double dist) {
      PeriodicEdge left_right{dist, uc_idx_l, uc_idx_r, asym_idx_l, asym_idx_r,
                              h,    k,        l,        c};
      PeriodicEdge right_left{dist, uc_idx_r, uc_idx_l, asym_idx_r, asym_idx_l,
                              -h,   -k,       -l,       c};
      graph.add_edge(m_vertices[uc_idx_l], m_vertices[uc_idx_r], left_right);
      graph.add_edge(m_vertices[uc_idx_r], m_vertices[uc_idx_l], right_left);
    };

    add_edge(conn, 0.0);
    if (conn == Connection::CloseContact && can_hbond(el_a, el_b)) {
      add_edge(Connection::HydrogenBond, 0.0);
    }
  }
}

} // namespace occ::crystal
