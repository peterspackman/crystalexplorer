#pragma once
#include <QString>
#include <QVariant>

class ComputationProvider {
public:
    virtual ~ComputationProvider() = default;
    virtual bool canProvideProperty(const QString& property) const = 0;
    virtual QVariant getProperty(const QString& property) const = 0;
    virtual bool hasValidData() const = 0;
    virtual QString description() const = 0;
};