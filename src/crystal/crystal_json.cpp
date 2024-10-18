#include "crystal_json.h"
#include "json.h"
#include "eigen_json.h"

namespace occ::crystal {

void to_json(nlohmann::json &j, const UnitCell &uc) {
  j["direct_matrix"] = uc.direct();
  j["reciprocal_matrix"] = uc.reciprocal();
}

void from_json(const nlohmann::json &j, UnitCell &uc) {
  uc = UnitCell(j["direct_matrix"].get<Mat3>());
}

void to_json(nlohmann::json &j, const AsymmetricUnit &asym) {
  j["site count"] = asym.atomic_numbers.rows();
  j["labels"] = asym.labels;
  j["atomic numbers"] = asym.atomic_numbers.transpose();
  j["positions"] = asym.positions;
  if (asym.occupations.rows() > 0)
    j["occupations"] = asym.occupations.transpose();
  if (asym.charges.rows() > 0)
    j["charges"] = asym.charges.transpose();
}

void from_json(const nlohmann::json &j, AsymmetricUnit &asym) {
  asym.atomic_numbers = j.at("atomic numbers").get<IVec>();
  asym.positions = j.at("positions").get<Mat3N>();
  asym.labels = j.at("labels");

  if (j.contains("occupations")) {
    asym.occupations = j.at("occupations").get<Vec>();
  }
  if (j.contains("charges")) {
    asym.charges = j.at("charges").get<Vec>();
  }
}

void to_json(nlohmann::json &j, const CrystalAtomRegion &region) {
  j["site count"] = region.size();
  j["fractional positions"] = region.frac_pos;
  j["cartesian positions"] = region.cart_pos;
  j["asymmetric atom index"] = region.asym_idx.transpose();
  j["unit cell index"] = region.uc_idx.transpose();
  j["unit cell offset"] = region.hkl;
  j["atomic numbers"] = region.atomic_numbers.transpose();
  j["symmetry operation"] = region.symop.transpose();
  j["disorder group"] = region.disorder_group.transpose();
}

void from_json(const nlohmann::json &j, CrystalAtomRegion &region) {
  size_t size = j["site count"].get<size_t>();
  region.resize(size);

  region.frac_pos = j["fractional positions"].get<Mat3N>();
  region.cart_pos = j["cartesian positions"].get<Mat3N>();
  region.asym_idx = j["asymmetric atom index"].get<IVec>();
  region.hkl = j["unit cell offset"].get<IMat3N>();
  region.uc_idx = j["unit cell index"].get<IVec>();
  region.atomic_numbers = j["atomic numbers"].get<IVec>();
  region.symop = j["symmetry operation"].get<IVec>();
  region.disorder_group = j["disorder group"].get<IVec>();
}

} // namespace occ::crystal

namespace nlohmann {

occ::crystal::SymmetryOperation
adl_serializer<occ::crystal::SymmetryOperation>::from_json(const json &j) {
  return occ::crystal::SymmetryOperation(j.at("integer_code").get<int>());
}

void adl_serializer<occ::crystal::SymmetryOperation>::to_json(
    json &j, const occ::crystal::SymmetryOperation &s) {
  j = json{{"seitz", s.seitz()},
           {"integer_code", s.to_int()},
           {"string_code", s.to_string()}};
}

occ::crystal::SpaceGroup
adl_serializer<occ::crystal::SpaceGroup>::from_json(const json &j) {
  return occ::crystal::SpaceGroup(j.at("symbol").get<std::string>());
}

void adl_serializer<occ::crystal::SpaceGroup>::to_json(
    json &j, const occ::crystal::SpaceGroup &sg) {
  j["symbol"] = sg.symbol();
  j["short name"] = sg.short_name();
  j["number"] = sg.number();
  nlohmann::json symops;
  for (const auto &symop : sg.symmetry_operations()) {
    symops.push_back(symop);
  }
  j["symmetry_operations"] = symops;
}

occ::crystal::Crystal
adl_serializer<occ::crystal::Crystal>::from_json(const json &j) {
  auto asym = j.at("asymmetric unit").get<occ::crystal::AsymmetricUnit>();
  auto sg = j.at("space group").get<occ::crystal::SpaceGroup>();
  auto uc = j.at("unit cell").get<occ::crystal::UnitCell>();
  return occ::crystal::Crystal(asym, sg, uc);
}

void adl_serializer<occ::crystal::Crystal>::to_json(
    json &j, const occ::crystal::Crystal &crystal) {
  j["asymmetric unit"] = crystal.asymmetric_unit();
  j["space group"] = crystal.space_group();
  j["unit cell"] = crystal.unit_cell();
  j["unit cell atoms"] = crystal.unit_cell_atoms();
  const auto &connectivity = crystal.unit_cell_connectivity();
  json connections;
  json edges;
  connections["number of edges"] = connectivity.num_edges();
  for (const auto &[edge_descriptor, edge] : connectivity.edges()) {
    nlohmann::json e;
    e["distance"] = edge.dist;
    e["source"] = edge.source;
    e["target"] = edge.target;
    e["source asym"] = edge.source_asym_idx;
    e["target asym"] = edge.target_asym_idx;
    json shift;
    shift.push_back(edge.h);
    shift.push_back(edge.k);
    shift.push_back(edge.l);
    e["shift"] = shift;
    edges.push_back(e);
  }
  connections["edges"] = edges;

  j["unit cell connectivity"] = connections;
}
} // namespace nlohmann
