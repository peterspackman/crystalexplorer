#include "pair_energy_results.h"

PairInteraction::PairInteraction(const QString& interactionModel, QObject* parent)
        : QObject(parent), m_interactionModel(interactionModel) {}

void PairInteraction::addComponent(const QString& component, double value) {
    m_components.append(qMakePair(component, value));
}

QList<QPair<QString, double>> PairInteraction::components() const {
    return m_components; 
}

QString PairInteraction::interactionModel() const {
    return m_interactionModel;
}

void PairInteraction::setParameters(const pair_energy::Parameters &params) {
    m_parameters = params;
}
const pair_energy::Parameters& PairInteraction::parameters() const {
    return m_parameters;
}

PairInteractions::PairInteractions(QObject* parent) : QObject(parent) {}


int PairInteractions::getCount(const QString &model) const {
    int result = 0;
    if(model.isEmpty()) {
        for(const auto &l: m_pairInteractions.values()) {
            result += l.size();
        }
    }
    else {
        result = m_pairInteractions.value(model, {}).size();
    }
    return result;
}

QList<QString> PairInteractions::interactionModels() const {
    return m_pairInteractions.keys();
}

void PairInteractions::add(PairInteraction* result)
{
    qDebug() << "Adding interaction" << result;
    QString model = result->interactionModel();
    m_pairInteractions[model].append(result);
    emit interactionAdded();
}

QList<PairInteraction*> PairInteractions::filterByModel(const QString& model) const
{
    return m_pairInteractions.value(model);
}

void PairInteractions::remove(PairInteraction* result)
{
    QString model = result->interactionModel();
    QList<PairInteraction*>& models = m_pairInteractions[model];
    int index = models.indexOf(result);
    if (index >= 0) {
        models.removeAt(index);
        if (models.isEmpty()) {
            m_pairInteractions.remove(model);
        }
        emit interactionRemoved();
    }
}

QList<PairInteraction*> PairInteractions::filterByComponent(const QString& component) const
{
    QList<PairInteraction*> filtereds;
    for (const auto& models : m_pairInteractions) {
        for (PairInteraction* result : models) {
            for (const auto& pair : result->components()) {
                if (pair.first == component) {
                    filtereds.append(result);
                    break;
                }
            }
        }
    }
    return filtereds;
}

QList<PairInteraction*> PairInteractions::filterByModelAndComponent(const QString& model, const QString& component) const
{
    QList<PairInteraction*> filtereds;
    const auto& models = m_pairInteractions.value(model);
    for (PairInteraction* result : models) {
        for (const auto& pair : result->components()) {
            if (pair.first == component) {
                filtereds.append(result);
                break;
            }
        }
    }
    return filtereds;
}


