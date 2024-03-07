#include "externalprogram.h"


class OccWavefunctionTask : public ExternalProgramTask {

public:
    inline static constexpr char wavefunctionSuffixDefault[] = {".owf.json"};
    inline static constexpr char inputSuffixDefault[] = {".json"};

    explicit OccWavefunctionTask(QObject * parent = nullptr);

    void setWavefunctionParameters(const exe::wfn::Parameters &);
    virtual void start() override;

    QString inputSuffix() const;
    QString wavefunctionSuffix() const;

private:
    exe::wfn::Parameters m_parameters;
    QString m_wavefunctionSuffix{".json"};
    QString m_basisSetDirectory;
};
