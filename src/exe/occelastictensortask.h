#pragma once
#include "externalprogram.h"

class OccElasticTensorTask : public ExternalProgramTask {
    Q_OBJECT
public:
    explicit OccElasticTensorTask(QObject *parent = nullptr);

    void setInputJsonFile(const QString &filename);
    QString outputJsonFilename() const;

    virtual void start() override;

private:
    QString m_inputJsonFile;
};
