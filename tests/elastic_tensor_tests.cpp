#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "elastic_tensor_results.h"
#include <occ/core/linear_algebra.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using Catch::Approx;

TEST_CASE("ElasticTensorResults basic functionality", "[core][elastic_tensor]") {
    // Create a simple isotropic elastic tensor (diamond-like)
    occ::Mat6 elasticMatrix = occ::Mat6::Zero();
    
    // Diamond-like properties (GPa)
    elasticMatrix(0,0) = 1076; elasticMatrix(0,1) = 125; elasticMatrix(0,2) = 125;
    elasticMatrix(1,1) = 1076; elasticMatrix(1,2) = 125;
    elasticMatrix(2,2) = 1076;
    elasticMatrix(3,3) = 576; // (C11-C12)/2 for cubic
    elasticMatrix(4,4) = 576;
    elasticMatrix(5,5) = 576;
    
    // Make symmetric
    for(int i = 0; i < 6; i++) {
        for(int j = i+1; j < 6; j++) {
            elasticMatrix(j,i) = elasticMatrix(i,j);
        }
    }
    
    ElasticTensorResults results(elasticMatrix, "Test Diamond-like");
    
    SECTION("Basic properties") {
        REQUIRE(results.name() == "Test Diamond-like");
        REQUIRE(results.isStable() == true);
        
        // Test that average properties are reasonable
        REQUIRE(results.averageBulkModulus() > 0);
        REQUIRE(results.averageShearModulus() > 0);
        REQUIRE(results.averageYoungsModulus() > 0);
        REQUIRE(results.averagePoissonRatio() > 0);
        REQUIRE(results.averagePoissonRatio() < 0.5); // Physical limit
    }
    
    SECTION("Directional properties") {
        occ::Vec3 xDirection(1, 0, 0);
        occ::Vec3 yDirection(0, 1, 0);
        occ::Vec3 zDirection(0, 0, 1);
        
        double youngX = results.youngsModulus(xDirection);
        double youngY = results.youngsModulus(yDirection);
        double youngZ = results.youngsModulus(zDirection);
        
        REQUIRE(youngX > 0);
        REQUIRE(youngY > 0);
        REQUIRE(youngZ > 0);
        
        // For isotropic material, should be roughly equal
        REQUIRE(youngX == Approx(youngY).margin(1e-6));
        REQUIRE(youngY == Approx(youngZ).margin(1e-6));
        
        // Test shear modulus
        double shearX = results.shearModulus(xDirection);
        REQUIRE(shearX > 0);
        
        // Test linear compressibility
        double compressX = results.linearCompressibility(xDirection);
        REQUIRE(compressX > 0);
    }
}

TEST_CASE("ElasticTensorResults mesh generation", "[core][elastic_tensor][mesh]") {
    // Simple isotropic tensor
    occ::Mat6 elasticMatrix = occ::Mat6::Zero();
    elasticMatrix(0,0) = 100; elasticMatrix(0,1) = 50; elasticMatrix(0,2) = 50;
    elasticMatrix(1,1) = 100; elasticMatrix(1,2) = 50;
    elasticMatrix(2,2) = 100;
    elasticMatrix(3,3) = 25;
    elasticMatrix(4,4) = 25;
    elasticMatrix(5,5) = 25;
    
    // Make symmetric
    for(int i = 0; i < 6; i++) {
        for(int j = i+1; j < 6; j++) {
            elasticMatrix(j,i) = elasticMatrix(i,j);
        }
    }
    
    ElasticTensorResults results(elasticMatrix, "Test Mesh");
    
    SECTION("Young's modulus mesh") {
        Mesh* mesh = results.createPropertyMesh(
            ElasticTensorResults::PropertyType::YoungsModulus, 1, 1.0);
        
        REQUIRE(mesh != nullptr);
        REQUIRE(mesh->numberOfVertices() > 0);
        REQUIRE(mesh->numberOfFaces() > 0);
        
        // Check that property was added
        QStringList props = mesh->availableVertexProperties();
        REQUIRE(props.size() > 0);
        REQUIRE(props.contains("Young's Modulus (GPa)"));
        
        // Check property values are reasonable
        const auto& propValues = mesh->vertexProperty("Young's Modulus (GPa)");
        for (int i = 0; i < propValues.size(); ++i) {
            REQUIRE(propValues(i) > 0);
            REQUIRE(propValues(i) < 1000); // Reasonable upper bound
        }
        
        delete mesh;
    }
    
    SECTION("Shear modulus mesh") {
        Mesh* mesh = results.createPropertyMesh(
            ElasticTensorResults::PropertyType::ShearModulusMax, 0, 0.5);
        
        REQUIRE(mesh != nullptr);
        REQUIRE(mesh->numberOfVertices() > 0);
        
        QStringList props = mesh->availableVertexProperties();
        REQUIRE(props.contains("Shear Modulus Max (GPa)"));
        
        delete mesh;
    }
    
    SECTION("Different subdivision levels") {
        Mesh* mesh0 = results.createPropertyMesh(
            ElasticTensorResults::PropertyType::YoungsModulus, 0, 1.0);
        Mesh* mesh1 = results.createPropertyMesh(
            ElasticTensorResults::PropertyType::YoungsModulus, 1, 1.0);
        
        REQUIRE(mesh0 != nullptr);
        REQUIRE(mesh1 != nullptr);
        
        // Higher subdivision should have more vertices
        REQUIRE(mesh1->numberOfVertices() > mesh0->numberOfVertices());
        
        delete mesh0;
        delete mesh1;
    }
}

TEST_CASE("ElasticTensorResults JSON serialization", "[core][elastic_tensor][json]") {
    occ::Mat6 elasticMatrix = occ::Mat6::Zero();
    elasticMatrix(0,0) = 200; elasticMatrix(0,1) = 100;
    elasticMatrix(1,1) = 200; elasticMatrix(1,2) = 100;
    elasticMatrix(2,2) = 200;
    elasticMatrix(3,3) = 50;
    elasticMatrix(4,4) = 50;
    elasticMatrix(5,5) = 50;
    
    // Make symmetric
    for(int i = 0; i < 6; i++) {
        for(int j = i+1; j < 6; j++) {
            elasticMatrix(j,i) = elasticMatrix(i,j);
        }
    }
    
    ElasticTensorResults original(elasticMatrix, "JSON Test");
    original.setDescription("Test description");
    
    SECTION("To JSON") {
        json j = original.toJson();
        
        REQUIRE(j["name"].get<std::string>() == "JSON Test");
        REQUIRE(j["description"].get<std::string>() == "Test description");
        REQUIRE(j.contains("elasticMatrix"));
        REQUIRE(j.contains("averageProperties"));
        
        auto matrix = j["elasticMatrix"];
        REQUIRE(matrix.size() == 6);
        REQUIRE(matrix[0].size() == 6);
        REQUIRE(matrix[0][0].get<double>() == Approx(200.0));
    }
    
    SECTION("From JSON round trip") {
        json j = original.toJson();
        
        ElasticTensorResults roundTrip;
        bool success = roundTrip.fromJson(j);
        
        REQUIRE(success == true);
        REQUIRE(roundTrip.name() == original.name());
        REQUIRE(roundTrip.description() == original.description());
        
        // Test that elastic matrices are the same
        const auto& originalMatrix = original.elasticMatrix();
        const auto& roundTripMatrix = roundTrip.elasticMatrix();
        
        for (int i = 0; i < 6; ++i) {
            for (int k = 0; k < 6; ++k) {
                REQUIRE(roundTripMatrix(i,k) == Approx(originalMatrix(i,k)));
            }
        }
        
        // Test that computed properties are the same
        REQUIRE(roundTrip.averageBulkModulus() == Approx(original.averageBulkModulus()));
        REQUIRE(roundTrip.averageShearModulus() == Approx(original.averageShearModulus()));
    }
}

TEST_CASE("ElasticTensorResults edge cases", "[core][elastic_tensor][edge_cases]") {
    SECTION("Zero matrix") {
        occ::Mat6 zeroMatrix = occ::Mat6::Zero();
        ElasticTensorResults results(zeroMatrix, "Zero Test");
        
        // Should not be stable
        REQUIRE(results.isStable() == false);
    }
    
    SECTION("Invalid mesh parameters") {
        occ::Mat6 matrix = occ::Mat6::Identity() * 100;
        ElasticTensorResults results(matrix, "Invalid Test");
        
        // Test invalid subdivisions
        Mesh* mesh1 = results.createPropertyMesh(ElasticTensorResults::PropertyType::YoungsModulus, -1, 1.0);
        REQUIRE(mesh1 == nullptr);
        
        Mesh* mesh2 = results.createPropertyMesh(ElasticTensorResults::PropertyType::YoungsModulus, 10, 1.0);
        REQUIRE(mesh2 == nullptr);
        
        // Test invalid radius
        Mesh* mesh3 = results.createPropertyMesh(ElasticTensorResults::PropertyType::YoungsModulus, 2, -1.0);
        REQUIRE(mesh3 == nullptr);
        
        Mesh* mesh4 = results.createPropertyMesh(ElasticTensorResults::PropertyType::YoungsModulus, 2, 0.0);
        REQUIRE(mesh4 == nullptr);
    }
    
    SECTION("Name and description changes") {
        occ::Mat6 matrix = occ::Mat6::Identity() * 100;
        ElasticTensorResults results(matrix, "Initial");
        
        REQUIRE(results.name() == "Initial");
        
        results.setName("Changed");
        REQUIRE(results.name() == "Changed");
        
        results.setDescription("New description");
        REQUIRE(results.description() == "New description");
    }
}

TEST_CASE("ElasticTensorCollection management", "[core][elastic_tensor][collection]") {
    ElasticTensorCollection collection;
    
    SECTION("Empty collection") {
        REQUIRE(collection.count() == 0);
        REQUIRE(collection.tensors().isEmpty());
        REQUIRE(collection.findByName("nonexistent") == nullptr);
    }
    
    SECTION("Add and remove tensors") {
        occ::Mat6 matrix1 = occ::Mat6::Identity() * 100;
        occ::Mat6 matrix2 = occ::Mat6::Identity() * 200;
        
        auto* tensor1 = new ElasticTensorResults(matrix1, "Tensor1");
        auto* tensor2 = new ElasticTensorResults(matrix2, "Tensor2");
        
        collection.add(tensor1);
        collection.add(tensor2);
        
        REQUIRE(collection.count() == 2);
        REQUIRE(collection.at(0) == tensor1);
        REQUIRE(collection.at(1) == tensor2);
        REQUIRE(collection.findByName("Tensor1") == tensor1);
        REQUIRE(collection.findByName("Tensor2") == tensor2);
        
        collection.remove(tensor1);
        REQUIRE(collection.count() == 1);
        REQUIRE(collection.findByName("Tensor1") == nullptr);
        
        collection.clear();
        REQUIRE(collection.count() == 0);
    }
    
    SECTION("Collection JSON serialization") {
        occ::Mat6 matrix = occ::Mat6::Identity() * 150;
        auto* tensor = new ElasticTensorResults(matrix, "Collection Test");
        
        collection.add(tensor);
        
        json j = collection.toJson();
        REQUIRE(j.contains("tensors"));
        REQUIRE(j["tensors"].size() == 1);
        
        ElasticTensorCollection newCollection;
        bool success = newCollection.fromJson(j);
        
        REQUIRE(success == true);
        REQUIRE(newCollection.count() == 1);
        REQUIRE(newCollection.at(0)->name() == "Collection Test");
    }
}