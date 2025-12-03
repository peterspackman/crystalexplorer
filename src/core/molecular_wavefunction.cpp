#include "molecular_wavefunction.h"
#include <QDebug>
#include <QFile>

MolecularWavefunction::MolecularWavefunction(QObject *parent)
    : QObject(parent) {}

const QByteArray &MolecularWavefunction::rawContents() const {
  return m_rawContents;
}

void MolecularWavefunction::setRawContents(QByteArray &&contents) {
  m_rawContents = contents;
}

void MolecularWavefunction::setRawContents(const QByteArray &contents) {
  m_rawContents = contents;
}

const wfn::Parameters &MolecularWavefunction::parameters() const {
  return m_parameters;
}

void MolecularWavefunction::setParameters(const wfn::Parameters &params) {
  m_parameters = params;
}

const std::vector<GenericAtomIndex> &
MolecularWavefunction::atomIndices() const {
  return m_parameters.atoms;
}

void MolecularWavefunction::setAtomIndices(
    const std::vector<GenericAtomIndex> &idxs) {
  m_parameters.atoms = idxs;
}

bool MolecularWavefunction::writeToFile(const QString &filename) {
  QFile file(filename);
  if (file.open(QIODevice::WriteOnly)) {
    file.write(this->rawContents());
    file.close();
    return true;
  } else {
    qWarning() << "Could not open file for writing:" << filename;
    return false;
  }
}

bool MolecularWavefunction::haveContents() const {
  return m_rawContents.size() != 0;
}

wfn::FileFormat MolecularWavefunction::fileFormat() const {
  return m_fileFormat;
}

QString MolecularWavefunction::fileFormatSuffix() const {
  return wfn::fileFormatSuffix(m_fileFormat);
}

void MolecularWavefunction::setFileFormat(wfn::FileFormat fmt) {
  m_fileFormat = fmt;
}

int MolecularWavefunction::charge() const { return m_parameters.charge; }

int MolecularWavefunction::multiplicity() const {
  return m_parameters.multiplicity;
}

size_t MolecularWavefunction::fileSize() const { return m_rawContents.size(); }

const QString &MolecularWavefunction::method() const {
  return m_parameters.method;
}

const QString &MolecularWavefunction::basis() const {
  return m_parameters.basis;
}

int MolecularWavefunction::numberOfBasisFunctions() const { return m_nbf; }

void MolecularWavefunction::setNumberOfBasisFunctions(int nbf) { m_nbf = nbf; }

int MolecularWavefunction::numberOfOccupiedOrbitals() const {
  return m_numOccupied;
}

void MolecularWavefunction::setNumberOfOccupiedOrbitals(int n) {
  m_numOccupied = n;
}

int MolecularWavefunction::numberOfVirtualOrbitals() const {
  return m_numVirtual;
}

void MolecularWavefunction::setNumberOfVirtualOrbitals(int n) {
  m_numVirtual = n;
}

double MolecularWavefunction::totalEnergy() const { return m_totalEnergy; }

void MolecularWavefunction::setTotalEnergy(double e) { m_totalEnergy = e; }

void MolecularWavefunction::setOrbitalEnergies(
    const std::vector<double> &energies) {
  m_orbitalEnergies = energies;
}

QString MolecularWavefunction::description() const {
  return QString("%1/%2").arg(m_parameters.method).arg(m_parameters.basis);
}

QString MolecularWavefunction::fileSuffix() const {
  switch (m_fileFormat) {
  case wfn::FileFormat::OccWavefunction:
    return ".owf.json";
  case wfn::FileFormat::Fchk:
    return ".fchk";
  case wfn::FileFormat::Molden:
    return ".molden";
  default:
    return ".owf.json";
  }
}

void to_json(nlohmann::json &j, const wfn::Parameters &params) {
  j["charge"] = params.charge;
  j["multiplicity"] = params.multiplicity;
  j["method"] = params.method;
  j["basis"] = params.basis;

  j["program"] = wfn::programName(params.program);
  j["atoms"] = params.atoms;
  j["accepted"] = params.accepted;
  j["userEditRequested"] = params.userEditRequested;
  j["name"] = params.name;
  j["userInputContents"] = params.userInputContents;
}

void from_json(const nlohmann::json &j, wfn::Parameters &params) {
  j.at("charge").get_to(params.charge);
  j.at("multiplicity").get_to(params.multiplicity);
  j.at("method").get_to(params.method);
  j.at("basis").get_to(params.basis);

  params.program = wfn::programFromName(j["program"].get<QString>());
  j.at("atoms").get_to(params.atoms);
  j.at("accepted").get_to(params.accepted);
  j.at("userEditRequested").get_to(params.userEditRequested);
  j.at("name").get_to(params.name);
  j.at("userInputContents").get_to(params.userInputContents);
}

nlohmann::json MolecularWavefunction::toJson() const {
  nlohmann::json j;
  j["nbf"] = m_nbf;
  j["numOccupied"] = m_numOccupied;
  j["numVirtual"] = m_numVirtual;
  j["orbitalEnergies"] = m_orbitalEnergies;
  j["totalEnergy"] = m_totalEnergy;
  j["fileFormat"] = wfn::fileFormatString(m_fileFormat);
  j["fileContents"] = m_rawContents;
  j["name"] = objectName();
  to_json(j["parameters"], m_parameters);
  return j;
}

bool MolecularWavefunction::fromJson(const nlohmann::json &j) {
  try {
    from_json(j["parameters"], m_parameters);
    if (j.contains("name")) {
      setObjectName(j.at("name").get<QString>());
    }
    j.at("nbf").get_to(m_nbf);
    j.at("numOccupied").get_to(m_numOccupied);
    j.at("numVirtual").get_to(m_numVirtual);
    j.at("orbitalEnergies").get_to(m_orbitalEnergies);
    j.at("totalEnergy").get_to(m_totalEnergy);
    m_fileFormat = wfn::fileFormatFromString(j["fileFormat"]);
    j.at("fileContents").get_to(m_rawContents);
  } catch (nlohmann::json::parse_error &e) {
    qWarning() << "JSON parse error loading MolecularWavefunction:" << e.what();
    return false;
  }
  return true;
}
