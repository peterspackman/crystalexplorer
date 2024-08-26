#include "load_pair_energy_json.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


PairInteraction * load_pair_energy_json(const QString& filename) {
    constexpr double hartree_to_kj_per_mol{2625.5};
    PairInteraction *result{nullptr};

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isObject())
        return result;

    QJsonObject jsonObj = jsonDoc.object();

    // Extract interaction model
    QJsonObject interactionModelObj = jsonObj["interaction_model"].toObject();
    QString interactionModel = interactionModelObj["name"].toString();


    result = new PairInteraction(interactionModel);
    QJsonObject interactionEnergyObj = jsonObj["interaction_energy"].toObject();
    for (const QString& component : interactionEnergyObj.keys()) {
        double value = interactionEnergyObj[component].toDouble() * hartree_to_kj_per_mol;
        result->addComponent(component, value);
    }
    return result;
}
