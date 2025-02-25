#include "wavefunction_parameters.h"
#include "xtb_parameters.h"

namespace wfn {

// Array of all FileFormat values
inline constexpr std::array<FileFormat, 3> availableFileFormats = {
    FileFormat::OccWavefunction, FileFormat::Fchk, FileFormat::Molden};

QString fileFormatString(FileFormat fmt) {
  switch (fmt) {
  case FileFormat::Fchk:
    return "FCHK";
  case FileFormat::Molden:
    return "Molden";
  default:
    return "OWF JSON";
  }
}

FileFormat fileFormatFromString(const QString &fmtString) {
  // Iterate through all FileFormat values
  for (const auto &fmt : availableFileFormats) {
    if (fmtString.compare(fileFormatString(fmt), Qt::CaseInsensitive))
      return fmt;
  }
  return FileFormat::OccWavefunction;
}

QString fileFormatSuffix(FileFormat fmt) {
  switch (fmt) {
  case FileFormat::Fchk:
    return ".fchk";
  case FileFormat::Molden:
    return ".molden";
  default:
    return ".owf.json";
  }
}

FileFormat fileFormatFromFilename(const QString &filename) {
  QString lowercaseFilename = filename.toLower();

  // Iterate through all FileFormat values
  for (const auto &fmt : availableFileFormats) {
    if (lowercaseFilename.endsWith(fileFormatSuffix(fmt))) {
      return fmt;
    }
  }

  // If no match is found, default to OccWavefunction
  return FileFormat::OccWavefunction;
}

// Array of all Program values
inline constexpr std::array<Program, 6> availablePrograms = {
    Program::Occ,    Program::Orca, Program::Gaussian,
    Program::NWChem, Program::Psi4, Program::Xtb};

QString programName(Program prog) {
  switch (prog) {
  case Program::Occ:
    return "OCC";
  case Program::Orca:
    return "Orca";
  case Program::Gaussian:
    return "Gaussian";
  case Program::NWChem:
    return "NWChem";
  case Program::Psi4:
    return "Psi4";
  case Program::Xtb:
    return "XTB";
  }
  return "OCC";
}

Program programFromName(const QString &name) {
  QString lowercaseName = name.toLower();
  for (const auto &prog : availablePrograms) {
    QString candidate = programName(prog).toLower();
    if (lowercaseName == candidate)
      return prog;
  }
  return Program::Occ;
}

bool Parameters::isXtbMethod() const { return xtb::isXtbMethod(method); }

} // namespace wfn
