#include "pair_energy_results.h"

PairInteractionResult::PairInteractionResult(const QString& interactionModel, QObject* parent)
        : QObject(parent), m_interactionModel(interactionModel) {}

void PairInteractionResult::addComponent(const QString& component, double value) {
    m_components.append(qMakePair(component, value));
}

QList<QPair<QString, double>> PairInteractionResult::components() const {
    return m_components; 
}

QString PairInteractionResult::interactionModel() const {
    return m_interactionModel;
}

void PairInteractionResult::setParameters(const pair_energy::Parameters &params) {
    m_parameters = params;
}
const pair_energy::Parameters& PairInteractionResult::parameters() const {
    return m_parameters;
}

PairInteractionResults::PairInteractionResults(QObject* parent) : QObject(parent) {}


int PairInteractionResults::getCount(const QString &model) const {
    int result = 0;
    if(model.isEmpty()) {
        for(const auto &l: m_pairInteractionResults.values()) {
            result += l.size();
        }
    }
    else {
        result = m_pairInteractionResults.value(model, {}).size();
    }
    return result;
}

QList<QString> PairInteractionResults::interactionModels() const {
    return m_pairInteractionResults.keys();
}

void PairInteractionResults::addPairInteractionResult(PairInteractionResult* result)
{
    qDebug() << "Adding result" << result;
    QString model = result->interactionModel();
    m_pairInteractionResults[model].append(result);
    emit resultAdded();
}

QList<PairInteractionResult*> PairInteractionResults::filterByModel(const QString& model) const
{
    return m_pairInteractionResults.value(model);
}

void PairInteractionResults::removePairInteractionResult(PairInteractionResult* result)
{
    QString model = result->interactionModel();
    QList<PairInteractionResult*>& modelResults = m_pairInteractionResults[model];
    int index = modelResults.indexOf(result);
    if (index >= 0) {
        modelResults.removeAt(index);
        if (modelResults.isEmpty()) {
            m_pairInteractionResults.remove(model);
        }
        emit resultRemoved();
    }
}

QList<PairInteractionResult*> PairInteractionResults::filterByComponent(const QString& component) const
{
    QList<PairInteractionResult*> filteredResults;
    for (const auto& modelResults : m_pairInteractionResults) {
        for (PairInteractionResult* result : modelResults) {
            for (const auto& pair : result->components()) {
                if (pair.first == component) {
                    filteredResults.append(result);
                    break;
                }
            }
        }
    }
    return filteredResults;
}

QList<PairInteractionResult*> PairInteractionResults::filterByModelAndComponent(const QString& model, const QString& component) const
{
    QList<PairInteractionResult*> filteredResults;
    const auto& modelResults = m_pairInteractionResults.value(model);
    for (PairInteractionResult* result : modelResults) {
        for (const auto& pair : result->components()) {
            if (pair.first == component) {
                filteredResults.append(result);
                break;
            }
        }
    }
    return filteredResults;
}


