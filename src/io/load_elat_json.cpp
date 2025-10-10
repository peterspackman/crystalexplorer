#include "load_elat_json.h"
#include "crystalstructure.h"
#include "pairinteractions.h"
#include "pairinteraction.h"
#include <occ/interaction/interaction_json.h>
#include <QDebug>
#include <QFile>

namespace io {

CrystalStructure* loadElatJson(const QString& filename) {
    // Use occ's native JSON loader
    occ::interaction::ElatResults elatData;

    try {
        elatData = occ::interaction::read_elat_json(filename.toStdString());
    } catch (const std::exception& e) {
        qCritical() << "Failed to read elat JSON:" << e.what();
        return nullptr;
    }

    qDebug() << "Loaded elat data:";
    qDebug() << "  Title:" << QString::fromStdString(elatData.title);
    qDebug() << "  Model:" << QString::fromStdString(elatData.model);

    const auto& dimers = elatData.lattice_energy_result.dimers;
    qDebug() << "  Unique dimers:" << dimers.unique_dimers.size();
    qDebug() << "  Sites:" << dimers.molecule_neighbors.size();

    // Count total dimer instances
    size_t totalInstances = 0;
    for (const auto& site : dimers.molecule_neighbors) {
        totalInstances += site.size();
    }
    qDebug() << "  Total dimer instances (all symmetry-related):" << totalInstances;

    // Create CrystalStructure from the occ::crystal::Crystal
    CrystalStructure* structure = new CrystalStructure();
    structure->setOccCrystal(elatData.crystal);
    structure->setName(QString::fromStdString(elatData.title));
    structure->generateFragmentsFromCrystal();

    // Create PairInteractions container
    PairInteractions* pairInteractions = new PairInteractions();
    structure->setPairInteractions(pairInteractions);

    QString model = QString::fromStdString(elatData.model);

    // Process each unique dimer
    for (size_t uniqueIdx = 0; uniqueIdx < dimers.unique_dimers.size(); ++uniqueIdx) {
        const auto& uniqueDimer = dimers.unique_dimers[uniqueIdx];

        // Get energy components
        const auto& energies = uniqueDimer.interaction_energies();

        // Create PairInteraction for this unique dimer
        PairInteraction* interaction = new PairInteraction();
        interaction->setLabel(QString::number(uniqueIdx + 1));  // Simple numeric label

        // Set energy components
        QMap<QString, double> components;
        for (const auto& [key, value] : energies) {
            components[QString::fromStdString(key)] = value;
        }
        interaction->setComponents(components);

        // We need to create fragments from the unique dimer
        // For now, store a placeholder - we'll need to reconstruct the FragmentDimer
        // from the first instance of this unique dimer in molecule_neighbors

        // Find first instance of this unique dimer
        bool found = false;
        for (const auto& site : dimers.molecule_neighbors) {
            for (const auto& [dimer, idx] : site) {
                if (idx == static_cast<int>(uniqueIdx)) {
                    // Found an instance of this unique dimer
                    // Now convert occ::core::Dimer to CE's FragmentDimer

                    const auto& molA = dimer.a();
                    const auto& molB = dimer.b();

                    // Get atom indices with offsets
                    std::vector<GenericAtomIndex> atomsA, atomsB;

                    const auto& a_uc_idx = molA.unit_cell_idx();
                    const auto& a_uc_shift = molA.unit_cell_atom_shift();
                    for (int i = 0; i < a_uc_idx.rows(); i++) {
                        GenericAtomIndex atom;
                        atom.unique = a_uc_idx(i);
                        atom.x = a_uc_shift(0, i);
                        atom.y = a_uc_shift(1, i);
                        atom.z = a_uc_shift(2, i);
                        atomsA.push_back(atom);
                    }

                    const auto& b_uc_idx = molB.unit_cell_idx();
                    const auto& b_uc_shift = molB.unit_cell_atom_shift();
                    for (int i = 0; i < b_uc_idx.rows(); i++) {
                        GenericAtomIndex atom;
                        atom.unique = b_uc_idx(i);
                        atom.x = b_uc_shift(0, i);
                        atom.y = b_uc_shift(1, i);
                        atom.z = b_uc_shift(2, i);
                        atomsB.push_back(atom);
                    }

                    // Create fragments
                    Fragment fragA = structure->makeFragment(atomsA);
                    Fragment fragB = structure->makeFragment(atomsB);

                    // Set dimer parameters
                    pair_energy::Parameters params;
                    params.fragmentDimer = FragmentDimer(fragA, fragB);

                    // Calculate nearest atom distance
                    double nearestDist = dimer.nearest_distance();
                    params.nearestAtomDistance = nearestDist;

                    interaction->setParameters(params);

                    found = true;
                    break;
                }
            }
            if (found) break;
        }

        if (!found) {
            qWarning() << "Could not find instance for unique dimer" << uniqueIdx;
            delete interaction;
            continue;
        }

        // Add to PairInteractions
        pairInteractions->add(interaction);
    }

    qDebug() << "Created" << pairInteractions->getCount(model) << "pair interactions";

    return structure;
}

} // namespace io
