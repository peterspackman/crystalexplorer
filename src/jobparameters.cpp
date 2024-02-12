#include "jobparameters.h"
#include "settings.h"

ExternalProgram JobParameters::prefferedWavefunctionSource() {
  QString source =
      settings::readSetting(settings::keys::PREFERRED_WAVEFUNCTION_SOURCE)
          .toString();
  if (source == "NWChem") {
    return ExternalProgram::NWChem;
  }
  if (source == "Psi4") {
    return ExternalProgram::Psi4;
  }
  if (source == "Gaussian") {
    return ExternalProgram::Gaussian;
  }
  if (source == "occ") {
    return ExternalProgram::Occ;
  }
  return ExternalProgram::Tonto;
}

JobParameters::JobParameters() {}

bool JobParameters::equivalentTo(const JobParameters &params) const {
  if (params.surfaceType != surfaceType)
    return false;
  if (params.resolution != resolution)
    return false;
  if (params.isovalue != isovalue)
    return false;
  if (surfaceType == IsosurfaceDetails::Type::Orbital) {
    if (params.molecularOrbitalType != molecularOrbitalType)
      return false;
    if (params.molecularOrbitalLevel != molecularOrbitalLevel)
      return false;
  }
  if (surfaceType == IsosurfaceDetails::Type::Hirshfeld ||
      surfaceType == IsosurfaceDetails::Type::PromoleculeDensity)
    return true;
  if (program != ExternalProgram::None) {

    if (params.theory != theory)
      return false;
    if (params.basisset != basisset)
      return false;
    if (params.theory == Method::kohnSham) {
      if (params.exchangePotential != exchangePotential)
        return false;
      if (params.correlationPotential != correlationPotential)
        return false;
    }
  }
  return true;
}

bool JobParameters::hasSameWavefunctionParameters(
    const JobParameters &rhs) const {
  if (program != rhs.program)
    return false;
  if (theory != rhs.theory)
    return false;
  if (basisset != rhs.basisset)
    return false;
  if (charge != rhs.charge)
    return false;
  if (multiplicity != rhs.multiplicity)
    return false;
  if (theory == Method::kohnSham) {
    if (exchangePotential != rhs.exchangePotential)
      return false;
    if (correlationPotential != rhs.correlationPotential)
      return false;
  }
  return true;
}

bool JobParameters::isXtbJob() const {
  switch (theory) {
  case Method::GFN0xTB:
    return true;
  case Method::GFN1xTB:
    return true;
  case Method::GFN2xTB:
    return true;
  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &ds, const JobParameters &job) {
  // General parameters
  ds << job.jobType;

  // Surface parameters
  ds << job.surfaceType << job.requestedPropertyType << job.isovalue
     << job.resolution << job.voidClusterPadding;

  // Molecular orbital parameters
  ds << job.molecularOrbitalType << job.molecularOrbitalLevel;

  // Basisset parameters
  ds << job.slaterBasisName;

  // Filenames and Directories
  ds << job.inputFilename << job.outputFilename;

  // Miscellaneous Options
  ds << job.overrideBondLengths << job.editInputFile;

  // General wavefunction parameters
  ds << job.charge << job.multiplicity;
  ds << job.program;
  ds << job.exchangePotential << job.correlationPotential << job.basisset
     << job.theory;
  ds << job.QMInputFilename;
  ds << job.wavefunctionTransforms;

  // Atoms
  ds << job.atoms;
  ds << job.atomGroups;

  // Atoms to Suppress
  ds << job.atomsToSuppress;

  return ds;
}

QDataStream &operator>>(QDataStream &ds, JobParameters &job) {
  // General parameters
  int jobType;
  ds >> jobType;
  job.jobType = static_cast<JobType>(jobType);

  // Surface parameters
  int surfaceType;
  ds >> surfaceType;
  job.surfaceType = static_cast<IsosurfaceDetails::Type>(surfaceType);

  int requestedPropertyType;
  ds >> requestedPropertyType;
  job.requestedPropertyType =
      static_cast<IsosurfacePropertyDetails::Type>(requestedPropertyType);

  ds >> job.isovalue;

  int resolution;
  ds >> resolution;
  job.resolution = static_cast<ResolutionDetails::Level>(resolution);

  ds >> job.voidClusterPadding;

  // Molecular orbital parameters
  int molecularOrbitalType;
  ds >> molecularOrbitalType;
  job.molecularOrbitalType = static_cast<OrbitalType>(molecularOrbitalType);

  ds >> job.molecularOrbitalLevel;

  // Basisset parameters
  ds >> job.slaterBasisName;

  // Filenames and Directories
  ds >> job.inputFilename >> job.outputFilename;

  // Miscellaneous Options
  ds >> job.overrideBondLengths >> job.editInputFile;

  // General wavefunction parameters
  ds >> job.charge >> job.multiplicity;
  int wavefunctionSource;
  ds >> wavefunctionSource;
  job.program = static_cast<ExternalProgram>(wavefunctionSource);

  // Tonto wavefunction calculation parameters
  int exchange;
  ds >> exchange;
  job.exchangePotential = static_cast<ExchangePotential>(exchange);

  int correlation;
  ds >> correlation;
  job.correlationPotential = static_cast<CorrelationPotential>(correlation);

  int basisset;
  ds >> basisset;
  job.basisset = static_cast<BasisSet>(basisset);

  int theory;
  ds >> theory;
  job.theory = static_cast<Method>(theory);

  ds >> job.QMInputFilename;
  ds >> job.wavefunctionTransforms;

  // Atoms
  ds >> job.atoms;
  ds >> job.atomGroups;

  // Atoms to Suppress;
  ds >> job.atomsToSuppress;

  return ds;
}
