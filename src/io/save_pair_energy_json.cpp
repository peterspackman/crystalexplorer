#include "save_pair_energy_json.h"
#include "json.h"
#include <QFile>

bool save_pair_energy_json(const PairInteraction *interaction, const QString &filename) {
    constexpr double kj_per_mol_to_hartree{1.0 / 2625.5};

    if (!interaction) {
        qWarning() << "Cannot save null PairInteraction to file:" << filename;
        return false;
    }

    nlohmann::json doc;

    // Set interaction model
    doc["interaction_model"]["name"] = interaction->interactionModel();

    // Set interaction energies (convert from kJ/mol to Hartree)
    nlohmann::json energies = nlohmann::json::object();
    for (const auto &[component, value] : interaction->components()) {
        energies[component.toStdString()] = value * kj_per_mol_to_hartree;
    }
    doc["interaction_energy"] = energies;

    // Write to file
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Couldn't open file for writing:" << filename;
        return false;
    }

    QString jsonString = QString::fromStdString(doc.dump(2));
    QByteArray data = jsonString.toUtf8();

    qint64 bytesWritten = file.write(data);
    file.close();

    if (bytesWritten != data.size()) {
        qWarning() << "Failed to write complete data to file:" << filename;
        return false;
    }

    return true;
}

bool save_pair_interactions_json(const PairInteractions *interactions, const QString &filename) {
    if (!interactions) {
        qWarning() << "Cannot save null PairInteractions to file:" << filename;
        return false;
    }

    nlohmann::json doc = interactions->toJson();

    // Write to file
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Couldn't open file for writing:" << filename;
        return false;
    }

    QString jsonString = QString::fromStdString(doc.dump(2));
    QByteArray data = jsonString.toUtf8();

    qint64 bytesWritten = file.write(data);
    file.close();

    if (bytesWritten != data.size()) {
        qWarning() << "Failed to write complete data to file:" << filename;
        return false;
    }

    return true;
}