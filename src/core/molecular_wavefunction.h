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
    void setRawContents(const QByteArray &);

    [[nodiscard]] const wfn::Parameters &parameters() const;
    void setParameters(const wfn::Parameters &);

    [[nodiscard]] const std::vector<GenericAtomIndex>& atomIndices() const;
    void setAtomIndices(const std::vector<GenericAtomIndex> &idxs);

    [[nodiscard]] bool haveContents() const;

    void writeToFile(const QString &);

    wfn::FileFormat fileFormat() const;
    void setFileFormat(wfn::FileFormat);

private:
    wfn::FileFormat m_fileFormat;
    std::vector<GenericAtomIndex> m_atomIndices;
    QByteArray m_rawContents;
    wfn::Parameters m_parameters;
};
