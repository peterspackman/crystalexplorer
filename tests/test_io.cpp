#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <QString>
#include "genericxyzfile.h"

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