#pragma once
#include <QObject>
#include <QColor>
#include "pair_energy_parameters.h"
#include <ankerl/unordered_dense.h>

class PairInteraction: public QObject
{
    Q_OBJECT
public:
    explicit PairInteraction(const QString& interactionModel, QObject* parent = nullptr);
    QString interactionModel() const;
    void addComponent(const QString& component, double value);
    inline const auto &components() const { return m_components; }
    double getComponent(const QString &) const;

    inline const QString &symmetry() const { return m_parameters.symmetry; }
    inline double nearestAtomDistance() const { return m_parameters.nearestAtomDistance; }
    inline double centroidDistance() const { return m_parameters.centroidDistance; }

    inline const auto &color() const { return m_color; }
    inline void setColor(QColor color) { m_color = color; }

    void setParameters(const pair_energy::Parameters&);
    const pair_energy::Parameters &parameters() const;

private:
    QColor m_color{Qt::blue};
    QString m_interactionModel;
    ankerl::unordered_dense::map<QString, double> m_components;
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

    QMap<QString, PairInteractionList> getInteractionsMatchingFragments(const std::vector<FragmentDimer> &frag);

    PairInteraction *getInteraction(const QString &model, const FragmentDimer &frag);

    QList<QString> interactionModels() const;
    QList<QString> interactionComponents(const QString &model);
    QList<PairInteraction*> filterByModel(const QString& model) const;
    QList<PairInteraction*> filterByComponent(const QString& component) const;
    QList<PairInteraction*> filterByModelAndComponent(const QString& model, const QString& component) const;

    int getCount(const QString &model = "") const;

signals:
    void interactionAdded();
    void interactionRemoved();
private:
    QMap<QString, PairInteractionList> m_pairInteractions;
    QMap<QString, double> m_maxNearestDistance;
};
