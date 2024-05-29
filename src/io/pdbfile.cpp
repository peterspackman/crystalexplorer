#include "pdbfile.h"
#include <QByteArray>

#include <QDebug>
#include <algorithm>
#include <array>
#include <occ/core/units.h>
#include <gemmi/pdb.hpp>
#include <gemmi/cif.hpp>
#include <gemmi/to_cif.hpp>

struct PdbAtomData {
  std::string element;
  std::string siteLabel;
  std::string residueName;
  std::string chainId;
  int residueNumber{-1};
  std::array<double, 3> position;
};

struct PdbCellData {
  std::array<double, 3> lengths;
  std::array<double, 3> angles;

  inline bool isValid() const {
    auto isPositive = [](double x) { return x > 0.0; };
    return std::all_of(lengths.begin(), lengths.end(), isPositive) &&
    std::all_of(angles.begin(), angles.end(), isPositive);
  }
};

struct PdbCrystalData {
  std::vector<PdbAtomData> atoms;
  gemmi::UnitCell cell;
  const gemmi::SpaceGroup * spaceGroup{nullptr};
  QByteArray cifContents;
  QString name{"crystal"};

  bool isValid() const {
    return (spaceGroup != nullptr) && atoms.size() > 0;
  }
};


std::vector<PdbAtomData> extractAtoms(const gemmi::Model &model) {
  std::vector<PdbAtomData> atoms;
  // Add atoms to the crystal structure
  for (const auto& chain : model.chains) {
    for (const auto& residue : chain.residues) {
      for (const auto &atom : residue.atoms) {
        atoms.push_back(PdbAtomData{
          atom.element.name(),
          atom.name,
          residue.name,
          chain.name,
          residue.group_idx,
          {atom.pos.x, atom.pos.y, atom.pos.z}
        });
      }
    }
  }
  return atoms;
}

std::vector<PdbCrystalData> readStructure(gemmi::Structure &structure) {
  std::vector<PdbCrystalData> result;
  int blockNumber = 0;
  const auto & cell = structure.cell;
  const auto * sg = structure.find_spacegroup();
  for (const auto &model : structure.models) {
    PdbCrystalData pdbData{{}, cell, sg, {}, QString::fromStdString(model.name)};

    pdbData.atoms = extractAtoms(model);

    if (pdbData.isValid()) {
      result.push_back(pdbData);
    } else {
      qDebug() << "Invalid crystal in block" << blockNumber;
      if (pdbData.spaceGroup == nullptr) {
        qDebug() << "Reason: invalid symmetry data";
      } else if (pdbData.atoms.size() == 0) {
        qDebug() << "Reason: no atom sites read";
      }
    }
    blockNumber++;
  }
  return result;
}

occ::crystal::AsymmetricUnit
buildAsymmetricUnit(const std::vector<PdbAtomData> &atoms, const occ::crystal::UnitCell &cell) {
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
  result.positions = cell.to_fractional(result.positions);
  return result;
}

occ::crystal::UnitCell buildUnitCell(const gemmi::UnitCell &cell) {
  return occ::crystal::UnitCell(cell.a, cell.b, cell.c, 
                                occ::units::radians(cell.alpha),
                                occ::units::radians(cell.beta),
                                occ::units::radians(cell.gamma));
}

occ::crystal::SpaceGroup buildSpacegroup(const gemmi::SpaceGroup * sg) {
  if (!sg) {
    qDebug() << "Symmetry data not valid, unable to determine space group from "
      "PDB, using P1";
    return occ::crystal::SpaceGroup(1);
  }
  return occ::crystal::SpaceGroup(sg->xhm());
}

bool PdbFile::readFromFile(const QString &fileName) {
  gemmi::Structure structure;
  try {
    structure = gemmi::read_pdb_file(fileName.toStdString());
  } catch (std::runtime_error &e) {
    qDebug() << "Error reading cif" << e.what();
    return false;
  }
  std::vector<PdbCrystalData> crystals = readStructure(structure);
  for (const auto &crystal : crystals) {
    auto uc = buildUnitCell(crystal.cell);
    m_crystals.emplace_back(
      occ::crystal::Crystal(buildAsymmetricUnit(crystal.atoms, uc),
                            buildSpacegroup(crystal.spaceGroup),
                            uc));
    m_crystalPdbContents.push_back(crystal.cifContents);
    m_crystalNames.push_back(crystal.name);
  }
  return true;
}

bool PdbFile::readFromString(const QString &content) {
  gemmi::Structure structure;
  try {
    structure = gemmi::read_pdb_string(content.toStdString(), "crystal");
  } catch (std::runtime_error &e) {
    qDebug() << "Error reading cif" << e.what();
    return false;
  }
  std::vector<PdbCrystalData> crystals = readStructure(structure);
  for (const auto &crystal : crystals) {
    auto uc = buildUnitCell(crystal.cell);
    m_crystals.emplace_back(
      occ::crystal::Crystal(buildAsymmetricUnit(crystal.atoms, uc),
                            buildSpacegroup(crystal.spaceGroup),
                            uc));
    m_crystalPdbContents.push_back(crystal.cifContents);
    m_crystalNames.push_back(crystal.name);
  }
  return true;
}

int PdbFile::numberOfCrystals() const { return m_crystals.size(); }

const occ::crystal::Crystal &PdbFile::getCrystalStructure(int index) const {
  return m_crystals.at(index);
}

const QByteArray& PdbFile::getCrystalPdbContents(int index) const {
  return m_crystalPdbContents.at(index);
}

const QString& PdbFile::getCrystalName(int index) const {
  return m_crystalNames.at(index);
}
