#include "externalprogram.h"
#include "pair_energy_parameters.h"


class OccPairTask: public ExternalProgramTask {

public:
    inline static constexpr char wavefunctionSuffixDefault[] = {".owf.json"};
    inline static constexpr char pairOutputSuffixDefault[] = {".json"};

    explicit OccPairTask(QObject * parent = nullptr);

    void setParameters(const pair_energy::Parameters &);
    virtual void start() override;

    int threads() const;
    QString jsonFilename() const;
    void setJsonFilename(QString);

private:
    void appendTransformArguments(QStringList &);

    QString kind() const;
    pair_energy::Parameters m_parameters;
    QString m_wavefunctionSuffix{".owf.json"};
    QString m_jsonFilename{"energies.json"};
    QString m_basisSetDirectory;
};