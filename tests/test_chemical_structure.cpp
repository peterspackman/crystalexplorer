#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "chemicalstructure.h"
#include <QColor>

using Catch::Approx;

TEST_CASE("ChemicalStructure basic functionality", "[core][chemical_structure]") {
    ChemicalStructure structure;
    
    SECTION("Initial state") {
        REQUIRE(structure.numberOfAtoms() == 0);
        REQUIRE(structure.name() == "structure");
        REQUIRE(structure.structureType() == ChemicalStructure::StructureType::Cluster);
        REQUIRE(structure.numberOfFragments() == 0);
    }
    
    SECTION("Set atoms") {
        std::vector<QString> elements = {"H", "C", "O"};
        std::vector<occ::Vec3> positions = {
            occ::Vec3(0.0, 0.0, 0.0),
            occ::Vec3(1.0, 0.0, 0.0), 
            occ::Vec3(2.0, 0.0, 0.0)
        };
        std::vector<QString> labels = {"H1", "C1", "O1"};
        
        structure.setAtoms(elements, positions, labels);
        
        REQUIRE(structure.numberOfAtoms() == 3);
        REQUIRE(structure.atomicNumbers()[0] == 1); // H
        REQUIRE(structure.atomicNumbers()[1] == 6); // C
        REQUIRE(structure.atomicNumbers()[2] == 8); // O
        REQUIRE(structure.labels()[0] == "H1");
        REQUIRE(structure.labels()[1] == "C1");
        REQUIRE(structure.labels()[2] == "O1");
    }
    
    SECTION("Add atoms") {
        std::vector<QString> elements1 = {"H", "C"};
        std::vector<occ::Vec3> positions1 = {
            occ::Vec3(0.0, 0.0, 0.0),
            occ::Vec3(1.0, 0.0, 0.0)
        };
        
        structure.setAtoms(elements1, positions1);
        REQUIRE(structure.numberOfAtoms() == 2);
        
        std::vector<QString> elements2 = {"O"};
        std::vector<occ::Vec3> positions2 = {occ::Vec3(2.0, 0.0, 0.0)};
        
        structure.addAtoms(elements2, positions2);
        REQUIRE(structure.numberOfAtoms() == 3);
        REQUIRE(structure.atomicNumbers()[2] == 8); // O
    }
    
    SECTION("Clear atoms") {
        std::vector<QString> elements = {"H", "C"};
        std::vector<occ::Vec3> positions = {
            occ::Vec3(0.0, 0.0, 0.0),
            occ::Vec3(1.0, 0.0, 0.0)
        };
        
        structure.setAtoms(elements, positions);
        REQUIRE(structure.numberOfAtoms() == 2);
        
        structure.clearAtoms();
        REQUIRE(structure.numberOfAtoms() == 0);
    }
}

TEST_CASE("ChemicalStructure atom properties", "[core][chemical_structure]") {
    ChemicalStructure structure;
    std::vector<QString> elements = {"H", "C", "N", "O", "H"};
    std::vector<occ::Vec3> positions = {
        occ::Vec3(0.0, 0.0, 0.0),
        occ::Vec3(1.0, 0.0, 0.0),
        occ::Vec3(2.0, 0.0, 0.0),
        occ::Vec3(3.0, 0.0, 0.0),
        occ::Vec3(4.0, 0.0, 0.0)
    };
    structure.setAtoms(elements, positions);
    
    SECTION("Unique element symbols") {
        auto unique = structure.uniqueElementSymbols();
        REQUIRE(unique.size() == 4);
        REQUIRE(unique.contains("H"));
        REQUIRE(unique.contains("C"));
        REQUIRE(unique.contains("N"));
        REQUIRE(unique.contains("O"));
    }
    
    SECTION("Chemical formula") {
        auto formula = structure.chemicalFormula(false);
        REQUIRE(formula.contains("H"));
        REQUIRE(formula.contains("C"));
        REQUIRE(formula.contains("N"));
        REQUIRE(formula.contains("O"));
    }
    
    SECTION("Atom position access") {
        GenericAtomIndex idx{0, 0, 0, 0};
        auto pos = structure.atomPosition(idx);
        REQUIRE(pos[0] == Approx(0.0));
        REQUIRE(pos[1] == Approx(0.0));
        REQUIRE(pos[2] == Approx(0.0));
    }
    
    SECTION("Index conversion") {
        GenericAtomIndex generic = structure.indexToGenericIndex(0);
        int converted = structure.genericIndexToIndex(generic);
        REQUIRE(converted == 0);
        
        GenericAtomIndex generic2 = structure.indexToGenericIndex(2);
        int converted2 = structure.genericIndexToIndex(generic2);
        REQUIRE(converted2 == 2);
    }
}

TEST_CASE("ChemicalStructure atom flags", "[core][chemical_structure][flags]") {
    ChemicalStructure structure;
    std::vector<QString> elements = {"H", "C", "O"};
    std::vector<occ::Vec3> positions = {
        occ::Vec3(0.0, 0.0, 0.0),
        occ::Vec3(1.0, 0.0, 0.0),
        occ::Vec3(2.0, 0.0, 0.0)
    };
    structure.setAtoms(elements, positions);
    
    GenericAtomIndex idx0 = structure.indexToGenericIndex(0);
    GenericAtomIndex idx1 = structure.indexToGenericIndex(1);
    GenericAtomIndex idx2 = structure.indexToGenericIndex(2);
    
    SECTION("Set and test single atom flag") {
        structure.setAtomFlag(idx0, AtomFlag::Selected, true);
        REQUIRE(structure.testAtomFlag(idx0, AtomFlag::Selected));
        REQUIRE_FALSE(structure.testAtomFlag(idx1, AtomFlag::Selected));
        
        structure.setAtomFlag(idx0, AtomFlag::Selected, false);
        REQUIRE_FALSE(structure.testAtomFlag(idx0, AtomFlag::Selected));
    }
    
    SECTION("Toggle atom flag") {
        REQUIRE_FALSE(structure.testAtomFlag(idx1, AtomFlag::Selected));
        structure.toggleAtomFlag(idx1, AtomFlag::Selected);
        REQUIRE(structure.testAtomFlag(idx1, AtomFlag::Selected));
        structure.toggleAtomFlag(idx1, AtomFlag::Selected);
        REQUIRE_FALSE(structure.testAtomFlag(idx1, AtomFlag::Selected));
    }
    
    SECTION("Set flag for all atoms") {
        structure.setFlagForAllAtoms(AtomFlag::Selected, true);
        REQUIRE(structure.testAtomFlag(idx0, AtomFlag::Selected));
        REQUIRE(structure.testAtomFlag(idx1, AtomFlag::Selected));
        REQUIRE(structure.testAtomFlag(idx2, AtomFlag::Selected));
        
        structure.setFlagForAllAtoms(AtomFlag::Selected, false);
        REQUIRE_FALSE(structure.testAtomFlag(idx0, AtomFlag::Selected));
        REQUIRE_FALSE(structure.testAtomFlag(idx1, AtomFlag::Selected));
        REQUIRE_FALSE(structure.testAtomFlag(idx2, AtomFlag::Selected));
    }
    
    SECTION("Set flag for specific atoms") {
        std::vector<GenericAtomIndex> atoms = {idx0, idx2};
        structure.setFlagForAtoms(atoms, AtomFlag::Selected, true);
        
        REQUIRE(structure.testAtomFlag(idx0, AtomFlag::Selected));
        REQUIRE_FALSE(structure.testAtomFlag(idx1, AtomFlag::Selected));
        REQUIRE(structure.testAtomFlag(idx2, AtomFlag::Selected));
    }
    
    SECTION("Check atoms with flags") {
        structure.setAtomFlag(idx0, AtomFlag::Selected, true);
        structure.setAtomFlag(idx1, AtomFlag::Contact, true);
        
        AtomFlags selectedFlag = AtomFlag::Selected;
        
        auto selectedAtoms = structure.atomsWithFlags(selectedFlag, true);
        REQUIRE(selectedAtoms.size() == 1);
        REQUIRE(selectedAtoms[0] == idx0);
        
        REQUIRE(structure.anyAtomHasFlags(selectedFlag));
        REQUIRE_FALSE(structure.allAtomsHaveFlags(selectedFlag));
    }
}

TEST_CASE("ChemicalStructure atom coloring", "[core][chemical_structure][coloring]") {
    ChemicalStructure structure;
    std::vector<QString> elements = {"H", "C", "O"};
    std::vector<occ::Vec3> positions = {
        occ::Vec3(0.0, 0.0, 0.0),
        occ::Vec3(1.0, 0.0, 0.0),
        occ::Vec3(2.0, 0.0, 0.0)
    };
    structure.setAtoms(elements, positions);
    
    GenericAtomIndex idx0 = structure.indexToGenericIndex(0);
    GenericAtomIndex idx1 = structure.indexToGenericIndex(1);
    
    SECTION("Default atom coloring") {
        structure.setAtomColoring(ChemicalStructure::AtomColoring::Element);
        auto color = structure.atomColor(idx0);
        REQUIRE(color.isValid());
    }
    
    SECTION("Override atom color") {
        QColor red(255, 0, 0);
        structure.overrideAtomColor(idx0, red);
        auto color = structure.atomColor(idx0);
        REQUIRE(color == red);
        
        structure.resetAtomColorOverrides();
        auto resetColor = structure.atomColor(idx0);
        REQUIRE(resetColor != red);
    }
    
    SECTION("Set color for atoms with flags") {
        structure.setAtomFlag(idx0, AtomFlag::Selected, true);
        
        AtomFlags selectedFlag = AtomFlag::Selected;
        QColor blue(0, 0, 255);
        
        structure.setColorForAtomsWithFlags(selectedFlag, blue);
        auto color = structure.atomColor(idx0);
        REQUIRE(color == blue);
    }
}

TEST_CASE("ChemicalStructure name and metadata", "[core][chemical_structure]") {
    ChemicalStructure structure;
    
    SECTION("Set and get name") {
        structure.setName("Test Structure");
        REQUIRE(structure.name() == "Test Structure");
    }
    
    SECTION("Filename and file contents") {
        QString filename = "test.cif";
        QByteArray contents = "test file contents";
        
        structure.setFilename(filename);
        structure.setFileContents(contents);
        
        REQUIRE(structure.filename() == filename);
        REQUIRE(structure.fileContents() == contents);
    }
}

TEST_CASE("ChemicalStructure origin and radius", "[core][chemical_structure]") {
    ChemicalStructure structure;
    std::vector<QString> elements = {"H", "C"};
    std::vector<occ::Vec3> positions = {
        occ::Vec3(0.0, 0.0, 0.0),
        occ::Vec3(3.0, 4.0, 0.0)  // Distance 5.0 from origin
    };
    structure.setAtoms(elements, positions);
    
    SECTION("Default origin") {
        auto origin = structure.origin();
        // Origin is calculated as center of atoms
        REQUIRE(origin[0] == Approx(1.5)); // Center of atoms at 0.0 and 3.0
        REQUIRE(origin[1] == Approx(2.0)); // Center of atoms at 0.0 and 4.0
        REQUIRE(origin[2] == Approx(0.0));
    }
    
    SECTION("Set origin") {
        occ::Vec3 newOrigin(1.0, 2.0, 3.0);
        structure.setOrigin(newOrigin);
        auto origin = structure.origin();
        REQUIRE(origin[0] == Approx(1.0));
        REQUIRE(origin[1] == Approx(2.0));
        REQUIRE(origin[2] == Approx(3.0));
    }
    
    SECTION("Reset origin") {
        structure.setOrigin(occ::Vec3(1.0, 2.0, 3.0));
        structure.resetOrigin();
        auto origin = structure.origin();
        // Reset origin recalculates center
        REQUIRE(origin[0] == Approx(1.5));
        REQUIRE(origin[1] == Approx(2.0));
        REQUIRE(origin[2] == Approx(0.0));
    }
    
    SECTION("Structure radius") {
        float radius = structure.radius();
        REQUIRE(radius > 0.0f);
    }
}

TEST_CASE("ChemicalStructure JSON serialization", "[core][chemical_structure][json]") {
    ChemicalStructure structure;
    std::vector<QString> elements = {"H", "C"};
    std::vector<occ::Vec3> positions = {
        occ::Vec3(0.0, 0.0, 0.0),
        occ::Vec3(1.0, 0.0, 0.0)
    };
    structure.setAtoms(elements, positions);
    structure.setName("Test JSON Structure");
    
    SECTION("To JSON") {
        auto json = structure.toJson();
        REQUIRE(json.contains("atomicNumbers"));
        REQUIRE(json.contains("atomicPositions"));
        // Check that we have the right number of atoms
        REQUIRE(json["atomicNumbers"].size() == 2);
        REQUIRE(json["atomicPositions"].size() == 3);
        REQUIRE(json["atomicPositions"][0].size() == 2);
    }
    
    SECTION("From JSON round trip") {
        auto originalJson = structure.toJson();
        ChemicalStructure newStructure;
        bool success = newStructure.fromJson(originalJson);
        
        REQUIRE(success);
        REQUIRE(newStructure.numberOfAtoms() == structure.numberOfAtoms());
        // Compare atomic numbers and positions
        for (int i = 0; i < structure.numberOfAtoms(); i++) {
            REQUIRE(newStructure.atomicNumbers()[i] == structure.atomicNumbers()[i]);
        }
    }
}

TEST_CASE("ChemicalStructure atom filtering", "[core][chemical_structure][filtering]") {
    ChemicalStructure structure;
    std::vector<QString> elements = {"H", "C", "O", "N"};
    std::vector<occ::Vec3> positions = {
        occ::Vec3(0.0, 0.0, 0.0),
        occ::Vec3(1.0, 0.0, 0.0),
        occ::Vec3(2.0, 0.0, 0.0),
        occ::Vec3(3.0, 0.0, 0.0)
    };
    structure.setAtoms(elements, positions);
    
    GenericAtomIndex idx0 = structure.indexToGenericIndex(0);
    GenericAtomIndex idx1 = structure.indexToGenericIndex(1);
    GenericAtomIndex idx2 = structure.indexToGenericIndex(2);
    
    SECTION("Atoms with specific flags") {
        structure.setAtomFlag(idx0, AtomFlag::Selected, true);
        structure.setAtomFlag(idx2, AtomFlag::Selected, true);
        
        AtomFlags selectedFlag = AtomFlag::Selected;
        
        auto selectedAtoms = structure.atomsWithFlags(selectedFlag, true);
        REQUIRE(selectedAtoms.size() == 2);
        
        auto notSelectedAtoms = structure.atomsWithFlags(selectedFlag, false);
        REQUIRE(notSelectedAtoms.size() == 2);
    }
    
    SECTION("Atoms surrounding other atoms") {
        std::vector<GenericAtomIndex> centerAtoms = {idx1};
        float radius = 1.5f;
        
        auto surrounding = structure.atomsSurroundingAtoms(centerAtoms, radius);
        REQUIRE(surrounding.size() >= 1); // Should include at least the center atom
    }
    
    SECTION("Atoms surrounding flagged atoms") {
        structure.setAtomFlag(idx1, AtomFlag::Selected, true);
        
        AtomFlags selectedFlag = AtomFlag::Selected;
        float radius = 1.5f;
        
        auto surrounding = structure.atomsSurroundingAtomsWithFlags(selectedFlag, radius);
        REQUIRE(surrounding.size() >= 1);
    }
}

TEST_CASE("ChemicalStructure formula generation", "[core][chemical_structure]") {
    ChemicalStructure structure;
    std::vector<QString> elements = {"C", "H", "H", "H", "H", "O"};
    std::vector<occ::Vec3> positions(6, occ::Vec3(0.0, 0.0, 0.0));
    structure.setAtoms(elements, positions);
    
    SECTION("Formula for all atoms") {
        auto formula = structure.chemicalFormula(false);
        REQUIRE(formula.contains("C"));
        REQUIRE(formula.contains("H"));
        REQUIRE(formula.contains("O"));
    }
    
    SECTION("Formula for subset of atoms") {
        std::vector<GenericAtomIndex> subset = {
            structure.indexToGenericIndex(0), // C
            structure.indexToGenericIndex(1), // H
            structure.indexToGenericIndex(2)  // H
        };
        auto formula = structure.formulaSumForAtoms(subset, false);
        REQUIRE(formula.contains("C"));
        REQUIRE(formula.contains("H"));
    }
}