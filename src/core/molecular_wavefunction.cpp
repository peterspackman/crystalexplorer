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

void MolecularWavefunction::writeToFile(const QString &filename) {
  QFile file(filename);
  if (file.open(QIODevice::WriteOnly)) {
    file.write(this->rawContents());
    file.close();
  } else {
    qDebug() << "Could not open file for writing in "
                "MolecularWavefunction::writeToFile";
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

double MolecularWavefunction::totalEnergy() const { return m_totalEnergy; }

void MolecularWavefunction::setTotalEnergy(double e) { m_totalEnergy = e; }

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
