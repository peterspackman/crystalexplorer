#include <iostream>
#include <occ/crystal/crystal.h>
#include <occ/crystal/dimer_mapping_table.h>

namespace occ::crystal {

DimerIndex
DimerMappingTable::symmetry_unique_dimer(const DimerIndex &dimer) const {
  auto it = m_symmetry_unique_dimer_map.find(dimer);
  if (it != m_symmetry_unique_dimer_map.end()) {
    return it->second;
  }
  // return dimer if not found
  return dimer;
}

std::vector<DimerIndex>
DimerMappingTable::symmetry_related_dimers(const DimerIndex &dimer) const {
  DimerIndex symmetry_unique = symmetry_unique_dimer(dimer);
  auto it = m_symmetry_related_dimers.find(symmetry_unique);
  if (it != m_symmetry_related_dimers.end()) {
    return it->second;
  }
  // return just dimer If not found
  return {dimer};
}

inline Vec3 clean_small_values(const Vec3 &v, double epsilon = 1e-14) {
  return v.unaryExpr(
      [epsilon](double x) { return (std::abs(x) < epsilon) ? 0.0 : x; });
}

inline Vec3 wrap_to_unit_cell(const Vec3 &v) {
  return Vec3(v.array() - v.array().floor());
}

inline SiteIndex find_matching_position(const Mat3N &positions,
                                        const Vec3 &point,
                                        double tolerance = 1e-5) {
  int matching_index = -1;
  double min_distance = std::numeric_limits<double>::max();
  Vec3 cell_offset = Vec3::Zero();

  for (int i = 0; i < positions.cols(); i++) {
    Vec3 diff = point - positions.col(i);
    Vec3 wrapped_diff = diff.array() - diff.array().round();
    Vec3 current_cell_offset = diff - wrapped_diff;

    double d = wrapped_diff.squaredNorm();
    if (d < min_distance) {
      min_distance = d;
      matching_index = i;
      cell_offset = current_cell_offset;
    }
  }

  if (min_distance < tolerance * tolerance) {
    return SiteIndex{matching_index,
                     HKL{static_cast<int>(std::round(cell_offset(0))),
                         static_cast<int>(std::round(cell_offset(1))),
                         static_cast<int>(std::round(cell_offset(2)))}};
  }
  return SiteIndex{-1, HKL{0, 0, 0}};
}

std::pair<Vec3, Vec3>
DimerMappingTable::dimer_positions(const core::Dimer &dimer) const {
  Vec3 a_pos = m_cell.to_fractional(dimer.a().centroid());
  Vec3 b_pos = m_cell.to_fractional(dimer.b().centroid());
  return {a_pos, b_pos};
}

DimerIndex DimerMappingTable::dimer_index(const core::Dimer &dimer) const {
  auto [a_pos, b_pos] = dimer_positions(dimer);
  SiteIndex a = find_matching_position(m_centroids, a_pos);
  SiteIndex b = find_matching_position(m_centroids, b_pos);
  return DimerIndex{a, b};
}

DimerIndex DimerMappingTable::dimer_index(const Vec3 &a_pos,
                                          const Vec3 &b_pos) const {
  SiteIndex a = find_matching_position(m_centroids, a_pos);
  SiteIndex b = find_matching_position(m_centroids, b_pos);
  return DimerIndex{a, b};
}

DimerIndex DimerMappingTable::normalized_dimer_index(const DimerIndex &idx) {
  DimerIndex normalized = idx;
  normalized.b.hkl -= idx.a.hkl;
  normalized.a.hkl = HKL{0, 0, 0};
  return normalized;
}

DimerIndex
DimerMappingTable::canonical_dimer_index(const DimerIndex &idx) const {
  DimerIndex normalized = normalized_dimer_index(idx);
  if (m_consider_inversion) {
    DimerIndex inverted = normalized_dimer_index(DimerIndex{idx.b, idx.a});
    return (normalized < inverted) ? normalized : inverted;
  }
  return normalized;
}

DimerMappingTable::DimerMappingTable(const Crystal &crystal,
                                     const CrystalDimers &dimers,
                                     bool consider_inversion) {
  m_consider_inversion = consider_inversion;
  m_cell = crystal.unit_cell();

  const auto &uc_mols = crystal.unit_cell_molecules();
  m_centroids = Mat3N(3, uc_mols.size());
  for (int i = 0; i < uc_mols.size(); i++) {
    m_centroids.col(i) = crystal.to_fractional(uc_mols[i].centroid());
  }

  std::cout << "Centroids\n" << m_centroids.transpose() << '\n';

  const auto &symops = crystal.symmetry_operations();
  ankerl::unordered_dense::set<DimerIndex, DimerIndexHash> unique_dimers_set;

  for (const auto &mol_dimers : dimers.molecule_neighbors) {
    for (const auto &[dimer, asym_idx] : mol_dimers) {

      auto [a_pos, b_pos] = dimer_positions(dimer);
      DimerIndex ab = dimer_index(dimer);
      DimerIndex norm_ab = normalized_dimer_index(ab);
      DimerIndex canonical_ab = canonical_dimer_index(ab);

      if (unique_dimers_set.insert(canonical_ab).second) {
        m_unique_dimers.push_back(canonical_ab);
        m_unique_dimer_map[canonical_ab] = canonical_ab;
        m_symmetry_unique_dimer_map[canonical_ab] = canonical_ab;

        std::vector<DimerIndex> related_dimers;
        for (const auto &symop : symops) {
          Vec3 ta = symop.apply(a_pos);
          Vec3 tb = symop.apply(b_pos);
          DimerIndex symmetry_ab = dimer_index(ta, tb);
          DimerIndex canonical_symmetry_ab = canonical_dimer_index(symmetry_ab);

          if (unique_dimers_set.insert(canonical_symmetry_ab).second) {
            m_unique_dimers.push_back(canonical_symmetry_ab);
            m_symmetry_unique_dimer_map[canonical_symmetry_ab] = canonical_ab;
          }
          related_dimers.push_back(canonical_symmetry_ab);
          m_unique_dimer_map[canonical_symmetry_ab] = canonical_symmetry_ab;
        }
        m_symmetry_related_dimers[canonical_ab] = related_dimers;
      }

      // Always map both ab and normalized ab to the canonical dimer
      m_unique_dimer_map[ab] = canonical_ab;
      m_unique_dimer_map[norm_ab] = canonical_ab;
      m_symmetry_unique_dimer_map[ab] =
          m_symmetry_unique_dimer_map[canonical_ab];
      m_symmetry_unique_dimer_map[norm_ab] =
          m_symmetry_unique_dimer_map[canonical_ab];
    }
  }

  // Populate m_symmetry_unique_dimers
  for (const auto &dimer : m_unique_dimers) {
    if (m_symmetry_unique_dimer_map[dimer] == dimer) {
      m_symmetry_unique_dimers.push_back(dimer);
    }
  }
}

bool DimerMappingTable::have_dimer(const DimerIndex &dimer) const {
  DimerIndex canonical = canonical_dimer_index(dimer);
  return m_unique_dimer_map.find(canonical) != m_unique_dimer_map.end();
}

DimerMappingTable
DimerMappingTable::create_atomic_pair_table(const Crystal &crystal,
                                            bool consider_inversion) {
  DimerMappingTable table;
  table.m_consider_inversion = consider_inversion;
  table.m_cell = crystal.unit_cell();

  // Get expanded slab like in unit_cell_connectivity
  auto s = crystal.slab({-2, -2, -2}, {2, 2, 2});
  const auto &uc_atoms = crystal.unit_cell_atoms();
  table.m_centroids = uc_atoms.frac_pos;

  const auto &symops = crystal.symmetry_operations();
  ankerl::unordered_dense::set<DimerIndex, DimerIndexHash> unique_dimers_set;

  // For each atom in the unit cell
  for (int i = 0; i < uc_atoms.size(); i++) {
    Vec3 pos_i = uc_atoms.frac_pos.col(i);

    // Look through expanded slab for possible bonds
    for (int j = 0; j < s.frac_pos.cols(); j++) {
      if (j % uc_atoms.size() <= i)
        continue; // avoid duplicates

      Vec3 pos_j = s.frac_pos.col(j);
      int uc_idx_j = s.uc_idx(j);
      HKL cell_offset{s.hkl(0, j), s.hkl(1, j), s.hkl(2, j)};

      DimerIndex ab{{i, {0, 0, 0}}, {uc_idx_j, cell_offset}};
      DimerIndex canonical_ab = table.canonical_dimer_index(ab);

      if (unique_dimers_set.insert(canonical_ab).second) {
        table.m_unique_dimers.push_back(canonical_ab);
        table.m_unique_dimer_map[canonical_ab] = canonical_ab;
        table.m_symmetry_unique_dimer_map[canonical_ab] = canonical_ab;

        std::vector<DimerIndex> related_dimers;
        for (const auto &symop : symops) {
          Vec3 ta = symop.apply(pos_i);
          Vec3 tb = symop.apply(pos_j);
          DimerIndex symmetry_ab = table.dimer_index(ta, tb);
          DimerIndex canonical_symmetry_ab =
              table.canonical_dimer_index(symmetry_ab);

          if (unique_dimers_set.insert(canonical_symmetry_ab).second) {
            table.m_unique_dimers.push_back(canonical_symmetry_ab);
            table.m_symmetry_unique_dimer_map[canonical_symmetry_ab] =
                canonical_ab;
          }
          related_dimers.push_back(canonical_symmetry_ab);
          table.m_unique_dimer_map[canonical_symmetry_ab] =
              canonical_symmetry_ab;
        }
        table.m_symmetry_related_dimers[canonical_ab] = related_dimers;
      }

      table.m_unique_dimer_map[ab] = canonical_ab;
      DimerIndex norm_ab = table.normalized_dimer_index(ab);
      table.m_unique_dimer_map[norm_ab] = canonical_ab;
      table.m_symmetry_unique_dimer_map[ab] =
          table.m_symmetry_unique_dimer_map[canonical_ab];
      table.m_symmetry_unique_dimer_map[norm_ab] =
          table.m_symmetry_unique_dimer_map[canonical_ab];
    }
  }

  // Populate unique dimers as before
  for (const auto &dimer : table.m_unique_dimers) {
    if (table.m_symmetry_unique_dimer_map[dimer] == dimer) {
      table.m_symmetry_unique_dimers.push_back(dimer);
    }
  }

  return table;
}
} // namespace occ::crystal

auto fmt::formatter<occ::crystal::DimerIndex>::format(
    const occ::crystal::DimerIndex &idx, format_context &ctx) const
    -> decltype(ctx.out()) {
  return fmt::format_to(ctx.out(), "DimerIndex [{} {} -> {} {}]", idx.a.offset,
                        idx.a.hkl, idx.b.offset, idx.b.hkl);
}
