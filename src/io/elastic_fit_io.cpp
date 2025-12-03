#include "elastic_fit_io.h"
#include "chemicalstructure.h"
#include "crystalstructure.h"
#include "fragment_index.h"
#include "json.h"
#include <QFile>

bool save_elastic_fit_pairs_json(const PairInteractions *interactions,
                                  const ChemicalStructure *structure,
                                  const QString &model,
                                  const QString &filename) {
    if (!interactions || !structure) {
        qWarning() << "Invalid interactions or structure pointer";
        return false;
    }

    const auto *crystalStructure = qobject_cast<const CrystalStructure *>(structure);
    if (!crystalStructure) {
        qWarning() << "Structure is not a CrystalStructure";
        return false;
    }

    // Get interactions filtered by model
    auto modelInteractions = interactions->filterByModel(model);
    if (modelInteractions.empty()) {
        qWarning() << "No interactions found for model:" << model;
        return false;
    }

    // Check if we have permutation symmetry (inversion)
    bool hasPermutationSymmetry = interactions->hasPermutationSymmetry();

    nlohmann::json doc;
    doc["format_type"] = "elastic_fit_pairs";
    doc["format_version"] = "1.0";
    doc["model"] = model.toStdString();

    // Lattice vectors (3x3 matrix, column-major as rows)
    const auto &cell = crystalStructure->occCrystal().unit_cell();
    occ::Mat3 direct = cell.direct();
    doc["lattice_vectors"] = {
        {direct(0, 0), direct(1, 0), direct(2, 0)},
        {direct(0, 1), direct(1, 1), direct(2, 1)},
        {direct(0, 2), direct(1, 2), direct(2, 2)}
    };

    // Volume in Angstrom^3
    doc["volume"] = cell.volume();

    // Molecules - use unit_cell_molecules() directly
    const auto &ucMols = crystalStructure->occCrystal().unit_cell_molecules();

    nlohmann::json moleculesArray = nlohmann::json::array();
    for (size_t i = 0; i < ucMols.size(); ++i) {
        const auto &mol = ucMols[i];

        nlohmann::json molJson;
        molJson["id"] = static_cast<int>(i);
        molJson["mass"] = mol.molar_mass();

        occ::Vec3 com = mol.center_of_mass();
        molJson["center_of_mass"] = {com(0), com(1), com(2)};

        moleculesArray.push_back(molJson);
    }
    doc["molecules"] = moleculesArray;

    // Get the dimer mapping table to find symmetry-unique dimers
    const auto &dimerTable = crystalStructure->dimerMappingTable(hasPermutationSymmetry);
    const auto &crystalDimers = crystalStructure->unitCellDimers();

    // Pairs - iterate over ALL molecule neighbors
    nlohmann::json pairsArray = nlohmann::json::array();

    for (size_t asymIdx = 0; asymIdx < crystalDimers.molecule_neighbors.size(); ++asymIdx) {
        const auto &neighbors = crystalDimers.molecule_neighbors[asymIdx];

        for (const auto &neighbor : neighbors) {
            const auto &dimer = neighbor.dimer;

            // Get unit cell molecule indices
            int molA = dimer.a().unit_cell_molecule_idx();
            int molB = dimer.b().unit_cell_molecule_idx();

            // Create DimerIndex from the molecule positions
            occ::Vec3 posA = crystalStructure->occCrystal().to_fractional(dimer.a().centroid());
            occ::Vec3 posB = crystalStructure->occCrystal().to_fractional(dimer.b().centroid());
            occ::crystal::DimerIndex dimerIdx = dimerTable.dimer_index(posA, posB);

            // Get the symmetry-unique dimer
            occ::crystal::DimerIndex canonicalIdx = dimerTable.canonical_dimer_index(dimerIdx);
            occ::crystal::DimerIndex symmetryUniqueIdx = dimerTable.symmetry_unique_dimer(canonicalIdx);

            // Convert to FragmentIndexPair to look up in stored interactions
            FragmentIndexPair uniquePair = FragmentIndexPair::fromDimerIndex(symmetryUniqueIdx);

            // Look up energy from stored interactions
            double energy = 0.0;
            auto it = modelInteractions.find(uniquePair);
            if (it != modelInteractions.end()) {
                energy = it->second->getComponent("Total");
            } else if (hasPermutationSymmetry) {
                // Try reversed pair
                FragmentIndexPair reversedPair{uniquePair.b, uniquePair.a};
                it = modelInteractions.find(reversedPair);
                if (it != modelInteractions.end()) {
                    energy = it->second->getComponent("Total");
                }
            }

            // Skip pairs with zero energy (no interaction calculated)
            if (std::abs(energy) < 1e-10) {
                continue;
            }

            // Get center of mass positions and vector
            occ::Vec3 comA = dimer.a().center_of_mass();
            occ::Vec3 comB = dimer.b().center_of_mass();
            occ::Vec3 v_ab = comB - comA;

            nlohmann::json pairJson;
            pairJson["molecule_a"] = molA;
            pairJson["molecule_b"] = molB;
            pairJson["v_ab_com"] = {v_ab(0), v_ab(1), v_ab(2)};
            pairJson["energy"] = energy;

            pairsArray.push_back(pairJson);
        }
    }
    doc["pairs"] = pairsArray;

    // Write to file
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filename;
        return false;
    }

    std::string jsonStr = doc.dump(2);
    file.write(jsonStr.c_str(), jsonStr.size());
    file.close();

    return true;
}
