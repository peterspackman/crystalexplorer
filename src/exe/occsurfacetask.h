#pragma once
#include "externalprogram.h"
#include "isosurface_parameters.h"


class OccSurfaceTask: public ExternalProgramTask {

public:
    inline static constexpr char wavefunctionSuffixDefault[10] = {".owf.json"};
    inline static constexpr char surfaceSuffixDefault[5] = {".ply"};

    explicit OccSurfaceTask(QObject * parent = nullptr);

    void setSurfaceParameters(const isosurface::Parameters &);
    virtual void start() override;

    QString inputFileName() const;
    QString environmentFileName() const;
    QString wavefunctionSuffix() const;
    QString wavefunctionFilename() const;
    QString outputFileNameTemplate() const;
    QStringList outputFileNames() const;
    QStringList orbitalLabels() const;
    QStringList getMeshLabels() const;

    float separation() const;
    float isovalue() const;
    int threads() const;

private:
    void appendWavefunctionTransformArguments(QStringList &args);
    void appendOrbitalLabels(QStringList &args);
    QString kind() const;
    isosurface::Parameters m_parameters;
    QString m_wavefunctionSuffix{".json"};
    QString m_basisSetDirectory;
};
