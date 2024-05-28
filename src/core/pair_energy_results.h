#pragma once
#include <QObject>
#include <QAbstractItemModel>
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

class PairInteractionResults : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit PairInteractionResults(QObject* parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    void addPairInteractionResult(PairInteractionResult* result);
    void removePairInteractionResult(PairInteractionResult* result);

    inline const auto &pairInteractionResults() const { return m_pairInteractionResults; }

    QList<PairInteractionResult*> filterByModel(const QString& model) const;
    QList<PairInteractionResult*> filterByComponent(const QString& component) const;
    QList<PairInteractionResult*> filterByModelAndComponent(const QString& model, const QString& component) const;

signals:
    void resultAdded();
    void resultRemoved();
private:
    QList<PairInteractionResult*> m_pairInteractionResults;
};
