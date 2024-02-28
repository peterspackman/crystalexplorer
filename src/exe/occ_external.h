#include "externalprogram.h"


class OccWavefunctionTask : public ExternalProgramTask {

public:
    explicit OccWavefunctionTask(QObject * parent = nullptr);

    void setBasisSetDirectory(const QString &d);

    void setWavefunctionParameters(const exe::wfn::Parameters &);

    virtual void start() override;

private:
    exe::wfn::Parameters m_parameters;
    QString m_wavefunctionSuffix{".json"};
    QString m_basisSetDirectory;
};
