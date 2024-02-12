#pragma once

#include "atomid.h"
#include "jobparameters.h"

enum class WavefunctionFileType {
  gaussianFChkFile,
  tontoMolecularOrbitals,
  moldenFile
};

class Wavefunction {
  friend QDataStream &operator<<(QDataStream &, const Wavefunction &);
  friend QDataStream &operator>>(QDataStream &, Wavefunction &);

public:
  Wavefunction(); // required for the stream operators DO NOT USE otherwise
  Wavefunction(const JobParameters &, const QString &);
  bool isValid(const QVector<AtomId> &) const;
  bool isComplete() const { return _wavefunctionIsComplete; }
  const JobParameters &jobParameters() const { return _jobParams; }
  QString restoreWavefunctionFile(const QString &, int) const;
  QString description() const;
  const QVector<AtomId> &atomIds() const { return _jobParams.atoms; }
  const auto &wavefunctionFile() const { return m_wavefunctionFile; }

  // Static functions
  static QString methodString(Method method) {
    return QString("%1").arg(methodLabels[static_cast<int>(method)]);
  }
  static QString levelOfTheoryString(Method method, BasisSet basisset) {
    return QString("%1/%2")
        .arg(methodLabels[static_cast<int>(method)])
        .arg(basisSetLabel(basisset));
  }

private:
  bool wavefunctionDefinedForAtoms(const QVector<AtomId> &) const;
  bool storeWavefunctionFileWithType(const QString &, WavefunctionFileType);
  QString calculationName();
  QString tontoSuffix();

  JobParameters _jobParams;
  QString _crystalName;
  WavefunctionFileType m_wavefunctionType{
      WavefunctionFileType::gaussianFChkFile};
  QByteArray m_wavefunctionFile;

  bool _wavefunctionIsComplete;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &, const WavefunctionFileType &);
QDataStream &operator>>(QDataStream &, WavefunctionFileType &);

QDataStream &operator<<(QDataStream &, const Wavefunction &);
QDataStream &operator>>(QDataStream &, Wavefunction &);
