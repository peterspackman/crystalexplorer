#include "externalprogram.h"


class OccSurfaceTask: public ExternalProgramTask {

public:
    inline static constexpr char wavefunctionSuffixDefault[] = {".owf.json"};
    inline static constexpr char surfaceSuffixDefault[] = {".ply"};

    explicit OccSurfaceTask(QObject * parent = nullptr);

    void setSurfaceParameters(const exe::surface::Parameters &);
    virtual void start() override;

    QString inputFileName() const;
    QString environmentFileName() const;
    QString wavefunctionSuffix() const;
    QString outputFileName() const;

    float separation() const;
    float isovalue() const;

private:
    exe::surface::Parameters m_parameters;
    QString m_wavefunctionSuffix{".json"};
    QString m_basisSetDirectory;
};
