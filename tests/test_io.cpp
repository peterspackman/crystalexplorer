#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <QString>
#include "genericxyzfile.h"
#include "save_pair_energy_json.h"
#include "load_pair_energy_json.h"
#include "pair_energy_results.h"
#include "crystalclear.h"
#include "crystalstructure.h"
#include <QTemporaryFile>
#include <QFile>
#include <QDir>

using Catch::Approx;

TEST_CASE("GenericXYZFile correct format", "[io][xyz]") {
    QString testData = 
        "2\n"
        "x y z e neighbor\n"
        "1.0 2.0 3.0 4.0 5\n"
        "1.1 2.1 3.1 4.1 6\n";

    GenericXYZFile xyzFile;
    bool success = xyzFile.readFromString(testData);
    
    SECTION("Reading succeeds") {
        REQUIRE(success);
    }
    
    SECTION("Column names are correct") {
        auto columnNames = xyzFile.columnNames();
        REQUIRE(columnNames[0] == "x");
        REQUIRE(columnNames[4] == "neighbor");
    }
    
    SECTION("Data values are correct") {
        const auto &neighbors = xyzFile.column("neighbor");
        REQUIRE(neighbors.rows() == 2);
        REQUIRE(neighbors(1) == Approx(6.0f));
    }
}

TEST_CASE("GenericXYZFile incorrect format", "[io][xyz]") {
    QString testData = 
        "2\n"
        "x y z e neighbor\n"
        "1.0 2.0 3.0 4.0\n"  // Missing one value
        "1.1 2.1 3.1 4.1 6\n";

    GenericXYZFile xyzFile;
    bool success = xyzFile.readFromString(testData);
    
    REQUIRE_FALSE(success); // Expect failure due to incorrect format
}

TEST_CASE("GenericXYZFile empty file", "[io][xyz]") {
    QString testData;

    GenericXYZFile xyzFile;
    bool success = xyzFile.readFromString(testData);

    REQUIRE_FALSE(success); // Expect failure due to empty content
}

TEST_CASE("save_pair_energy_json functionality", "[io][pair_energy]") {

    SECTION("Save single PairInteraction to elat_results.json format") {
        auto *interaction = new PairInteraction("GFN2-xTB");
        interaction->addComponent("Total", 42.5); // kJ/mol
        interaction->addComponent("Electrostatic", 15.3);
        interaction->addComponent("Exchange", -8.7);

        QTemporaryFile tempFile;
        tempFile.setFileTemplate(QDir::tempPath() + "/test_elat_results_XXXXXX.json");
        REQUIRE(tempFile.open());
        QString filename = tempFile.fileName();
        tempFile.close();

        bool saveResult = save_pair_energy_json(interaction, filename);
        REQUIRE(saveResult);

        QFile file(filename);
        REQUIRE(file.open(QIODevice::ReadOnly));
        QByteArray data = file.readAll();
        file.close();

        QString content = QString::fromUtf8(data);
        REQUIRE(content.contains("interaction_model"));
        REQUIRE(content.contains("GFN2-xTB"));
        REQUIRE(content.contains("interaction_energy"));
        REQUIRE(content.contains("Total"));
        REQUIRE(content.contains("Electrostatic"));
        REQUIRE(content.contains("Exchange"));

        delete interaction;
    }

    SECTION("Round-trip test: save then load") {
        auto *originalInteraction = new PairInteraction("CE-B3LYP");
        originalInteraction->addComponent("Total", 123.45); // kJ/mol
        originalInteraction->addComponent("Coulomb", 67.89);
        originalInteraction->addComponent("Dispersion", -12.34);

        QTemporaryFile tempFile;
        tempFile.setFileTemplate(QDir::tempPath() + "/test_roundtrip_XXXXXX.json");
        REQUIRE(tempFile.open());
        QString filename = tempFile.fileName();
        tempFile.close();

        bool saveResult = save_pair_energy_json(originalInteraction, filename);
        REQUIRE(saveResult);

        auto *loadedInteraction = load_pair_energy_json(filename);
        REQUIRE(loadedInteraction != nullptr);

        REQUIRE(loadedInteraction->interactionModel() == "CE-B3LYP");

        REQUIRE_THAT(loadedInteraction->getComponent("Total"),
                     Catch::Matchers::WithinAbs(123.45, 0.01));
        REQUIRE_THAT(loadedInteraction->getComponent("Coulomb"),
                     Catch::Matchers::WithinAbs(67.89, 0.01));
        REQUIRE_THAT(loadedInteraction->getComponent("Dispersion"),
                     Catch::Matchers::WithinAbs(-12.34, 0.01));

        delete originalInteraction;
        delete loadedInteraction;
    }

    SECTION("Save null interaction should fail gracefully") {
        QTemporaryFile tempFile;
        REQUIRE(tempFile.open());
        QString filename = tempFile.fileName();
        tempFile.close();

        bool result = save_pair_energy_json(nullptr, filename);
        REQUIRE_FALSE(result);
    }
}

TEST_CASE("elat_results.json round-trip", "[io][elat]") {
    // Load an existing elat_results.json file
    QString elatFile = "build/PYRAZI01_elat_results.json";

    if (!QFile::exists(elatFile)) {
        SKIP("Test file not found: " + elatFile.toStdString());
    }

    SECTION("Load, save, and reload elat_results.json") {
        // Load original
        auto *originalStructure = io::loadCrystalClearJson(elatFile);
        REQUIRE(originalStructure != nullptr);

        auto *originalInteractions = originalStructure->pairInteractions();
        REQUIRE(originalInteractions != nullptr);

        QStringList models = originalInteractions->interactionModels();
        REQUIRE(!models.isEmpty());
        QString model = models.first();

        int originalCount = originalInteractions->getCount(model);
        REQUIRE(originalCount > 0);

        // Save to temp file
        QTemporaryFile tempFile;
        tempFile.setFileTemplate(QDir::tempPath() + "/test_elat_roundtrip_XXXXXX.json");
        REQUIRE(tempFile.open());
        QString filename = tempFile.fileName();
        tempFile.close();

        bool saveResult = save_pair_interactions_for_model_json(
            originalInteractions, originalStructure, model, filename);
        REQUIRE(saveResult);

        // Reload from temp file
        auto *reloadedStructure = io::loadCrystalClearJson(filename);
        REQUIRE(reloadedStructure != nullptr);

        auto *reloadedInteractions = reloadedStructure->pairInteractions();
        REQUIRE(reloadedInteractions != nullptr);

        int reloadedCount = reloadedInteractions->getCount(model);
        REQUIRE(reloadedCount == originalCount);

        // Compare interaction maps
        auto originalMap = originalInteractions->filterByModel(model);
        auto reloadedMap = reloadedInteractions->filterByModel(model);

        REQUIRE(originalMap.size() == reloadedMap.size());

        // Verify energies match for each interaction
        int energyMatches = 0;
        constexpr double kj_per_mol_to_hartree = 1.0 / 2625.5;

        for (const auto &[fragPair, origInteraction] : originalMap) {
            // Find matching interaction in reloaded map
            auto it = reloadedMap.find(fragPair);
            REQUIRE(it != reloadedMap.end());

            auto *reloadedInteraction = it->second;

            // Compare energy components
            const auto &origComponents = origInteraction->components();
            const auto &reloadedComponents = reloadedInteraction->components();

            REQUIRE(origComponents.size() == reloadedComponents.size());

            for (const auto &[component, origValue] : origComponents) {
                REQUIRE(reloadedComponents.contains(component));
                auto it = reloadedComponents.find(component);
                REQUIRE(it != reloadedComponents.end());
                double reloadedValue = it->second;

                // Values should match within conversion tolerance
                // Original is in Hartree, we convert to kJ/mol and back
                REQUIRE_THAT(reloadedValue,
                    Catch::Matchers::WithinAbs(origValue, 0.01));
                energyMatches++;
            }

            // Verify fragment indices match
            REQUIRE(origInteraction->pairIndex() == reloadedInteraction->pairIndex());

            // Verify distances match
            REQUIRE_THAT(origInteraction->nearestAtomDistance(),
                Catch::Matchers::WithinAbs(reloadedInteraction->nearestAtomDistance(), 0.001));
        }

        REQUIRE(energyMatches > 0); // Ensure we actually compared some energies

        // Verify crystal structure is preserved
        REQUIRE(originalStructure->name() == reloadedStructure->name());

        // Compare number of atoms in unit cell
        const auto &origCrystal = originalStructure->occCrystal();
        const auto &reloadedCrystal = reloadedStructure->occCrystal();
        REQUIRE(origCrystal.unit_cell_atoms().size() == reloadedCrystal.unit_cell_atoms().size());

        delete originalStructure;
        delete reloadedStructure;
    }
}