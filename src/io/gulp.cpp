#include "gulp.h"
#include <QFile>
#include <QRegularExpression>
#include <gemmi/cif.hpp>
#include <occ/core/units.h>
#include <occ/crystal/crystal.h>

namespace io {

GulpInputFile::GulpInputFile(const QString &filename) {
  m_success = load(filename);
}

GulpInputFile::GulpInputFile(const GulpInputFile &other)
    : m_keywords(other.m_keywords), m_cellParams(other.m_cellParams),
      m_atoms(other.m_atoms), m_spaceGroup(other.m_spaceGroup),
      m_errorMessage(other.m_errorMessage), m_success(other.m_success) {}

GulpInputFile &GulpInputFile::operator=(const GulpInputFile &other) {
  if (this != &other) {
    m_keywords = other.m_keywords;
    m_cellParams = other.m_cellParams;
    m_atoms = other.m_atoms;
    m_spaceGroup = other.m_spaceGroup;
    m_errorMessage = other.m_errorMessage;
    m_success = other.m_success;
  }
  return *this;
}

double GulpAtomPosition::parseFractional(const QString &value) {
  if (value.contains('/')) {
    QStringList parts = value.split('/');
    if (parts.size() == 2) {
      bool ok1, ok2;
      double num = parts[0].toDouble(&ok1);
      double denom = parts[1].toDouble(&ok2);
      if (ok1 && ok2 && denom != 0.0) {
        return num / denom;
      }
    }
  }
  return value.toDouble();
}

bool GulpInputFile::load(const QString &filename) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    m_errorMessage = "Failed to open file: " + filename;
    return false;
  }

  QTextStream contentStream(&file);
  m_fileContents = contentStream.readAll();
  file.seek(0);

  QTextStream in(&file);
  QString line;

  enum class ParseState {
    Initial,
    ExpectingCell,
    ExpectingSpace,
    ExpectingAtomLine,
    SkipPotentials,
    Error
  } state = ParseState::Initial;

  int atomCount = -1;  // Expected number of atoms
  int atomsParsed = 0; // Number of atoms parsed so far

  while (!in.atEnd()) {
    line = in.readLine().trimmed();
    if (line.isEmpty() || line.startsWith('#')) {
      continue;
    }

    if (line.startsWith("cell", Qt::CaseInsensitive)) {
      state = ParseState::ExpectingCell;
      continue;
    } else if (line.startsWith("pcel", Qt::CaseInsensitive)) {
      state = ParseState::Error;
      m_errorMessage = "pcell is not supported";
      break;
    } else if (line.startsWith("scel", Qt::CaseInsensitive)) {
      state = ParseState::Error;
      m_errorMessage = "scell is not supported";
      break;
    } else if (line.startsWith("cart", Qt::CaseInsensitive)) {
      state = ParseState::ExpectingAtomLine;
      m_fractional = false;

      QStringList tokens = tokenize(line);
      if (tokens.size() > 1) {
        bool ok;
        atomCount = tokens[1].toInt(&ok);
        if (!ok)
          atomCount = -1;
      }

      continue;
    } else if (line.startsWith("frac", Qt::CaseInsensitive)) {
      state = ParseState::ExpectingAtomLine;
      m_fractional = true;

      QStringList tokens = tokenize(line);
      if (tokens.size() > 1) {
        bool ok;
        atomCount = tokens[1].toInt(&ok);
        if (!ok)
          atomCount = -1;
      }

      continue;
    } else if (line.startsWith("space", Qt::CaseInsensitive)) {
      state = ParseState::ExpectingSpace;
      continue;
    } else if (isKeywordLine(line)) {
      state = ParseState::SkipPotentials;
      continue;
    }

    switch (state) {
    case ParseState::Error:
      return false;
    case ParseState::ExpectingCell:
      parseCell(line);
      state = ParseState::Initial;
      break;
    case ParseState::ExpectingSpace:
      m_spaceGroup = line;
      state = ParseState::Initial;
      break;
    case ParseState::ExpectingAtomLine:
      if (parseCoords(line)) {
        atomsParsed++;
        // If we've parsed all expected atoms, move to initial state
        if (atomCount > 0 && atomsParsed >= atomCount) {
          state = ParseState::Initial;
        }
      } else {
        // If coordinate parsing fails, it might be a potential line or other
        // keyword
        if (isKeywordLine(line)) {
          state = ParseState::SkipPotentials;
        } else {
          // Otherwise add it to the keywords (could be a new section)
          m_keywords += line;
          state = ParseState::Initial;
        }
      }
      break;
    case ParseState::SkipPotentials:
      break;
    case ParseState::Initial:
      m_keywords += line;
      break;
    }
  }

  return true;
}

bool GulpInputFile::isKeywordLine(const QString &line) {
  const QStringList keywords = {
      "morse",  "lennard",   "buckingham",  "three",     "torsion", "bond",
      "angle",  "shell",     "spring",      "species",   "shrink",  "kpoint",
      "output", "potential", "temperature", "pressure",  "cutp",    "dump",
      "reaxff", "library",   "uff",         "gasteiger", "fix",     "conp",
      "conv",   "opti",      "gradient",    "hessian"};

  QString lowerLine = line.toLower();
  foreach (const QString &keyword, keywords) {
    if (lowerLine.startsWith(keyword)) {
      return true;
    }
  }
  return false;
}

bool GulpInputFile::parseCoords(const QString &line) {
  QStringList tokens = tokenize(line);
  if (tokens.size() < 3)
    return false; // Need at least element and x,y coordinates

  if (tokens[0].toLower() == "end" || tokens[0].toLower() == "title" ||
      tokens[0].toLower() == "#") {
    return false;
  }

  QString element = tokens[0];
  if (element.length() > 4) {
    return false; // Not likely an element symbol
  }

  GulpAtomPosition atom;
  int idx = 0;

  atom.element = tokens[idx++];
  if (idx < tokens.size() &&
      (tokens[idx].toLower() == "core" || tokens[idx].toLower() == "shel")) {
    atom.core_shell = tokens[idx++].toLower();
  } else {
    return false;
  }

  if (idx + 2 >= tokens.size()) {
    return false;
  }

  bool xOk, yOk, zOk;
  atom.x = GulpAtomPosition::parseFractional(tokens[idx++]);
  atom.y = GulpAtomPosition::parseFractional(tokens[idx++]);
  atom.z = GulpAtomPosition::parseFractional(tokens[idx++]);

  if (idx < tokens.size()) {
    bool ok;
    double charge = tokens[idx].toDouble(&ok);
    if (ok) {
      atom.charge = charge;
      atom.has_charge = true;
      idx++;
    }
  }

  if (idx < tokens.size()) {
    bool ok;
    double occupancy = tokens[idx].toDouble(&ok);
    if (ok) {
      atom.occupancy = occupancy;
    }
  }

  m_atoms.push_back(atom);
  return true;
}

void GulpInputFile::parseCell(const QString &line) {
  QStringList tokens = tokenize(line);
  m_cellParams = {1.0, 1.0, 1.0, 90.0, 90.0, 90.0};

  for (int i = 0; i < tokens.size() && i < 6; ++i) {
    bool ok;
    double value = tokens[i].toDouble(&ok);
    if (ok) {
      m_cellParams[i] = value;
    }
  }
  m_periodicity = 3;
}

QStringList GulpInputFile::tokenize(const QString &line) {
  return line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
}

ChemicalStructure *GulpInputFile::toChemicalStructure() const {
  if (!m_success) {
    qDebug() << "Cannot create crystal structure from gulp input:"
             << m_errorMessage;
    return nullptr;
  }

  std::vector<QString> symbols;
  std::vector<occ::Vec3> positions;

  for (const auto &atom : m_atoms) {
    symbols.push_back(atom.element);
    positions.push_back(occ::Vec3(atom.x, atom.y, atom.z));
  }
  if (!((symbols.size() == positions.size()) && (symbols.size() > 0))) {
    qDebug() << "Invalid atom list when loading gulp input";
    qDebug() << "Have " << symbols.size() << "atom symbols and"
             << positions.size() << "atomic positions";
    return nullptr;
  }

  ChemicalStructure *structure = new ChemicalStructure();
  structure->setAtoms(symbols, positions);
  structure->updateBondGraph();

  structure->setProperty("gulp_contents", m_fileContents);

  return structure;
}

using occ::crystal::AsymmetricUnit;
using occ::crystal::SpaceGroup;
using occ::crystal::UnitCell;

inline AsymmetricUnit
buildAsymmetricUnit(const std::vector<GulpAtomPosition> &atoms) {
  AsymmetricUnit result;
  size_t numAtoms = atoms.size();
  result.atomic_numbers = occ::IVec(numAtoms);
  result.positions = occ::Mat3N(3, numAtoms);
  result.adps = occ::Mat6N::Zero(6, numAtoms);
  for (int i = 0; i < atoms.size(); i++) {
    const auto &atom = atoms[i];
    result.positions.col(i) = occ::Vec3(atom.x, atom.y, atom.z);
    auto el = atom.element.toStdString();
    result.atomic_numbers(i) = occ::core::Element(el).atomic_number();
    qDebug() << atom.element << atom.x << atom.y << atom.z
             << result.atomic_numbers(i);
    result.labels.push_back(el);
  }
  return result;
}

inline UnitCell buildUnitCell(const std::array<double, 6> &cellData) {
  using occ::units::radians;
  return UnitCell(cellData[0], cellData[1], cellData[2], radians(cellData[3]),
                  radians(cellData[4]), radians(cellData[5]));
}

inline SpaceGroup buildSpaceGroup(const QString &symbol) {
  qDebug() << "Space group symbol: " << symbol;
  bool isNumber{false};
  int num = symbol.toInt(&isNumber);
  if (isNumber) {
    return SpaceGroup(num);
  }
  auto sym = symbol.toStdString();
  const auto *sgdata = gemmi::find_spacegroup_by_name(sym);
  if (sgdata) {
    return SpaceGroup(sym);
  }

  return SpaceGroup(1);
}

CrystalStructure *GulpInputFile::toCrystalStructure() const {
  if (!m_success) {
    qDebug() << "Cannot create crystal structure from gulp input:"
             << m_errorMessage;
    return nullptr;
  }
  CrystalStructure *structure = new CrystalStructure();

  AsymmetricUnit asym = buildAsymmetricUnit(m_atoms);
  UnitCell cell = buildUnitCell(m_cellParams);
  SpaceGroup sg = buildSpaceGroup(m_spaceGroup);
  if (!m_fractional) {
    asym.positions = cell.to_fractional(asym.positions);
  }
  occ::crystal::Crystal crystal(asym, sg, cell);

  structure->setOccCrystal(crystal);
  structure->setFileContents(m_fileContents.toUtf8());

  return structure;
}

GulpInputFile *
GulpInputFile::fromChemicalStructure(const ChemicalStructure *structure) {
  GulpInputFile *gulp = new GulpInputFile();

  gulp->setKeywords(QStringList() << "opti" << "conp");

  const auto &symbols = structure->labels();
  const auto &positions = structure->atomicPositions();

  for (int i = 0; i < symbols.size(); ++i) {
    GulpAtomPosition atom;
    atom.element = symbols[i];
    atom.core_shell = "core";

    const auto &pos = positions.col(i);
    atom.x = pos.x();
    atom.y = pos.y();
    atom.z = pos.z();

    gulp->addAtom(atom);
  }

  return gulp;
}

GulpInputFile *
GulpInputFile::fromCrystalStructure(const CrystalStructure *structure) {
  using occ::units::degrees;
  GulpInputFile *gulp = new GulpInputFile();

  gulp->setKeywords(QStringList() << "opti" << "conp");

  const auto &crystal = structure->occCrystal();
  const auto &asym = crystal.asymmetric_unit();
  const auto &sg = crystal.space_group();
  const auto &cell = crystal.unit_cell();

  std::array<double, 6> cellParams{cell.a(),
                                   cell.b(),
                                   cell.c(),
                                   degrees(cell.alpha()),
                                   degrees(cell.beta()),
                                   degrees(cell.gamma())};
  gulp->setCellParameters(cellParams);

  gulp->setSpaceGroup(QString::fromStdString(sg.symbol()));

  for (int i = 0; i < asym.size(); ++i) {
    GulpAtomPosition atom;
    atom.element = QString::fromStdString(asym.labels[i]);
    atom.core_shell = "core";

    const auto &pos = asym.positions.col(i);
    atom.x = pos.x();
    atom.y = pos.y();
    atom.z = pos.z();
    gulp->addAtom(atom);
  }

  return gulp;
}

} // namespace io
