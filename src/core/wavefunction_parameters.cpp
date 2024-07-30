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

bool Parameters::isXtbMethod() const {
  return xtb::isXtbMethod(method);
}

} // namespace wfn
