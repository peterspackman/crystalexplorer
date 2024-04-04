#include "molecular_wavefunction.h"


MolecularWavefunction::MolecularWavefunction(QObject *parent) : QObject(parent) {
}


const QByteArray& MolecularWavefunction::rawContents() const {
    return m_rawContents;
}

void MolecularWavefunction::setRawContents(QByteArray &&contents) {
    m_rawContents = contents;
}

void MolecularWavefunction::setRawContents(const QByteArray &contents) {
    m_rawContents = contents;
}


const wfn::Parameters& MolecularWavefunction::parameters() const {
    return m_parameters;
}

void  MolecularWavefunction::setParameters(const wfn::Parameters &params) {
    m_parameters = params;
}


const std::vector<GenericAtomIndex>& MolecularWavefunction::atomIndices() const {
    return m_atomIndices; 
}

void  MolecularWavefunction::setAtomIndices(const std::vector<GenericAtomIndex> &idxs) {
    m_atomIndices = idxs;
}

void MolecularWavefunction::writeToFile(const QString &dest) {

}

bool MolecularWavefunction::haveContents() const {
    return m_rawContents.size() != 0;
}


wfn::FileFormat MolecularWavefunction::fileFormat() const {
    return m_fileFormat;
}

void MolecularWavefunction::setFileFormat(wfn::FileFormat fmt) {
    m_fileFormat = fmt;
}

int MolecularWavefunction::charge() const {
    return m_parameters.charge;
}

int MolecularWavefunction::multiplicity() const {
    return m_parameters.multiplicity;
}

size_t MolecularWavefunction::fileSize() const {
    return m_rawContents.size();
}

const QString& MolecularWavefunction::method() const {
    return m_parameters.method;
}

const QString& MolecularWavefunction::basis() const {
    return m_parameters.basis;
}


int MolecularWavefunction::numberOfBasisFunctions() const {
    return m_nbf;
}

void MolecularWavefunction::setNumberOfBasisFunctions(int nbf) {
    m_nbf = nbf;
}


double MolecularWavefunction::totalEnergy() const {
    return m_totalEnergy;
}

void MolecularWavefunction::setTotalEnergy(double e) {
    m_totalEnergy = e;
}
