#pragma once
#include "externalprogram.h"


class TontoTask: public ExternalProgramTask {
    Q_OBJECT
public:
    explicit TontoTask(QObject * parent = nullptr);
    virtual void start() override;

    QString basisSetDirectory() const;
    QString slaterBasisName() const;

    void setCifFileName(const QString &);
    QString cifFileName() const;

    void setCxcFileName(const QString &);
    QString cxcFileName() const;

    bool overrideBondLengths() const;

    virtual QString getInputText() = 0;

    void appendHeaderBlock(const QString &header, QString &dest);
    void appendBasisSetDirectoryBlock(QString &dest);
    void appendCifDataBlock(const QString &dataBlock, QString &dest);
    void appendFooterBlock(QString &dest);
};

class TontoCifProcessingTask: public TontoTask {
    Q_OBJECT
public:
    explicit TontoCifProcessingTask(QObject * parent = nullptr);
    virtual void start() override;

    virtual QString getInputText() override;
};
