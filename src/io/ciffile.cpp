#include "ciffile.h"
#include <QByteArray>

#include <algorithm>
#include <array>
#include <gemmi/cif.hpp>
#include <gemmi/numb.hpp>
#include <gemmi/to_cif.hpp>

struct CifAtomData {
  std::string element;
  std::string siteLabel;
  std::string residueName;
  std::string chainId;
  std::string adp_type;
  int residueNumber{-1};
  std::array<double, 3> position;
  double uiso{0.0};
};

struct AdpData {
  std::string anisoLabel;
  double u11{0.0};
  double u22{0.0};
  double u33{0.0};
  double u12{0.0};
  double u13{0.0};
  double u23{0.0};
};

struct CifCellData {
  std::array<double, 3> lengths;
  std::array<double, 3> angles;

  inline bool isValid() const {
    auto isPositive = [](double x) { return x > 0.0; };
    return std::all_of(lengths.begin(), lengths.end(), isPositive) &&
           std::all_of(angles.begin(), angles.end(), isPositive);
  }
};

struct CifSymmetryData {
  int number{-1};
  std::string HM{""};
  std::string Hall{""};
  std::vector<std::string> symmetryOperations;

  inline bool isValid() const {
    if (number > 0)
      return true;
    if (!HM.empty())
      return true;
    if (!Hall.empty())
      return true;
    if (symmetryOperations.size() > 0)
      return true;
    return false;
  }
};

using AdpMap = ankerl::unordered_dense::map<std::string, AdpData>;

struct CifCrystalData {
  std::vector<CifAtomData> atoms;
  AdpMap adps;
  CifCellData cellData;
  CifSymmetryData symmetryData;
  QByteArray cifContents;
  QString name{"crystal"};

  bool isValid() const {
    return cellData.isValid() && symmetryData.isValid() && atoms.size() > 0;
  }
};

enum class AtomField {
  Ignore,
  Label,
  Element,
  FracX,
  FracY,
  FracZ,
  AdpType,
  Uiso,
  AdpLabel,
  AdpU11,
  AdpU22,
  AdpU33,
  AdpU12,
  AdpU13,
  AdpU23
};

static const ankerl::unordered_dense::map<std::string, AtomField>
    known_atom_fields{
        {"_atom_site_label", AtomField::Label},
        {"_atom_site_type_symbol", AtomField::Element},
        {"_atom_site_fract_x", AtomField::FracX},
        {"_atom_site_fract_y", AtomField::FracY},
        {"_atom_site_fract_z", AtomField::FracZ},
        {"_atom_site_adp_type", AtomField::AdpType},
        {"_atom_site_u_iso_or_equiv", AtomField::Uiso},
        {"_atom_site_aniso_label", AtomField::AdpLabel},
        {"_atom_site_aniso_u_11", AtomField::AdpU11},
        {"_atom_site_aniso_u_22", AtomField::AdpU22},
        {"_atom_site_aniso_u_33", AtomField::AdpU33},
        {"_atom_site_aniso_u_12", AtomField::AdpU12},
        {"_atom_site_aniso_u_13", AtomField::AdpU13},
        {"_atom_site_aniso_u_23", AtomField::AdpU23},
    };

inline void setAtomData(int index, const std::vector<AtomField> &fields,
                        const gemmi::cif::Loop &loop, CifAtomData &atom,
                        AdpData &adp) {
  using gemmi::cif::as_number;
  using enum AtomField;

  for (int field_index = 0; field_index < fields.size(); field_index++) {
    const auto &field = fields[field_index];
    const auto &value = loop.val(index, field_index);
    switch (field) {
    case Label:
      atom.siteLabel = value;
      break;
    case Element:
      atom.element = value;
      break;
    case FracX:
      atom.position[0] = as_number(value);
      break;
    case FracY:
      atom.position[1] = as_number(value);
      break;
    case FracZ:
      atom.position[2] = as_number(value);
      break;
    case AdpType:
      atom.adp_type = value;
      break;
    case Uiso:
      atom.uiso = as_number(value);
      break;
    case AdpLabel:
      adp.anisoLabel = value;
      break;
    case AdpU11:
      adp.u11 = as_number(value);
      break;
    case AdpU22:
      adp.u22 = as_number(value);
      break;
    case AdpU33:
      adp.u33 = as_number(value);
      break;
    case AdpU12:
      adp.u12 = as_number(value);
      break;
    case AdpU13:
      adp.u13 = as_number(value);
      break;
    case AdpU23:
      adp.u23 = as_number(value);
      break;
    default:
      break;
    }
  }
}

std::vector<CifAtomData> extractAtomSites(const gemmi::cif::Loop &loop,
                                          AdpMap &adps) {
  std::vector<CifAtomData> result;
  std::vector<AtomField> fields(loop.tags.size(), AtomField::Ignore);

  bool found_info = false;
  // Map tags to fields
  for (size_t i = 0; i < loop.tags.size(); ++i) {
    const auto tag = occ::util::to_lower_copy(loop.tags[i]);
    const auto kv = known_atom_fields.find(tag);
    if (kv != known_atom_fields.end()) {
      fields[i] = kv->second;
      found_info = true;
    }
  }

  if (!found_info)
    return {};

  result.resize(loop.length());
  for (size_t i = 0; i < loop.length(); i++) {
    AdpData adp;
    auto &atom = result[i];
    setAtomData(i, fields, loop, atom, adp);
    if (atom.element.empty())
      atom.element = atom.siteLabel;

    if (!adp.anisoLabel.empty()) {
      adps.insert({adp.anisoLabel, adp});
    }
  }
  return result;
}

void extractCellParameter(const gemmi::cif::Pair &pair,
                          CifCellData &destination) {
  const auto &tag = pair.front();
  if (tag == "_cell_length_a")
    destination.lengths[0] = gemmi::cif::as_number(pair.back());
  else if (tag == "_cell_length_b")
    destination.lengths[1] = gemmi::cif::as_number(pair.back());
  else if (tag == "_cell_length_c")
    destination.lengths[2] = gemmi::cif::as_number(pair.back());
  else if (tag == "_cell_angle_alpha")
    destination.angles[0] =
        occ::units::radians(gemmi::cif::as_number(pair.back()));
  else if (tag == "_cell_angle_beta")
    destination.angles[1] =
        occ::units::radians(gemmi::cif::as_number(pair.back()));
  else if (tag == "_cell_angle_gamma")
    destination.angles[2] =
        occ::units::radians(gemmi::cif::as_number(pair.back()));
}

void removeQuotes(std::string &s) {
  const auto &front = s.front();
  if (front == '"' || front == '\'' || front == '`') {
    s.erase(0, 1);
  }
  const auto &back = s.back();
  if (back == '"' || back == '\'' || back == '`') {
    s.erase(s.size() - 1);
  }
}

std::vector<std::string>
extractSymmetryOperations(const gemmi::cif::Loop &loop) {
  int index = loop.find_tag("_symmetry_equiv_pos_as_xyz");
  if (index < 0)
    index = loop.find_tag("_space_group_symop_operation_xyz");
  if (index < 0)
    return {};
  std::vector<std::string> result;
  for (size_t i = 0; i < loop.length(); i++) {
    result.emplace_back(gemmi::cif::as_string(loop.val(i, index)));
  }
  return result;
}

void extractSymmetryData(const gemmi::cif::Pair &pair,
                         CifSymmetryData &destination) {
  const auto &tag = occ::util::to_lower_copy(pair.front());
  if (tag == "_symmetry_space_group_name_hall")
    destination.Hall = gemmi::cif::as_string(pair.back());
  else if (tag == "_symmetry_space_group_name_h-m")
    destination.HM = gemmi::cif::as_string(pair.back());
  else if (tag == "_space_group_it_number" ||
           tag == "_symmetry_int_tables_number")
    destination.number = gemmi::cif::as_int(pair.back());

  auto cleanUpString = [](std::string &s) {
    if (s.find('_') != std::string::npos)
      occ::util::remove_character_occurences(s, '_');
  };
  cleanUpString(destination.Hall);
  cleanUpString(destination.HM);
}

QByteArray blockToQByteArray(
    const gemmi::cif::Block &block,
    gemmi::cif::WriteOptions options = gemmi::cif::WriteOptions()) {
  std::stringstream ss;
  gemmi::cif::write_cif_block_to_stream(ss, block, options);
  return QByteArray::fromStdString(ss.str());
}

std::vector<CifCrystalData> readDocument(gemmi::cif::Document &document) {
  std::vector<CifCrystalData> result;
  int blockNumber = 0;
  for (const auto &block : document.blocks) {
    CifCrystalData cifData;

    cifData.cifContents = blockToQByteArray(block);
    cifData.name = QString::fromStdString(block.name);
    for (const auto &item : block.items) {
      switch (item.type) {
      case gemmi::cif::ItemType::Pair:
        if (item.has_prefix("_cell")) {
          extractCellParameter(item.pair, cifData.cellData);
        } else if (item.has_prefix("_symmetry") ||
                   item.has_prefix("_space_group")) {
          extractSymmetryData(item.pair, cifData.symmetryData);
        }
        break;
      case gemmi::cif::ItemType::Loop:
        if (item.has_prefix("_atom_site_")) {
          if (cifData.atoms.size() != 0)
            continue;
          cifData.atoms = extractAtomSites(item.loop, cifData.adps);
        } else if (item.has_prefix("_symmetry_equiv_pos") ||
                   item.has_prefix("_space_group_symop")) {
          cifData.symmetryData.symmetryOperations =
              extractSymmetryOperations(item.loop);
        }
        break;
      default:
        continue;
      }
    }
    if (cifData.isValid()) {
      result.push_back(cifData);
    } else {
      qDebug() << "Invalid crystal in block" << blockNumber;
      if (!cifData.cellData.isValid()) {
        qDebug() << "Reason: invalid Cell data";
      } else if (!cifData.symmetryData.isValid()) {
        qDebug() << "Reason: invalid symmetry data";
      } else if (cifData.atoms.size() == 0) {
        qDebug() << "Reason: no atom sites read";
      }
    }
    blockNumber++;
  }
  return result;
}

occ::crystal::AsymmetricUnit
buildAsymmetricUnit(const std::vector<CifAtomData> &atoms, const AdpMap &adps) {
  occ::crystal::AsymmetricUnit result;
  size_t numAtoms = atoms.size();
  result.atomic_numbers = occ::IVec(numAtoms);
  result.positions = occ::Mat3N(3, numAtoms);
  result.adps.resize(6, numAtoms);
  for (int i = 0; i < atoms.size(); i++) {
    const auto &atom = atoms[i];
    result.positions.col(i) = Eigen::Map<const occ::Vec3>(atom.position.data());
    result.atomic_numbers(i) = occ::core::Element(atom.element).atomic_number();
    result.labels.push_back(atom.siteLabel);
    // set Uiso in case we don't have an adp, by default should be 0 anyway;
    result.adps(0, i) = atom.uiso;
    result.adps(1, i) = atom.uiso;
    result.adps(2, i) = atom.uiso;

    const auto kv = adps.find(atom.siteLabel);
    if (kv != adps.end()) {
      const auto &adp = kv->second;
      result.adps(0, i) = adp.u11;
      result.adps(1, i) = adp.u22;
      result.adps(2, i) = adp.u33;
      result.adps(3, i) = adp.u12;
      result.adps(4, i) = adp.u13;
      result.adps(5, i) = adp.u23;
    }
  }
  return result;
}

occ::crystal::UnitCell buildUnitCell(const CifCellData &cellData) {
  return occ::crystal::UnitCell(cellData.lengths[0], cellData.lengths[1],
                                cellData.lengths[2], cellData.angles[0],
                                cellData.angles[1], cellData.angles[2]);
}

occ::crystal::SpaceGroup buildSpacegroup(const CifSymmetryData &symmetryData) {
  if (!symmetryData.isValid()) {
    qDebug() << "Symmetry data not valid, unable to determine space group from "
                "CIF, using P1";
    return occ::crystal::SpaceGroup(1);
  }
  if (!symmetryData.HM.empty()) {
    const auto *sgdata = gemmi::find_spacegroup_by_name(symmetryData.HM);
    if (sgdata) {
      return occ::crystal::SpaceGroup(symmetryData.HM);
    }
  }
  if (!symmetryData.Hall.empty()) {
    const auto *sgdata = gemmi::find_spacegroup_by_name(symmetryData.Hall);
    if (sgdata) {
      return occ::crystal::SpaceGroup(symmetryData.Hall);
    }
  }
  if (symmetryData.symmetryOperations.size() > 0) {
    gemmi::GroupOps ops;
    for (const auto &symop : symmetryData.symmetryOperations) {
      ops.sym_ops.push_back(gemmi::parse_triplet(symop));
    }
    const auto *sgdata = gemmi::find_spacegroup_by_ops(ops);
    if (sgdata) {
      return occ::crystal::SpaceGroup(symmetryData.symmetryOperations);
    }
  }
  if (symmetryData.number > 0) {
    const auto *sgdata = gemmi::find_spacegroup_by_number(symmetryData.number);
    if (sgdata) {
      return occ::crystal::SpaceGroup(symmetryData.number);
    }
  }
  qDebug() << "Valid symmetry data, but unable to determine space group from "
              "CIF, using P1";
  return occ::crystal::SpaceGroup(1);
}

bool CifFile::readFromFile(const QString &fileName) {
  gemmi::cif::Document document;
  try {
    document = gemmi::cif::read_file(fileName.toStdString());
  } catch (std::runtime_error &e) {
    qDebug() << "Error reading cif" << e.what();
    return false;
  }
  std::vector<CifCrystalData> crystals = readDocument(document);
  for (const auto &crystal : crystals) {
    m_crystals.emplace_back(
        occ::crystal::Crystal(buildAsymmetricUnit(crystal.atoms, crystal.adps),
                              buildSpacegroup(crystal.symmetryData),
                              buildUnitCell(crystal.cellData)));
    m_crystalCifContents.push_back(crystal.cifContents);
    m_crystalNames.push_back(crystal.name);
  }
  return true;
}

bool CifFile::readFromString(const QString &content) {
  gemmi::cif::Document document;
  try {
    document = gemmi::cif::read_string(content.toStdString());
  } catch (std::runtime_error &e) {
    qDebug() << "Error reading cif" << e.what();
    return false;
  }
  std::vector<CifCrystalData> crystals = readDocument(document);
  for (const auto &crystal : crystals) {
    m_crystals.emplace_back(
        occ::crystal::Crystal(buildAsymmetricUnit(crystal.atoms, crystal.adps),
                              buildSpacegroup(crystal.symmetryData),
                              buildUnitCell(crystal.cellData)));
    m_crystalCifContents.push_back(crystal.cifContents);
    m_crystalNames.push_back(crystal.name);
  }
  return true;
}

int CifFile::numberOfCrystals() const { return m_crystals.size(); }

const OccCrystal &CifFile::getCrystalStructure(int index) const {
  return m_crystals.at(index);
}

const QByteArray &CifFile::getCrystalCifContents(int index) const {
  return m_crystalCifContents.at(index);
}

const QString &CifFile::getCrystalName(int index) const {
  return m_crystalNames.at(index);
}
