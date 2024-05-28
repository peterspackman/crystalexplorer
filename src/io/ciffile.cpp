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
  int residueNumber{-1};
  std::array<double, 3> position;
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

struct CifCrystalData {
  std::vector<CifAtomData> atoms;
  CifCellData cellData;
  CifSymmetryData symmetryData;
  QByteArray cifContents;
  QString name{"crystal"};

  bool isValid() const {
    return cellData.isValid() && symmetryData.isValid() && atoms.size() > 0;
  }
};

std::vector<CifAtomData> extractAtomSites(const gemmi::cif::Loop &loop) {
  int labelIndex = loop.find_tag("_atom_site_label");
  int symbolIndex = loop.find_tag("_atom_site_type_symbol");
  int xIndex = loop.find_tag("_atom_site_fract_x");
  int yIndex = loop.find_tag("_atom_site_fract_y");
  int zIndex = loop.find_tag("_atom_site_fract_z");
  std::vector<CifAtomData> result;
  for (size_t i = 0; i < loop.length(); i++) {
    CifAtomData atom;
    bool some_info_found = false;
    if (labelIndex > -1) {
      atom.siteLabel = loop.val(i, labelIndex);
      some_info_found = true;
    }
    if (symbolIndex > -1) {
      atom.element = loop.val(i, symbolIndex);
      some_info_found = true;
    }
    if (xIndex > -1) {
      atom.position[0] = gemmi::cif::as_number(loop.val(i, xIndex));
      some_info_found = true;
    }
    if (yIndex > -1) {
      atom.position[1] = gemmi::cif::as_number(loop.val(i, yIndex));
      some_info_found = true;
    }
    if (zIndex > -1) {
      atom.position[2] = gemmi::cif::as_number(loop.val(i, zIndex));
      some_info_found = true;
    }
    if (some_info_found) {
      if (atom.element.empty())
        atom.element = atom.siteLabel;
      result.push_back(atom);
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

QByteArray blockToQByteArray(const gemmi::cif::Block& block, gemmi::cif::WriteOptions options = gemmi::cif::WriteOptions()) {
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
    for (const auto &item : block.items) { switch (item.type) {
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
          cifData.atoms = extractAtomSites(item.loop);
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
buildAsymmetricUnit(const std::vector<CifAtomData> &atoms) {
  occ::crystal::AsymmetricUnit result;
  size_t numAtoms = atoms.size();
  result.atomic_numbers = occ::IVec(numAtoms);
  result.positions = occ::Mat3N(3, numAtoms);
  for (int i = 0; i < atoms.size(); i++) {
    const auto &atom = atoms[i];
    result.positions.col(i) = Eigen::Map<const occ::Vec3>(atom.position.data());
    result.atomic_numbers(i) = occ::core::Element(atom.element).atomic_number();
    result.labels.push_back(atom.siteLabel);
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
        occ::crystal::Crystal(buildAsymmetricUnit(crystal.atoms),
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
        occ::crystal::Crystal(buildAsymmetricUnit(crystal.atoms),
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

const QByteArray& CifFile::getCrystalCifContents(int index) const {
    return m_crystalCifContents.at(index);
}

const QString& CifFile::getCrystalName(int index) const {
    return m_crystalNames.at(index);
}
