#pragma once
#include <QObject>
#include "pair_energy_parameters.h"

class PairInteractionResult : public QObject
{
    Q_OBJECT
public:
    explicit PairInteractionResult(const QString& interactionModel, QObject* parent = nullptr);
    QString interactionModel() const;
    void addComponent(const QString& component, double value);
    QList<QPair<QString, double>> components() const;

    void setParameters(const pair_energy::Parameters&);
    const pair_energy::Parameters &parameters() const;

private:
    QString m_interactionModel;
    QList<QPair<QString, double>> m_components;
    pair_energy::Parameters m_parameters;
};

class PairInteractionResults : public QObject
{
    Q_OBJECT
public:
    using PairInteractions = QList<PairInteractionResult *>;
    explicit PairInteractionResults(QObject* parent = nullptr);

    void addPairInteractionResult(PairInteractionResult* result);
    void removePairInteractionResult(PairInteractionResult* result);

    inline const auto &pairInteractionResults() const { return m_pairInteractionResults; }

    QList<QString> interactionModels() const;
    QList<PairInteractionResult*> filterByModel(const QString& model) const;
    QList<PairInteractionResult*> filterByComponent(const QString& component) const;
    QList<PairInteractionResult*> filterByModelAndComponent(const QString& model, const QString& component) const;

    int getCount(const QString &model = "") const;

signals:
    void resultAdded();
    void resultRemoved();
private:
    QMap<QString, PairInteractions> m_pairInteractionResults;
};
