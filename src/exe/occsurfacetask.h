#include "externalprogram.h"


class OccSurfaceTask: public ExternalProgramTask {

public:
    inline static constexpr char wavefunctionSuffixDefault[] = {".owf.json"};
    inline static constexpr char inputSuffixDefault[] = {".json"};
    inline static constexpr char surfaceSuffixDefault[] = {".json"};

    explicit OccSurfaceTask(QObject * parent = nullptr);

    void setSurfaceParameters(const exe::surface::Parameters &);
    virtual void start() override;

    QString inputSuffix() const;
    QString wavefunctionSuffix() const;
    QString surfaceSuffix() const;

private:
    exe::surface::Parameters m_parameters;
    QString m_wavefunctionSuffix{".json"};
    QString m_basisSetDirectory;
};
