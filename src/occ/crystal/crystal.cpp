#include <fmt/core.h>
#include <occ/core/element.h>
#include <occ/core/kdtree.h>
#include <occ/core/linear_algebra.h>
#include <occ/crystal/crystal.h>
#include <occ/crystal/standard_bonds.h>
#include <occ/crystal/unit_cell_connectivity.h>

namespace occ::crystal {

// Atom Slab
const CrystalAtomRegion &Crystal::unit_cell_atoms() const {
  if (m_unit_cell_atoms_needs_update)
    update_unit_cell_atoms();
  return m_unit_cell_atoms;
}

inline Mat3N clean_small_values(const Mat3N &v, double epsilon = 1e-14) {
  return v.unaryExpr(
      [epsilon](double x) { return (std::abs(x) < epsilon) ? 0.0 : x; });
}

inline Mat3N wrap_to_unit_cell(const Mat3N &v) {
  return (v.array() - v.array().floor());
}

void Crystal::update_unit_cell_atoms() const {
  // TODO merge sites
  constexpr double merge_tolerance = 1e-2;
  const auto &pos = m_asymmetric_unit.positions;
  const auto &atoms = m_asymmetric_unit.atomic_numbers;
  const int natom = num_sites();
  const int nsymops = symmetry_operations().size();
  Vec occupation = m_asymmetric_unit.occupations.replicate(nsymops, 1);
  IVec uc_nums = atoms.replicate(nsymops, 1);
  IVec asym_idx = IVec::LinSpaced(natom, 0, natom - 1).replicate(nsymops, 1);
  IVec sym;
  Mat3N uc_pos;
  std::tie(sym, uc_pos) = m_space_group.apply_all_symmetry_operations(pos);
  uc_pos = clean_small_values(uc_pos);
  uc_pos = wrap_to_unit_cell(uc_pos);

  occ::MaskArray mask(uc_pos.cols());
  mask.setConstant(false);

  for (size_t i = 0; i < uc_pos.cols(); i++) {
    if ((mask(i)))
      continue;
    occ::Vec3 p = uc_pos.col(i);
    for (size_t j = i + 1; j < uc_pos.cols(); j++) {
      double dist = (uc_pos.col(j) - p).norm();
      if (dist < merge_tolerance)
        mask(j) = true;
      if (occupation.rows() > 0)
        occupation(i) += occupation(j);
    }
  }
  IVec idxs(uc_pos.cols() - mask.count());
  IVec uc_idxs(uc_pos.cols() - mask.count());
  size_t n = 0;
  Mat3N uc_pos_masked(3, idxs.rows());
  for (size_t i = 0; i < uc_pos.cols(); i++) {
    if (!mask(i)) {
      idxs(n) = i;
      uc_idxs(n) = n;
      uc_pos_masked.col(n) = uc_pos.col(i);
      n++;
    }
  }
  m_unit_cell_atoms = CrystalAtomRegion{uc_pos_masked,
                                        m_unit_cell.to_cartesian(uc_pos_masked),
                                        idxs.unaryExpr(asym_idx),
                                        uc_idxs,
                                        IMat3N::Zero(3, uc_pos_masked.cols()),
                                        idxs.unaryExpr(uc_nums),
                                        idxs.unaryExpr(sym)};
  m_unit_cell_atoms_needs_update = false;
}

CrystalAtomRegion Crystal::slab(const HKL &lower, const HKL &upper) const {
  int ncells = (upper.h - lower.h + 1) * (upper.k - lower.k + 1) *
               (upper.l - lower.l + 1);
  const CrystalAtomRegion &uc_atoms = unit_cell_atoms();
  const size_t n_uc = uc_atoms.size();
  CrystalAtomRegion result;
  const int rows = uc_atoms.frac_pos.rows();
  const int cols = uc_atoms.frac_pos.cols();
  result.frac_pos.resize(3, ncells * n_uc);
  result.frac_pos.block(0, 0, rows, cols) = uc_atoms.frac_pos;
  result.hkl = IMat3N::Zero(3, ncells * n_uc);
  result.hkl.block(0, 0, 3, cols) = uc_atoms.hkl;
  result.asym_idx = uc_atoms.asym_idx.replicate(ncells, 1);
  result.uc_idx = uc_atoms.uc_idx.replicate(ncells, 1);
  result.symop = uc_atoms.symop.replicate(ncells, 1);
  result.atomic_numbers = uc_atoms.atomic_numbers.replicate(ncells, 1);
  int offset = n_uc;
  for (int h = lower.h; h <= upper.h; h++) {
    for (int k = lower.k; k <= upper.k; k++) {
      for (int l = lower.l; l <= upper.l; l++) {
        if (h == 0 && k == 0 && l == 0)
          continue;
        auto tmp = uc_atoms.frac_pos;
        tmp.colwise() +=
            Eigen::Vector3d{static_cast<double>(h), static_cast<double>(k),
                            static_cast<double>(l)};
        auto tmphkl = uc_atoms.hkl;
        tmphkl.colwise() += IVec3{h, k, l};
        result.frac_pos.block(0, offset, rows, cols) = tmp;
        result.hkl.block(0, offset, 3, cols) = tmphkl;
        offset += n_uc;
      }
    }
  }
  result.cart_pos = to_cartesian(result.frac_pos);
  return result;
}

CrystalAtomRegion Crystal::atom_surroundings(int asym_idx,
                                             double radius) const {
  // TODO streamline this code with the full asymmetric unit equivalent
  HKL upper = HKL::minimum();
  HKL lower = HKL::maximum();
  occ::Vec3 frac_radius = (radius + 1.0) / m_unit_cell.lengths().array();

  for (size_t i = 0; i < m_asymmetric_unit.positions.cols(); i++) {
    const auto &pos = m_asymmetric_unit.positions.col(i);
    upper.h =
        std::max(upper.h, static_cast<int>(ceil(pos(0) + frac_radius(0))));
    upper.k =
        std::max(upper.k, static_cast<int>(ceil(pos(1) + frac_radius(1))));
    upper.l =
        std::max(upper.l, static_cast<int>(ceil(pos(2) + frac_radius(2))));

    lower.h =
        std::min(lower.h, static_cast<int>(floor(pos(0) - frac_radius(0))));
    lower.k =
        std::min(lower.k, static_cast<int>(floor(pos(1) - frac_radius(1))));
    lower.l =
        std::min(lower.l, static_cast<int>(floor(pos(2) - frac_radius(2))));
  }

  auto atom_slab = slab(lower, upper);
  occ::core::KDTree<double> tree(atom_slab.cart_pos.rows(), atom_slab.cart_pos,
                                 occ::core::max_leaf);
  tree.index->buildIndex();

  core::KdResultSet idxs_dists;
  core::KdRadiusResultSet results(radius * radius, idxs_dists);

  Mat3N asym_cart_pos = to_cartesian(m_asymmetric_unit.positions);

  double *q = asym_cart_pos.col(asym_idx).data();
  tree.index->findNeighbors(results, q, nanoflann::SearchParams());

  CrystalAtomRegion result;
  if (idxs_dists.size() < 1)
    return result;
  result.resize(idxs_dists.size()); // -1 for the self

  int result_idx = 0;
  for (const auto &[idx, d] : idxs_dists) {
    if (d < 1e-3) {
      continue;
    }
    result.frac_pos.col(result_idx) = atom_slab.frac_pos.col(idx);
    result.atomic_numbers(result_idx) = atom_slab.atomic_numbers(idx);
    result.asym_idx(result_idx) = atom_slab.asym_idx(idx);
    result.uc_idx(result_idx) = atom_slab.uc_idx(idx);
    result.cart_pos.col(result_idx) = atom_slab.cart_pos.col(idx);
    result.symop(result_idx) = atom_slab.symop(idx);
    result_idx++;
  }
  result.resize(result_idx);
  return result;
}

std::vector<CrystalAtomRegion>
Crystal::asymmetric_unit_atom_surroundings(double radius) const {

  HKL upper = HKL::minimum();
  HKL lower = HKL::maximum();
  occ::Vec3 frac_radius = radius * 2 / m_unit_cell.lengths().array();

  for (size_t i = 0; i < m_asymmetric_unit.positions.cols(); i++) {
    const auto &pos = m_asymmetric_unit.positions.col(i);
    upper.h =
        std::max(upper.h, static_cast<int>(ceil(pos(0) + frac_radius(0))));
    upper.k =
        std::max(upper.k, static_cast<int>(ceil(pos(1) + frac_radius(1))));
    upper.l =
        std::max(upper.l, static_cast<int>(ceil(pos(2) + frac_radius(2))));

    lower.h =
        std::min(lower.h, static_cast<int>(floor(pos(0) - frac_radius(0))));
    lower.k =
        std::min(lower.k, static_cast<int>(floor(pos(1) - frac_radius(1))));
    lower.l =
        std::min(lower.l, static_cast<int>(floor(pos(2) - frac_radius(2))));
  }
  auto atom_slab = slab(lower, upper);
  occ::core::KDTree<double> tree(atom_slab.cart_pos.rows(), atom_slab.cart_pos,
                                 occ::core::max_leaf);
  tree.index->buildIndex();

  core::KdResultSet idxs_dists;
  core::KdRadiusResultSet results(radius * radius, idxs_dists);

  Mat3N asym_cart_pos = to_cartesian(m_asymmetric_unit.positions);

  std::vector<CrystalAtomRegion> regions;
  for (int asym_idx = 0; asym_idx < num_sites(); asym_idx++) {

    double *q = asym_cart_pos.col(asym_idx).data();
    tree.index->findNeighbors(results, q, nanoflann::SearchParams());
    regions.push_back(CrystalAtomRegion{});
    auto &reg = regions.back();
    reg.resize(idxs_dists.size() - 1); // -1 for the self

    int result_idx = 0;
    for (const auto &[idx, d] : idxs_dists) {
      if (d < 1e-3)
        continue;
      reg.frac_pos.col(result_idx) = atom_slab.frac_pos.col(idx);
      reg.atomic_numbers(result_idx) = atom_slab.atomic_numbers(idx);
      reg.asym_idx(result_idx) = atom_slab.asym_idx(idx);
      reg.uc_idx(result_idx) = atom_slab.uc_idx(idx);
      reg.cart_pos.col(result_idx) = atom_slab.cart_pos.col(idx);
      reg.symop(result_idx) = atom_slab.symop(idx);
      result_idx++;
    }
    reg.resize(result_idx);
    results.clear();
  }
  return regions;
}

std::vector<CrystalAtomRegion>
Crystal::unit_cell_atom_surroundings(double radius) const {

  HKL upper = HKL::minimum();
  HKL lower = HKL::maximum();
  occ::Vec3 frac_radius = radius * 2 / m_unit_cell.lengths().array();

  for (size_t i = 0; i < m_asymmetric_unit.positions.cols(); i++) {
    const auto &pos = m_asymmetric_unit.positions.col(i);
    upper.h =
        std::max(upper.h, static_cast<int>(ceil(pos(0) + frac_radius(0))));
    upper.k =
        std::max(upper.k, static_cast<int>(ceil(pos(1) + frac_radius(1))));
    upper.l =
        std::max(upper.l, static_cast<int>(ceil(pos(2) + frac_radius(2))));

    lower.h =
        std::min(lower.h, static_cast<int>(floor(pos(0) - frac_radius(0))));
    lower.k =
        std::min(lower.k, static_cast<int>(floor(pos(1) - frac_radius(1))));
    lower.l =
        std::min(lower.l, static_cast<int>(floor(pos(2) - frac_radius(2))));
  }
  auto atom_slab = slab(lower, upper);
  occ::core::KDTree<double> tree(atom_slab.cart_pos.rows(), atom_slab.cart_pos,
                                 occ::core::max_leaf);
  tree.index->buildIndex();

  std::vector<std::pair<Eigen::Index, double>> idxs_dists;
  nanoflann::RadiusResultSet results(radius * radius, idxs_dists);

  const auto &uc_atoms = unit_cell_atoms();
  const Mat3N &uc_cart_pos = uc_atoms.cart_pos;

  std::vector<CrystalAtomRegion> regions;
  for (int uc_idx = 0; uc_idx < uc_atoms.size(); uc_idx++) {

    const double *q = uc_cart_pos.col(uc_idx).data();
    tree.index->findNeighbors(results, q, nanoflann::SearchParams());
    regions.push_back(CrystalAtomRegion{});
    auto &reg = regions.back();
    reg.resize(idxs_dists.size() - 1); // -1 for the self

    int result_idx = 0;
    for (const auto &[idx, d] : idxs_dists) {
      if (d < 1e-3)
        continue;
      reg.frac_pos.col(result_idx) = atom_slab.frac_pos.col(idx);
      reg.atomic_numbers(result_idx) = atom_slab.atomic_numbers(idx);
      reg.asym_idx(result_idx) = atom_slab.asym_idx(idx);
      reg.uc_idx(result_idx) = atom_slab.uc_idx(idx);
      reg.cart_pos.col(result_idx) = atom_slab.cart_pos.col(idx);
      reg.symop(result_idx) = atom_slab.symop(idx);
      reg.hkl.col(result_idx) = atom_slab.hkl.col(idx);
      result_idx++;
    }
    reg.resize(result_idx);
    results.clear();
  }
  return regions;
}

std::vector<CrystalAtomRegion>
Crystal::atoms_surrounding_points(const Mat3N &frac_pos, double radius,
                                  double tolerance) const {

  HKL upper = HKL::minimum();
  HKL lower = HKL::maximum();
  occ::Vec3 frac_radius = radius * 2 / m_unit_cell.lengths().array();

  for (size_t i = 0; i < frac_pos.cols(); i++) {
    const auto &pos = frac_pos.col(i);
    upper.h =
        std::max(upper.h, static_cast<int>(ceil(pos(0) + frac_radius(0))));
    upper.k =
        std::max(upper.k, static_cast<int>(ceil(pos(1) + frac_radius(1))));
    upper.l =
        std::max(upper.l, static_cast<int>(ceil(pos(2) + frac_radius(2))));

    lower.h =
        std::min(lower.h, static_cast<int>(floor(pos(0) - frac_radius(0))));
    lower.k =
        std::min(lower.k, static_cast<int>(floor(pos(1) - frac_radius(1))));
    lower.l =
        std::min(lower.l, static_cast<int>(floor(pos(2) - frac_radius(2))));
  }
  auto atom_slab = slab(lower, upper);
  occ::core::KDTree<double> tree(atom_slab.cart_pos.rows(), atom_slab.cart_pos,
                                 occ::core::max_leaf);
  tree.index->buildIndex();

  std::vector<std::pair<Eigen::Index, double>> idxs_dists;
  nanoflann::RadiusResultSet results(radius * radius, idxs_dists);

  Mat3N cart_pos = to_cartesian(frac_pos);

  std::vector<CrystalAtomRegion> regions;
  for (int asym_idx = 0; asym_idx < num_sites(); asym_idx++) {

    double *q = cart_pos.col(asym_idx).data();
    tree.index->findNeighbors(results, q, nanoflann::SearchParams());
    regions.push_back(CrystalAtomRegion{});
    auto &reg = regions.back();
    reg.resize(idxs_dists.size() - 1); // -1 for the self

    int result_idx = 0;
    for (const auto &[idx, d] : idxs_dists) {
      if (d < 1e-3)
        continue;
      reg.frac_pos.col(result_idx) = atom_slab.frac_pos.col(idx);
      reg.atomic_numbers(result_idx) = atom_slab.atomic_numbers(idx);
      reg.asym_idx(result_idx) = atom_slab.asym_idx(idx);
      reg.uc_idx(result_idx) = atom_slab.uc_idx(idx);
      reg.cart_pos.col(result_idx) = atom_slab.cart_pos.col(idx);
      reg.symop(result_idx) = atom_slab.symop(idx);
      result_idx++;
    }
    reg.resize(result_idx);
    results.clear();
  }
  return regions;
}

Crystal::Crystal(const AsymmetricUnit &asym, const SpaceGroup &sg,
                 const UnitCell &uc)
    : m_asymmetric_unit(asym), m_space_group(sg), m_unit_cell(uc) {
  m_bond_mapping_table =
      DimerMappingTable::create_atomic_pair_table(*this, true);
}

const PeriodicBondGraph &Crystal::unit_cell_connectivity() const {
  if (m_unit_cell_connectivity_needs_update)
    update_unit_cell_connectivity();
  return m_bond_graph;
}

void Crystal::set_connectivity_criteria(bool guess) {
  m_guess_connectivity = guess;
}

void Crystal::add_bond_override(size_t atom_a, size_t atom_b,
                                const HKL &cell_offset,
                                core::graph::PeriodicEdge::Connection type) {
  // Create original edge and corresponding dimer
  core::graph::PBCEdge edge{static_cast<int>(atom_a), static_cast<int>(atom_b),
                            cell_offset.h, cell_offset.k, cell_offset.l};

  DimerIndex dimer{{edge.source, {0, 0, 0}},
                   {edge.target, {edge.h, edge.k, edge.l}}};

  // Get canonical form
  DimerIndex canonical = m_bond_mapping_table.symmetry_unique_dimer(dimer);

  // Convert back to edge and store
  core::graph::PBCEdge canonical_edge{static_cast<int>(canonical.a.offset),
                                      static_cast<int>(canonical.b.offset),
                                      canonical.b.hkl.h, canonical.b.hkl.k,
                                      canonical.b.hkl.l};

  m_bond_overrides[canonical_edge] = type;
  m_unit_cell_connectivity_needs_update = true;
}

void Crystal::clear_bond_overrides() {
  if (!m_bond_overrides.empty()) {
    m_bond_overrides.clear();
    m_unit_cell_connectivity_needs_update = true;
  }
}

void Crystal::update_unit_cell_connectivity() const {
  if (!m_guess_connectivity) {
    m_unit_cell_connectivity_needs_update = false;
    return;
  }

  BondOverrides full_overrides;

  // Generate all symmetry-related overrides
  for (const auto &[edge, conn_type] : m_bond_overrides) {
    DimerIndex dimer{{edge.source, {0, 0, 0}},
                     {edge.target, {edge.h, edge.k, edge.l}}};

    // Get all related dimers
    auto related = m_bond_mapping_table.symmetry_related_dimers(dimer);

    for (const auto &related_dimer : related) {
      // Forward direction
      core::graph::PBCEdge sym_edge{
          static_cast<int>(related_dimer.a.offset),
          static_cast<int>(related_dimer.b.offset), related_dimer.b.hkl.h,
          related_dimer.b.hkl.k, related_dimer.b.hkl.l};
      full_overrides[sym_edge] = conn_type;

      // Reverse direction
      core::graph::PBCEdge reverse_edge{sym_edge.target, sym_edge.source,
                                        -sym_edge.h, -sym_edge.k, -sym_edge.l};
      full_overrides[reverse_edge] = conn_type;
    }
  }

  UnitCellConnectivityBuilder builder(*this);
  m_bond_graph = builder.build(full_overrides);
  m_unit_cell_connectivity_needs_update = false;
  m_symmetry_unique_molecules_needs_update = true;
  m_unit_cell_molecules_needs_update = true;
}

const std::vector<occ::core::Molecule> &Crystal::unit_cell_molecules() const {
  if (m_unit_cell_molecules_needs_update ||
      m_unit_cell_connectivity_needs_update)
    update_unit_cell_molecules();
  return m_unit_cell_molecules;
}

void Crystal::update_unit_cell_molecules() const {
  using vertex_desc =
      typename occ::core::graph::PeriodicBondGraph::VertexDescriptor;
  using edge_desc =
      typename occ::core::graph::PeriodicBondGraph::EdgeDescriptor;
  using Connection = occ::core::graph::PeriodicEdge::Connection;
  m_unit_cell_molecules.clear();
  auto g = unit_cell_connectivity();
  const auto &edges = g.edges();
  auto atoms = unit_cell_atoms();
  std::vector<HKL> shifts_vec(atoms.size());
  std::vector<std::vector<int>> groups;

  size_t uc_mol_idx{0};
  // reuse these
  IVec molecule_index(atoms.size());
  std::vector<std::vector<int>> atom_indices;
  std::vector<std::vector<std::pair<size_t, size_t>>> mol_bonds;
  IMat3N shifts = IMat3N::Zero(3, atoms.size());
  ankerl::unordered_dense::set<vertex_desc> visited;

  auto predicate = [&edges](const edge_desc &e) {
    return edges.at(e).connectionType == Connection::CovalentBond;
  };

  auto visitor = [&](const vertex_desc &v, const vertex_desc &prev,
                     const edge_desc &e) {
    const auto &edge = edges.at(e);
    auto &idxs = atom_indices[uc_mol_idx];
    visited.insert(v);
    molecule_index(v) = uc_mol_idx;
    idxs.push_back(v);
    if (v != prev) {
      IVec3 uc_shift(edge.h, edge.k, edge.l);
      shifts.col(v) = shifts.col(prev) + uc_shift;
      mol_bonds[uc_mol_idx].push_back({prev, v});
    }
  };

  for (const auto &v : g.vertices()) {
    atom_indices.push_back({});
    mol_bonds.push_back({});
    if (visited.contains(v.first))
      continue;
    g.breadth_first_traversal_with_edge_filtered(v.first, visitor, predicate);
    uc_mol_idx++;
  }

  Mat3N cart_pos = to_cartesian(atoms.frac_pos + shifts.cast<double>());
  for (size_t i = 0; i < uc_mol_idx; i++) {
    auto idx = atom_indices[i];

    // sort by asymmetric atom index, then symop,
    // but retain ordering otherwise
    // if they have the same asym atom and symop using stable sort
    std::stable_sort(idx.begin(), idx.end(), [&atoms](int a, int b) {
      if (atoms.asym_idx(a) == atoms.asym_idx(b)) {
        return atoms.symop(a) < atoms.symop(b);
      }
      return atoms.asym_idx(a) < atoms.asym_idx(b);
    });
    IMat3N shift_hkl = shifts(Eigen::all, idx);

    occ::core::Molecule m(atoms.atomic_numbers(idx), cart_pos({0, 1, 2}, idx));
    m.set_unit_cell_idx(Eigen::Map<const IVec>(idx.data(), idx.size()));
    m.set_unit_cell_atom_shift(shift_hkl);
    m.set_asymmetric_unit_idx(atoms.asym_idx(idx));
    m.set_asymmetric_unit_symop(atoms.symop(idx));
    m.set_unit_cell_molecule_idx(i);
    m_unit_cell_molecules.push_back(m);
    // TODO set bonding information
  }

  m_unit_cell_molecules_needs_update = false;
}

const std::vector<occ::core::Molecule> &
Crystal::symmetry_unique_molecules() const {
  if (m_symmetry_unique_molecules_needs_update ||
      m_unit_cell_connectivity_needs_update)
    update_symmetry_unique_molecules();
  return m_symmetry_unique_molecules;
}

void Crystal::update_symmetry_unique_molecules() const {
  const auto &uc_molecules = unit_cell_molecules();
  auto asym_atoms_found = std::vector<bool>(m_asymmetric_unit.size());
  m_symmetry_unique_molecules.clear();

  // sort by proportion of identity symop
  std::vector<size_t> indexes(uc_molecules.size());
  std::iota(indexes.begin(), indexes.end(), 0);
  auto sort_func = [&uc_molecules](size_t a, size_t b) {
    const auto &symops_a = uc_molecules[a].asymmetric_unit_symop();
    size_t n_a = symops_a.rows();
    const auto &symops_b = uc_molecules[b].asymmetric_unit_symop();
    size_t n_b = symops_b.rows();
    double pct_a =
        std::count(symops_a.data(), symops_a.data() + n_a, 16484) * 1.0 / n_a;
    double pct_b =
        std::count(symops_b.data(), symops_b.data() + n_b, 16484) * 1.0 / n_b;
    return pct_a > pct_b;
  };
  std::stable_sort(indexes.begin(), indexes.end(), sort_func);

  int num_found = 0;

  for (const auto &idx : indexes) {
    const auto &mol = uc_molecules[idx];
    const auto &asym_atoms_in_group = mol.asymmetric_unit_idx();
    bool all_found = true;
    for (size_t i = 0; i < asym_atoms_in_group.rows(); i++) {
      if (!asym_atoms_found[asym_atoms_in_group(i)]) {
        all_found = false;
        break;
      }
    }
    if (all_found)
      continue;
    else {
      for (size_t i = 0; i < asym_atoms_in_group.rows(); i++)
        asym_atoms_found[asym_atoms_in_group(i)] = true;
    }
    m_symmetry_unique_molecules.push_back(mol);
    m_symmetry_unique_molecules[num_found].set_asymmetric_molecule_idx(
        num_found);
    num_found++;
    if (std::all_of(asym_atoms_found.begin(), asym_atoms_found.end(),
                    [](bool v) { return v; }))
      break;
  }

  // now populate unit_cell_molecules
  for (auto &uc_mol : m_unit_cell_molecules) {
    if (uc_mol.asymmetric_molecule_idx() >= 0)
      continue;
    const auto uc_mol_asym = uc_mol.asymmetric_unit_idx();
    const auto uc_mol_size = uc_mol.size();
    for (const auto &asym_mol : m_symmetry_unique_molecules) {
      const auto asym_mol_size = asym_mol.size();
      if (asym_mol_size != uc_mol_size)
        continue;
      const auto asym_mol_asym = asym_mol.asymmetric_unit_idx();
      if ((uc_mol_asym.array() == asym_mol_asym.array()).all()) {
        uc_mol.set_asymmetric_molecule_idx(asym_mol.asymmetric_molecule_idx());
        break;
      }
    }
  }
  m_symmetry_unique_molecules_needs_update = false;
}

CrystalDimers Crystal::symmetry_unique_dimers(double radius) const {
  using occ::core::Dimer;
  CrystalDimers result;
  result.radius = radius;
  auto &dimers = result.unique_dimers;
  auto &mol_nbs = result.molecule_neighbors;

  HKL upper = HKL::minimum();
  HKL lower = HKL::maximum();
  occ::Vec3 frac_radius = radius * 2 / m_unit_cell.lengths().array();

  for (size_t i = 0; i < m_asymmetric_unit.size(); i++) {
    const auto &pos = m_asymmetric_unit.positions.col(i);
    upper.h =
        std::max(upper.h, static_cast<int>(ceil(pos(0) + frac_radius(0))));
    upper.k =
        std::max(upper.k, static_cast<int>(ceil(pos(1) + frac_radius(1))));
    upper.l =
        std::max(upper.l, static_cast<int>(ceil(pos(2) + frac_radius(2))));

    lower.h =
        std::min(lower.h, static_cast<int>(floor(pos(0) - frac_radius(0))));
    lower.k =
        std::min(lower.k, static_cast<int>(floor(pos(1) - frac_radius(1))));
    lower.l =
        std::min(lower.l, static_cast<int>(floor(pos(2) - frac_radius(2))));
  }

  const auto &uc_mols = unit_cell_molecules();
  const auto &asym_mols = symmetry_unique_molecules();
  mol_nbs.resize(asym_mols.size());

  for (int h = lower.h; h <= upper.h; h++) {
    for (int k = lower.k; k <= upper.k; k++) {
      for (int l = lower.l; l <= upper.l; l++) {
        occ::Vec3 cart_shift = to_cartesian(occ::Vec3{static_cast<double>(h),
                                                      static_cast<double>(k),
                                                      static_cast<double>(l)});
        for (const auto &asym_mol : asym_mols) {
          int asym_idx_a = asym_mol.asymmetric_molecule_idx();
          for (const auto &uc_mol : uc_mols) {
            auto mol_translated = uc_mol.translated(cart_shift);
            mol_translated.set_cell_shift({h, k, l});
            double distance =
                std::get<2>(asym_mol.nearest_atom(mol_translated));
            if ((distance < radius) && (distance > 1e-1)) {
              Dimer d(asym_mol, mol_translated);
              mol_nbs[asym_idx_a].push_back({d, -1});
              if (std::any_of(dimers.begin(), dimers.end(),
                              [&d](const Dimer &d2) { return d == d2; }))
                continue;
              dimers.push_back(d);
            }
          }
        }
      }
    }
  }

  auto dimer_sort_func = [](const Dimer &a, const Dimer &b) {
    return a.nearest_distance() < b.nearest_distance();
  };

  auto sort_func = [](const CrystalDimers::SymmetryRelatedDimer &a,
                      const CrystalDimers::SymmetryRelatedDimer &b) {
    return a.dimer.nearest_distance() < b.dimer.nearest_distance();
  };

  std::stable_sort(dimers.begin(), dimers.end(), dimer_sort_func);
  for (auto &vec : mol_nbs) {
    std::stable_sort(vec.begin(), vec.end(), sort_func);
    for (auto &d : vec) {
      size_t idx = std::distance(
          dimers.begin(), std::find(dimers.begin(), dimers.end(), d.dimer));
      d.unique_index = idx;
    }
  }
  return result;
}

CrystalDimers Crystal::unit_cell_dimers(double radius) const {
  using occ::core::Dimer;
  CrystalDimers result;
  result.radius = radius;
  auto &dimers = result.unique_dimers;
  auto &mol_nbs = result.molecule_neighbors;

  HKL upper = HKL::minimum();
  HKL lower = HKL::maximum();
  occ::Vec3 frac_radius = radius * 2 / m_unit_cell.lengths().array();

  const auto &uc_mols = unit_cell_molecules();

  for (const auto &mol : uc_mols) {
    Mat3N pos_frac = to_fractional(mol.positions());
    for (size_t i = 0; i < pos_frac.cols(); i++) {
      const auto &pos = pos_frac.col(i);
      upper.h =
          std::max(upper.h, static_cast<int>(ceil(pos(0) + frac_radius(0))));
      upper.k =
          std::max(upper.k, static_cast<int>(ceil(pos(1) + frac_radius(1))));
      upper.l =
          std::max(upper.l, static_cast<int>(ceil(pos(2) + frac_radius(2))));

      lower.h =
          std::min(lower.h, static_cast<int>(floor(pos(0) - frac_radius(0))));
      lower.k =
          std::min(lower.k, static_cast<int>(floor(pos(1) - frac_radius(1))));
      lower.l =
          std::min(lower.l, static_cast<int>(floor(pos(2) - frac_radius(2))));
    }
  }

  mol_nbs.resize(uc_mols.size());

  for (int h = lower.h; h <= upper.h; h++) {
    for (int k = lower.k; k <= upper.k; k++) {
      for (int l = lower.l; l <= upper.l; l++) {
        occ::Vec3 cart_shift = to_cartesian(occ::Vec3{static_cast<double>(h),
                                                      static_cast<double>(k),
                                                      static_cast<double>(l)});
        int uc_idx_a = 0;
        for (const auto &uc_mol1 : uc_mols) {
          for (const auto &uc_mol2 : uc_mols) {
            auto mol_translated = uc_mol2.translated(cart_shift);
            mol_translated.set_cell_shift({h, k, l});
            double distance = std::get<2>(uc_mol1.nearest_atom(mol_translated));
            if ((distance < radius) && (distance > 1e-1)) {
              Dimer d(uc_mol1, mol_translated);
              mol_nbs[uc_idx_a].push_back({d, -1});
              if (std::any_of(dimers.begin(), dimers.end(),
                              [&d](const Dimer &d2) { return d == d2; }))
                continue;
              dimers.push_back(d);
            }
          }
          uc_idx_a++;
        }
      }
    }
  }

  auto dimer_sort_func = [](const Dimer &a, const Dimer &b) {
    return a.nearest_distance() < b.nearest_distance();
  };

  auto sort_func = [](const CrystalDimers::SymmetryRelatedDimer &a,
                      const CrystalDimers::SymmetryRelatedDimer &b) {
    return a.dimer.nearest_distance() < b.dimer.nearest_distance();
  };

  std::stable_sort(dimers.begin(), dimers.end(), dimer_sort_func);

  for (auto &vec : mol_nbs) {
    std::stable_sort(vec.begin(), vec.end(), sort_func);
    for (auto &d : vec) {
      size_t idx = std::distance(
          dimers.begin(), std::find(dimers.begin(), dimers.end(), d.dimer));
      d.unique_index = idx;
    }
  }
  return result;
}

Crystal Crystal::create_primitive_supercell(const Crystal &c, HKL hkl) {
  const auto &uc = c.unit_cell();
  auto supercell = UnitCell(uc.a() * hkl.h, uc.b() * hkl.k, uc.c() * hkl.l,
                            uc.alpha(), uc.beta(), uc.gamma());
  const auto &uc_mols = c.unit_cell_molecules();
  size_t natoms = std::accumulate(uc_mols.begin(), uc_mols.end(), 0,
                                  [](size_t a, const auto &mol) {
                                    return a + mol.size();
                                  }) *
                  hkl.h * hkl.k * hkl.l;
  Mat3N positions(3, natoms);
  IVec numbers(natoms);
  Vec3 t;
  size_t offset{0};
  for (int h = 0; h < hkl.h; h++) {
    for (int k = 0; k < hkl.k; k++) {
      for (int l = 0; l < hkl.l; l++) {
        for (const auto &uc_mol : uc_mols) {
          t = Vec3(h, k, l);
          size_t n = uc_mol.size();
          positions.block(0, offset, 3, n) =
              c.to_fractional(uc_mol.positions());
          positions.block(0, offset, 3, n).colwise() += t;
          numbers.block(offset, 0, n, 1) = uc_mol.atomic_numbers();
          offset += n;
        }
      }
    }
  }
  return Crystal(AsymmetricUnit(positions, numbers), SpaceGroup(1), supercell);
}

std::string
Crystal::dimer_symmetry_string(const occ::core::Dimer &dimer) const {
  const auto &a = dimer.a();
  const auto &b = dimer.b();
  if (a.asymmetric_molecule_idx() != b.asymmetric_molecule_idx())
    return "-";

  int sa_int = a.asymmetric_unit_symop()(0);
  int sb_int = b.asymmetric_unit_symop()(0);

  SymmetryOperation symop_a(sa_int);
  SymmetryOperation symop_b(sb_int);

  auto symop_ab = symop_b * symop_a.inverted();
  occ::Vec3 c_a = symop_ab(to_fractional(a.positions())).rowwise().mean();
  occ::Vec3 v_ab = to_fractional(b.centroid()) - c_a;

  symop_ab = symop_ab.translated(v_ab);
  return symop_ab.to_string();
}

double Crystal::volume() const { return m_unit_cell.volume(); }

int Crystal::normalize_hydrogen_bondlengths(
    const ankerl::unordered_dense::map<int, double>& custom_lengths) {

  int normalized_count = 0;

  // Get unit cell connectivity to find bonds
  const auto& bond_graph = unit_cell_connectivity();
  const auto& uc_atoms = unit_cell_atoms();

  // Work with asymmetric unit positions
  Mat3N cart_pos = to_cartesian(m_asymmetric_unit.positions);

  // Track which hydrogens we've already normalized
  ankerl::unordered_dense::set<int> normalized_h;

  // Iterate through all vertices and their neighbors in the bond graph
  const auto& adjacency_list = bond_graph.adjacency_list();
  const auto& vertices = bond_graph.vertices();
  const auto& edges = bond_graph.edges();

  for (const auto& [v1, neighbors] : adjacency_list) {
    // Get the first vertex's unit cell atom index
    auto v1_it = vertices.find(v1);
    if (v1_it == vertices.end()) continue;
    int uc_idx1 = v1_it->second.uc_idx;

    for (const auto& [v2, edge_desc] : neighbors) {
      // Only process covalent bonds, skip VDW contacts and hydrogen bonds
      const auto& edge = edges.at(edge_desc);
      if (edge.connectionType != occ::core::graph::PeriodicEdge::Connection::CovalentBond) {
        continue;
      }

      // Get the second vertex's unit cell atom index
      auto v2_it = vertices.find(v2);
      if (v2_it == vertices.end()) continue;
      int uc_idx2 = v2_it->second.uc_idx;

      // Map back to asymmetric unit indices
      if (uc_idx1 >= uc_atoms.asym_idx.size() || uc_idx2 >= uc_atoms.asym_idx.size()) {
        continue;
      }

      int asym_idx1 = uc_atoms.asym_idx(uc_idx1);
      int asym_idx2 = uc_atoms.asym_idx(uc_idx2);

      // Skip if indices are out of range
      if (asym_idx1 < 0 || asym_idx2 < 0 ||
          asym_idx1 >= m_asymmetric_unit.atomic_numbers.size() ||
          asym_idx2 >= m_asymmetric_unit.atomic_numbers.size()) {
        continue;
      }

      int h_idx = -1;
      int heavy_idx = -1;

      // Check if one is hydrogen and the other is not
      if (m_asymmetric_unit.atomic_numbers(asym_idx1) == 1 &&
          m_asymmetric_unit.atomic_numbers(asym_idx2) != 1) {
        h_idx = asym_idx1;
        heavy_idx = asym_idx2;
      } else if (m_asymmetric_unit.atomic_numbers(asym_idx2) == 1 &&
                 m_asymmetric_unit.atomic_numbers(asym_idx1) != 1) {
        h_idx = asym_idx2;
        heavy_idx = asym_idx1;
      } else {
        continue; // Not an H-X bond
      }

      // Skip if we've already normalized this hydrogen
      if (normalized_h.contains(h_idx)) {
        continue;
      }

      int heavy_atom_z = m_asymmetric_unit.atomic_numbers(heavy_idx);

      // Get target bond length
      double target_length = -1.0;
      if (custom_lengths.contains(heavy_atom_z)) {
        target_length = custom_lengths.at(heavy_atom_z);
      } else {
        target_length = StandardBondLengths::get_hydrogen_bond_length(heavy_atom_z);
      }

      // Skip if no standard length available
      if (target_length < 0) {
        continue;
      }

      // Calculate normalized position
      Vec3 bond_vector = cart_pos.col(h_idx) - cart_pos.col(heavy_idx);
      double current_length = bond_vector.norm();

      // Only normalize if the difference is significant (> 0.001 Å)
      if (std::abs(current_length - target_length) > 0.001) {
        bond_vector = (target_length / current_length) * bond_vector;
        cart_pos.col(h_idx) = cart_pos.col(heavy_idx) + bond_vector;
        normalized_h.insert(h_idx);
        normalized_count++;
      }
    }
  }

  // Convert back to fractional coordinates
  m_asymmetric_unit.positions = to_fractional(cart_pos);

  // After changing positions, mark all cached data as needing update
  m_unit_cell_atoms_needs_update = true;
  m_unit_cell_molecules_needs_update = true;
  m_unit_cell_connectivity_needs_update = true;
  m_symmetry_unique_molecules_needs_update = true;

  return normalized_count;
}

} // namespace occ::crystal
