#include <QFile>

#include "gaussianinterface.h"
#include "nwcheminterface.h"
#include "occinterface.h"
#include "psi4interface.h"
#include "tontointerface.h"
#include "wavefunction.h"

Wavefunction::Wavefunction() : _jobParams(), _crystalName() {
  _wavefunctionIsComplete = false;
}

Wavefunction::Wavefunction(const JobParameters &jobParams,
                           const QString &crystalName) {
  _jobParams = jobParams;
  _crystalName = crystalName;
  qDebug() << "Wavefunction constructor for " << jobParams.atoms.size()
           << " atoms";

  bool successfullyReadWavefunctionFiles = false;
  QString filename;
  switch (_jobParams.program) {
  case ExternalProgram::Tonto: {
    filename = TontoInterface::tontoSBFName(_jobParams, _crystalName);
    successfullyReadWavefunctionFiles = storeWavefunctionFileWithType(
        filename, WavefunctionFileType::tontoMolecularOrbitals);

    break;
  }

  case ExternalProgram::Gaussian: {
    filename = GaussianInterface::defaultFChkFilename();
    successfullyReadWavefunctionFiles = storeWavefunctionFileWithType(
        filename, WavefunctionFileType::gaussianFChkFile);
    break;
  }

  case ExternalProgram::Psi4: {
    filename = Psi4Interface::fchkFilename(_jobParams, _crystalName);
    successfullyReadWavefunctionFiles = storeWavefunctionFileWithType(
        filename, WavefunctionFileType::gaussianFChkFile);
    break;
  }

  case ExternalProgram::NWChem: {
    filename = NWChemInterface::moldenFileName(_jobParams, _crystalName);
    successfullyReadWavefunctionFiles = storeWavefunctionFileWithType(
        filename, WavefunctionFileType::moldenFile);
    break;
  }

  case ExternalProgram::Occ: {
    filename = OccInterface::wavefunctionFilename(_jobParams, _crystalName);
    successfullyReadWavefunctionFiles = storeWavefunctionFileWithType(
        filename, WavefunctionFileType::gaussianFChkFile);
    break;
  }
  default: 
    Q_ASSERT(false);
  }

  qDebug() << "Wavefunction filename: " << filename;
  //  if(successfullyReadWavefunctionFiles) {
  //      if(settings::readSetting(settings::keys::DELETE_WORKING_FILES).toBool())
  //      {
  //          qDebug() << "Removing " << filename;
  //          QFile(filename).remove();
  //      }
  //  }
  _wavefunctionIsComplete = successfullyReadWavefunctionFiles;
}

bool Wavefunction::isValid(const QVector<AtomId> &selectedAtoms) const {
  return _jobParams.atoms.size() > 0 &&
         wavefunctionDefinedForAtoms(selectedAtoms);
}

bool Wavefunction::wavefunctionDefinedForAtoms(
    const QVector<AtomId> &atoms) const {
  if (atoms.size() != _jobParams.atoms.size()) {
    return false;
  }

  QVectorIterator<AtomId> i(atoms);

  while (i.hasNext()) {
    if (!_jobParams.atoms.contains(i.next())) {
      return false;
    }
  }
  return true;
}

bool Wavefunction::storeWavefunctionFileWithType(
    const QString &filename, WavefunctionFileType fileType) {
  QFile file(filename);
  qDebug() << "Reading wavefunction from file" << filename;
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }
  m_wavefunctionType = fileType;
  m_wavefunctionFile = file.readAll();
  qDebug() << "Read wavefunction file";
  return true;
}

QString Wavefunction::restoreWavefunctionFile(const QString &currentPath,
                                              int id) const {
  QString filename;

  switch (m_wavefunctionType) {
  case WavefunctionFileType::gaussianFChkFile:
    filename = TontoInterface::fchkFilename(_jobParams, _crystalName);
    break;
  case WavefunctionFileType::tontoMolecularOrbitals:
    filename = TontoInterface::tontoSBFName(_jobParams, _crystalName);
    break;
  case WavefunctionFileType::moldenFile:
    filename = TontoInterface::moldenFilename(_jobParams, _crystalName);
    break;
  }

  if (id > 0) {
    // Prepend the id to the filename
    filename = QString("%1_").arg(id) + filename;
  }
  QFile file(currentPath + QDir::separator() + filename);
  qDebug() << "Writing wavefunction file to " << file.fileName();
  if (!file.open(QIODevice::WriteOnly)) {
    return QString();
  }

  file.write(m_wavefunctionFile);
  file.close();
  return filename;
}

QString Wavefunction::description() const {
  QString source = _jobParams.programName();
  QString basisset = _jobParams.basisSetName();

  QString method;
  switch (_jobParams.theory) {
  case Method::hartreeFock:
    method = methodLabels[static_cast<int>(Method::hartreeFock)];
    break;
  case Method::mp2:
    method = methodLabels[static_cast<int>(Method::mp2)];
    break;
  case Method::b3lyp:
    method = methodLabels[static_cast<int>(Method::b3lyp)];
    break;
  case Method::kohnSham:
    method = exchangePotentialLabels[static_cast<int>(
                 _jobParams.exchangePotential)] +
             correlationPotentialLabels[static_cast<int>(
                 _jobParams.correlationPotential)];
    break;
  default:
    method = "Unknown method for wavefunction";
    break;
  }

  return method + "/" + basisset + " [" + source + "]";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QDataStream &operator<<(QDataStream &ds, const WavefunctionFileType &fileType) {
  ds << (int)fileType;
  return ds;
}

QDataStream &operator>>(QDataStream &ds, WavefunctionFileType &fileType) {
  int ft;
  ds >> ft;
  fileType = static_cast<WavefunctionFileType>(ft);
  return ds;
}

QDataStream &operator<<(QDataStream &ds, const Wavefunction &wavefunction) {
  ds << wavefunction._jobParams;
  ds << wavefunction._crystalName;
  ds << wavefunction._wavefunctionIsComplete;
  ds << wavefunction.m_wavefunctionType;
  ds << wavefunction.m_wavefunctionFile;

  return ds;
}

QDataStream &operator>>(QDataStream &ds, Wavefunction &wavefunction) {
  ds >> wavefunction._jobParams;
  ds >> wavefunction._crystalName;
  ds >> wavefunction._wavefunctionIsComplete;
  ds >> wavefunction.m_wavefunctionType;
  ds >> wavefunction.m_wavefunctionFile;
  return ds;
}
