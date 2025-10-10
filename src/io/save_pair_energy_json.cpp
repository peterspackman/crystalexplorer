#include "save_pair_energy_json.h"
#include "chemicalstructure.h"
#include "crystalstructure.h"
#include "crystal_json.h"
#include "json.h"
#include <QFile>
#include <occ/crystal/dimer_mapping_table.h>

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

bool save_pair_interactions_for_model_json(const PairInteractions *interactions, const ChemicalStructure *structure, const QString &model, const QString &filename) {
    if (!interactions) {
        qWarning() << "Cannot save null PairInteractions to file:" << filename;
        return false;
    }

    if (!structure) {
        qWarning() << "Cannot save without ChemicalStructure to file:" << filename;
        return false;
    }

    auto modelInteractions = interactions->filterByModel(model);
    if (modelInteractions.empty()) {
        qWarning() << "No interactions found for model:" << model;
        return false;
    }

    const CrystalStructure *crystalStructure = qobject_cast<const CrystalStructure*>(structure);
    if (!crystalStructure) {
        qWarning() << "Structure must be a CrystalStructure to export elat_results.json:" << filename;
        return false;
    }

    nlohmann::json doc;
    doc["result_type"] = "elat";
    doc["model"] = model.toStdString();
    doc["title"] = structure->name().toStdString();
    doc["crystal"] = crystalStructure->occCrystal();

    // Get hasPermutationSymmetry from first interaction
    bool hasPermutationSymmetry = true;
    if (!modelInteractions.empty()) {
        hasPermutationSymmetry = modelInteractions.begin()->second->parameters().hasPermutationSymmetry;
    }
    doc["has_permutation_symmetry"] = hasPermutationSymmetry;

    // Get the dimer mapping table to ensure canonical dimer indices
    const auto &dimerMappingTable = crystalStructure->dimerMappingTable(hasPermutationSymmetry);

    // Helper to convert GenericAtomIndex to SiteIndex
    auto toSiteIndex = [](const GenericAtomIndex &gai) -> occ::crystal::SiteIndex {
        return {gai.unique, {gai.x, gai.y, gai.z}};
    };

    // Helper to convert Fragment to canonical DimerIndex
    auto getCanonicalDimerIndex = [&](const Fragment &fragA, const Fragment &fragB) -> occ::crystal::DimerIndex {
        // Use first atom of each fragment to define the dimer
        if (fragA.atomIndices.empty() || fragB.atomIndices.empty()) {
            qWarning() << "Empty fragment atom indices";
            return {};
        }

        occ::crystal::DimerIndex dimer{
            toSiteIndex(fragA.atomIndices[0]),
            toSiteIndex(fragB.atomIndices[0])
        };

        // Get symmetry-unique (canonical) form
        return dimerMappingTable.symmetry_unique_dimer(dimer);
    };

    // Build map from canonical dimer to unique index
    std::map<occ::crystal::DimerIndex, int> dimerToUniqueIndexMap;
    int uniqueIdx = 0;

    for (const auto &[fragmentPair, interaction] : modelInteractions) {
        const auto &params = interaction->parameters();
        auto canonicalDimer = getCanonicalDimerIndex(params.fragmentDimer.a, params.fragmentDimer.b);

        if (dimerToUniqueIndexMap.find(canonicalDimer) == dimerToUniqueIndexMap.end()) {
            dimerToUniqueIndexMap[canonicalDimer] = uniqueIdx++;
        }
    }

    // Group interactions by unique site index (offset of first atom in fragment A)
    std::map<int, std::vector<PairInteraction*>> interactionsByUniqueSite;
    for (const auto &[fragmentPair, interaction] : modelInteractions) {
        int uniqueSite = fragmentPair.a.u;
        interactionsByUniqueSite[uniqueSite].push_back(interaction);
    }

    // Find max unique site index
    int maxIdx = interactionsByUniqueSite.empty() ? 0 : interactionsByUniqueSite.rbegin()->first;

    nlohmann::json pairsArray = nlohmann::json::array();
    // Pre-size array with empty arrays
    for (int i = 0; i <= maxIdx; ++i) {
        pairsArray.push_back(nlohmann::json::array());
    }

    // Export all interactions grouped by unique site
    for (const auto &[uniqueSite, interactions] : interactionsByUniqueSite) {
        nlohmann::json fragmentNeighbors = nlohmann::json::array();

        for (auto *interaction : interactions) {
            const auto &params = interaction->parameters();
            const auto &fragA = params.fragmentDimer.a;
            const auto &fragB = params.fragmentDimer.b;

            // Get the canonical dimer index for this interaction
            auto canonicalDimer = getCanonicalDimerIndex(fragA, fragB);
            int uniqueIndex = dimerToUniqueIndexMap[canonicalDimer];

            nlohmann::json entry;
            entry["Label"] = interaction->label().toStdString();
            entry["Unique Index"] = uniqueIndex;

            // Nearest neighbor based on distance threshold (4.0 Angstroms)
            bool isNearestNeighbor = interaction->nearestAtomDistance() <= 4.0;
            entry["Nearest Neighbor"] = isNearestNeighbor;

            // Add energies (stored in kJ/mol in CE, write as-is)
            nlohmann::json energies = nlohmann::json::object();
            for (const auto &[component, value] : interaction->components()) {
                energies[component.toStdString()] = value;
            }
            entry["energies"] = energies;

            // Add uc_atom_offsets - need to get the actual Dimer object to access molecule data
            // For now, use the fragment atom indices which already contain the offsets
            nlohmann::json atomsAArray = nlohmann::json::array();
            for (const auto &atomIdx : fragA.atomIndices) {
                atomsAArray.push_back({atomIdx.unique, atomIdx.x, atomIdx.y, atomIdx.z});
            }

            nlohmann::json atomsBArray = nlohmann::json::array();
            for (const auto &atomIdx : fragB.atomIndices) {
                atomsBArray.push_back({atomIdx.unique, atomIdx.x, atomIdx.y, atomIdx.z});
            }

            entry["uc_atom_offsets"] = {atomsAArray, atomsBArray};

            fragmentNeighbors.push_back(entry);
        }

        pairsArray[uniqueSite] = fragmentNeighbors;
    }

    doc["pairs"] = pairsArray;

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