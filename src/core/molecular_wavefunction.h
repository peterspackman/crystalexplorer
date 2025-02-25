#pragma once
#include "generic_atom_index.h"
#include "json.h"
#include "wavefunction_parameters.h"
#include <Eigen/Dense>
#include <QObject>
#include <vector>

class MolecularWavefunction : public QObject {
  Q_OBJECT
public:
  explicit MolecularWavefunction(QObject *parent = nullptr);

  [[nodiscard]] const QByteArray &rawContents() const;
  void setRawContents(QByteArray &&);
  void setRawContents(const QByteArray &);

  [[nodiscard]] const wfn::Parameters &parameters() const;
  void setParameters(const wfn::Parameters &);

  [[nodiscard]] const std::vector<GenericAtomIndex> &atomIndices() const;
  void setAtomIndices(const std::vector<GenericAtomIndex> &idxs);

  [[nodiscard]] bool haveContents() const;

  void writeToFile(const QString &);

  wfn::FileFormat fileFormat() const;
  void setFileFormat(wfn::FileFormat);

  QString fileFormatSuffix() const;

  [[nodiscard]] int charge() const;
  [[nodiscard]] int multiplicity() const;

  [[nodiscard]] const QString &method() const;
  [[nodiscard]] const QString &basis() const;

  [[nodiscard]] size_t fileSize() const;

  [[nodiscard]] int numberOfBasisFunctions() const;
  void setNumberOfBasisFunctions(int);

  [[nodiscard]] int numberOfOccupiedOrbitals() const;
  void setNumberOfOccupiedOrbitals(int);

  [[nodiscard]] int numberOfVirtualOrbitals() const;
  void setNumberOfVirtualOrbitals(int);

  [[nodiscard]] inline int numberOfOrbitals() const {
    return m_numOccupied + m_numVirtual;
  }

  [[nodiscard]] inline bool haveOrbitalEnergies() const {
    return m_orbitalEnergies.size() > 0;
  }

  [[nodiscard]] inline const auto &orbitalEnergies() const {
    return m_orbitalEnergies;
  }
  void setOrbitalEnergies(const std::vector<double> &);

  [[nodiscard]] double totalEnergy() const;
  void setTotalEnergy(double);

  QString description() const;

  QString fileSuffix() const;

  nlohmann::json toJson() const;
  bool fromJson(const nlohmann::json &j);

private:
  int m_nbf{0}, m_numOccupied{0}, m_numVirtual{0};
  std::vector<double> m_orbitalEnergies;
  double m_totalEnergy{0.0};
  wfn::FileFormat m_fileFormat{wfn::FileFormat::OccWavefunction};
  QByteArray m_rawContents;
  wfn::Parameters m_parameters;
};

struct WavefunctionAndTransform {
  MolecularWavefunction *wavefunction{nullptr};
  Eigen::Isometry3d transform;
};
