#include "externalprogram.h"
#include "wavefunction_parameters.h"


class OccWavefunctionTask : public ExternalProgramTask {

public:
    inline static constexpr char wavefunctionSuffixDefault[] = {".owf.json"};
    inline static constexpr char inputSuffixDefault[] = {".json"};

    explicit OccWavefunctionTask(QObject * parent = nullptr);

    void setParameters(const wfn::Parameters &);
    const wfn::Parameters& getParameters() const;
    virtual void start() override;

    QString inputSuffix() const;
    QString wavefunctionSuffix() const;
    QString wavefunctionFilename() const;

private:
    wfn::Parameters m_parameters;
    QString m_wavefunctionSuffix{".json"};
    QString m_basisSetDirectory;
};
