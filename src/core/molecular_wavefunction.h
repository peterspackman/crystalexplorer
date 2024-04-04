#pragma once
#include "wavefunction_parameters.h"
#include "generic_atom_index.h"
#include <QObject>
#include <vector>

class MolecularWavefunction: public QObject {
    Q_OBJECT
public:
    explicit MolecularWavefunction(QObject *parent = nullptr);

    [[nodiscard]] const QByteArray &rawContents() const;
    void setRawContents(QByteArray &&);
    void setRawContents(const QByteArray &);

    [[nodiscard]] const wfn::Parameters &parameters() const;
    void setParameters(const wfn::Parameters &);

    [[nodiscard]] const std::vector<GenericAtomIndex>& atomIndices() const;
    void setAtomIndices(const std::vector<GenericAtomIndex> &idxs);

    [[nodiscard]] bool haveContents() const;

    void writeToFile(const QString &);

    wfn::FileFormat fileFormat() const;
    void setFileFormat(wfn::FileFormat);

    [[nodiscard]] int charge() const;
    [[nodiscard]] int multiplicity() const;

    [[nodiscard]] const QString &method() const;
    [[nodiscard]] const QString &basis() const;

    [[nodiscard]] size_t fileSize() const;

    [[nodiscard]] int numberOfBasisFunctions() const;
    void setNumberOfBasisFunctions(int);

    [[nodiscard]] double totalEnergy() const;
    void setTotalEnergy(double);

private:
    int m_nbf{0};
    double m_totalEnergy{0.0};
    wfn::FileFormat m_fileFormat;
    std::vector<GenericAtomIndex> m_atomIndices;
    QByteArray m_rawContents;
    wfn::Parameters m_parameters;
};
