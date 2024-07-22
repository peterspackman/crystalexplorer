#pragma once
#include <QObject>
#include "pair_energy_parameters.h"

class PairInteraction: public QObject
{
    Q_OBJECT
public:
    explicit PairInteraction(const QString& interactionModel, QObject* parent = nullptr);
    QString interactionModel() const;
    void addComponent(const QString& component, double value);
    QList<QPair<QString, double>> components() const;

    inline const QString &symmetry() const { return m_parameters.symmetry; }
    inline double nearestAtomDistance() const { return m_parameters.nearestAtomDistance; }
    inline double centroidDistance() const { return m_parameters.centroidDistance; }

    void setParameters(const pair_energy::Parameters&);
    const pair_energy::Parameters &parameters() const;

private:
    QString m_interactionModel;
    QList<QPair<QString, double>> m_components;
    pair_energy::Parameters m_parameters;
};

class PairInteractions: public QObject
{
    Q_OBJECT
public:
    using PairInteractionList = QList<PairInteraction *>;
    explicit PairInteractions(QObject* parent = nullptr);

    void add(PairInteraction* result);
    void remove(PairInteraction* result);


    QList<QString> interactionModels() const;
    QList<PairInteraction*> filterByModel(const QString& model) const;
    QList<PairInteraction*> filterByComponent(const QString& component) const;
    QList<PairInteraction*> filterByModelAndComponent(const QString& model, const QString& component) const;

    int getCount(const QString &model = "") const;

signals:
    void interactionAdded();
    void interactionRemoved();
private:
    QMap<QString, PairInteractionList> m_pairInteractions;
};
