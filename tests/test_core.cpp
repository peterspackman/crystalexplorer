#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "fragment.h"
#include "fragment_index.h"
#include "generic_atom_index.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using Catch::Approx;

TEST_CASE("FragmentIndex equality", "[core][fragment_index]") {
    FragmentIndex a{1, 2, 3, 4};
    FragmentIndex b{1, 2, 3, 4};
    FragmentIndex c{2, 2, 3, 4};

    REQUIRE(a == b);
    REQUIRE_FALSE(a == c);
}

TEST_CASE("FragmentIndex ordering", "[core][fragment_index]") {
    FragmentIndex a{1, 2, 3, 4};
    FragmentIndex b{2, 2, 3, 4};
    FragmentIndex c{1, 3, 3, 4};
    FragmentIndex d{1, 2, 4, 4};
    FragmentIndex e{1, 2, 3, 5};

    SECTION("Less than operator") {
        REQUIRE(a < b);
        REQUIRE(a < c);
        REQUIRE(a < d);
        REQUIRE(a < e);
        REQUIRE_FALSE(b < a);
    }
    
    SECTION("Greater than operator") {
        REQUIRE(b > a);
        REQUIRE(c > a);
        REQUIRE(d > a);
        REQUIRE(e > a);
        REQUIRE_FALSE(a > b);
    }
}

TEST_CASE("FragmentIndex JSON serialization", "[core][fragment_index][json]") {
    SECTION("To JSON") {
        FragmentIndex index{1, 2, 3, 4};
        json j = index;

        REQUIRE(j["u"].get<int>() == 1);
        REQUIRE(j["h"].get<int>() == 2);
        REQUIRE(j["k"].get<int>() == 3);
        REQUIRE(j["l"].get<int>() == 4);
    }
    
    SECTION("From JSON") {
        json j = {{"u", 5}, {"h", 6}, {"k", 7}, {"l", 8}};
        FragmentIndex index = j.get<FragmentIndex>();

        REQUIRE(index.u == 5);
        REQUIRE(index.h == 6);
        REQUIRE(index.k == 7);
        REQUIRE(index.l == 8);
    }
    
    SECTION("Round trip") {
        FragmentIndex original{13, 14, 15, 16};
        json j = original;
        FragmentIndex roundTrip = j.get<FragmentIndex>();

        REQUIRE(original.u == roundTrip.u);
        REQUIRE(original.h == roundTrip.h);
        REQUIRE(original.k == roundTrip.k);
        REQUIRE(original.l == roundTrip.l);
    }
}

TEST_CASE("FragmentIndex edge cases", "[core][fragment_index][edge_cases]") {
    SECTION("Zero values") {
        FragmentIndex zero{0, 0, 0, 0};
        json zeroJson = zero;
        FragmentIndex zeroRoundTrip = zeroJson.get<FragmentIndex>();
        
        REQUIRE(zero.u == zeroRoundTrip.u);
        REQUIRE(zero.h == zeroRoundTrip.h);
        REQUIRE(zero.k == zeroRoundTrip.k);
        REQUIRE(zero.l == zeroRoundTrip.l);
    }

    SECTION("Negative values") {
        FragmentIndex negative{-1, -2, -3, -4};
        json negativeJson = negative;
        FragmentIndex negativeRoundTrip = negativeJson.get<FragmentIndex>();
        
        REQUIRE(negative.u == negativeRoundTrip.u);
        REQUIRE(negative.h == negativeRoundTrip.h);
        REQUIRE(negative.k == negativeRoundTrip.k);
        REQUIRE(negative.l == negativeRoundTrip.l);
    }

    SECTION("Large values") {
        FragmentIndex large{INT_MAX, INT_MAX - 1, INT_MAX - 2, INT_MAX - 3};
        json largeJson = large;
        FragmentIndex largeRoundTrip = largeJson.get<FragmentIndex>();
        
        REQUIRE(large.u == largeRoundTrip.u);
        REQUIRE(large.h == largeRoundTrip.h);
        REQUIRE(large.k == largeRoundTrip.k);
        REQUIRE(large.l == largeRoundTrip.l);
    }
}

TEST_CASE("GenericAtomIndex equality", "[core][generic_atom_index]") {
    GenericAtomIndex a{1, 2, 3, 4};
    GenericAtomIndex b{1, 2, 3, 4};
    GenericAtomIndex c{2, 2, 3, 4};

    REQUIRE(a == b);
    REQUIRE_FALSE(a == c);
}

TEST_CASE("GenericAtomIndex ordering", "[core][generic_atom_index]") {
    GenericAtomIndex a{1, 2, 3, 4};
    GenericAtomIndex b{2, 2, 3, 4};
    GenericAtomIndex c{1, 3, 3, 4};
    GenericAtomIndex d{1, 2, 4, 4};
    GenericAtomIndex e{1, 2, 3, 5};

    SECTION("Less than operator") {
        REQUIRE(a < b);
        REQUIRE(a < c);
        REQUIRE(a < d);
        REQUIRE(a < e);
        REQUIRE_FALSE(b < a);
    }
    
    SECTION("Greater than operator") {
        REQUIRE(b > a);
        REQUIRE(c > a);
        REQUIRE(d > a);
        REQUIRE(e > a);
        REQUIRE_FALSE(a > b);
    }
}

TEST_CASE("GenericAtomIndex JSON serialization", "[core][generic_atom_index][json]") {
    SECTION("To JSON") {
        GenericAtomIndex index{1, 2, 3, 4};
        json j = index;

        REQUIRE(j["unique"].get<int>() == 1);
        REQUIRE(j["x"].get<int>() == 2);
        REQUIRE(j["y"].get<int>() == 3);
        REQUIRE(j["z"].get<int>() == 4);
    }
    
    SECTION("From JSON") {
        json j = {{"unique", 5}, {"x", 6}, {"y", 7}, {"z", 8}};
        GenericAtomIndex index = j.get<GenericAtomIndex>();

        REQUIRE(index.unique == 5);
        REQUIRE(index.x == 6);
        REQUIRE(index.y == 7);
        REQUIRE(index.z == 8);
    }
    
    SECTION("Round trip") {
        GenericAtomIndex original{13, 14, 15, 16};
        json j = original;
        GenericAtomIndex roundTrip = j.get<GenericAtomIndex>();

        REQUIRE(original.unique == roundTrip.unique);
        REQUIRE(original.x == roundTrip.x);
        REQUIRE(original.y == roundTrip.y);
        REQUIRE(original.z == roundTrip.z);
    }
}

TEST_CASE("GenericAtomIndex edge cases", "[core][generic_atom_index][edge_cases]") {
    SECTION("Zero values") {
        GenericAtomIndex zero{0, 0, 0, 0};
        json zeroJson = zero;
        GenericAtomIndex zeroRoundTrip = zeroJson.get<GenericAtomIndex>();
        
        REQUIRE(zero.unique == zeroRoundTrip.unique);
        REQUIRE(zero.x == zeroRoundTrip.x);
        REQUIRE(zero.y == zeroRoundTrip.y);
        REQUIRE(zero.z == zeroRoundTrip.z);
    }

    SECTION("Negative values") {
        GenericAtomIndex negative{-1, -2, -3, -4};
        json negativeJson = negative;
        GenericAtomIndex negativeRoundTrip = negativeJson.get<GenericAtomIndex>();
        
        REQUIRE(negative.unique == negativeRoundTrip.unique);
        REQUIRE(negative.x == negativeRoundTrip.x);
        REQUIRE(negative.y == negativeRoundTrip.y);
        REQUIRE(negative.z == negativeRoundTrip.z);
    }

    SECTION("Large values") {
        GenericAtomIndex large{INT_MAX, INT_MAX - 1, INT_MAX - 2, INT_MAX - 3};
        json largeJson = large;
        GenericAtomIndex largeRoundTrip = largeJson.get<GenericAtomIndex>();
        
        REQUIRE(large.unique == largeRoundTrip.unique);
        REQUIRE(large.x == largeRoundTrip.x);
        REQUIRE(large.y == largeRoundTrip.y);
        REQUIRE(large.z == largeRoundTrip.z);
    }
}

TEST_CASE("Fragment basic serialization", "[core][fragment][json]") {
    Fragment f;
    f.atomIndices = {GenericAtomIndex{1, 0, 0, 0}, GenericAtomIndex{2, 0, 0, 0}};
    f.atomicNumbers = Eigen::Vector2i(1, 6);
    f.positions = Eigen::Matrix3d::Identity();
    f.index = FragmentIndex{1, 0, 0, 0};
    f.name = "Test Fragment";

    json j = f;

    REQUIRE(j["atomIndices"].size() == 2);
    REQUIRE(j["atomicNumbers"].size() == 2);
    REQUIRE(j["positions"].size() == 3);
    REQUIRE(j["index"]["u"].get<int>() == 1);
    REQUIRE(j["name"].get<std::string>() == "Test Fragment");
}

TEST_CASE("Fragment empty serialization", "[core][fragment][json]") {
    Fragment f;
    json j = f;

    REQUIRE(j["atomIndices"].size() == 0);
    REQUIRE(j["atomicNumbers"].size() == 0);
    REQUIRE(j["positions"].size() == 3);
    REQUIRE(j["positions"][0].size() == 0);
    REQUIRE(j["name"].get<std::string>() == "Fragment?");
}

TEST_CASE("Fragment complex serialization", "[core][fragment][json]") {
    Fragment f;
    f.atomIndices = {GenericAtomIndex{1, 0, 0, 0}, GenericAtomIndex{2, 0, 0, 0}};
    f.atomicNumbers = Eigen::Vector2i(1, 6);
    f.positions = Eigen::Matrix3d::Identity();
    f.asymmetricFragmentIndex = FragmentIndex{2, 0, 0, 0};
    f.asymmetricFragmentTransform = Eigen::Isometry3d::Identity();
    f.index = FragmentIndex{1, 0, 0, 0};
    f.state = Fragment::State{-1, 2};
    f.asymmetricUnitIndices = Eigen::Vector2i(0, 1);
    f.color = QColor(255, 0, 0);
    f.name = "Complex Fragment";

    json j = f;

    REQUIRE(j["asymmetricFragmentIndex"]["u"].get<int>() == 2);
    REQUIRE(j["asymmetricFragmentTransform"].size() == 4);
    REQUIRE(j["state"]["charge"].get<int>() == -1);
    REQUIRE(j["state"]["multiplicity"].get<int>() == 2);
    REQUIRE(j["asymmetricUnitIndices"].size() == 2);
    REQUIRE(j["color"]["r"].get<int>() == 255);
    REQUIRE(j["color"]["g"].get<int>() == 0);
    REQUIRE(j["color"]["b"].get<int>() == 0);
}

TEST_CASE("Fragment round trip", "[core][fragment][json]") {
    Fragment original;
    original.atomIndices = {GenericAtomIndex{1, 0, 0, 0}, GenericAtomIndex{2, 0, 0, 0}};
    original.atomicNumbers = Eigen::Vector2i(1, 6);
    original.positions = Eigen::Matrix3d::Identity();
    original.asymmetricFragmentIndex = FragmentIndex{2, 0, 0, 0};
    original.asymmetricFragmentTransform = Eigen::Isometry3d::Identity();
    original.index = FragmentIndex{1, 0, 0, 0};
    original.state = Fragment::State{-1, 2};
    original.asymmetricUnitIndices = Eigen::Vector2i(0, 1);
    original.color = QColor(255, 0, 0);
    original.name = "Round Trip Fragment";

    json j = original;
    Fragment roundTrip = j.get<Fragment>();
    
    REQUIRE(roundTrip.atomIndices.size() == original.atomIndices.size());
    REQUIRE(roundTrip.atomicNumbers == original.atomicNumbers);
    REQUIRE(roundTrip.positions == original.positions);
    REQUIRE(roundTrip.asymmetricFragmentIndex == original.asymmetricFragmentIndex);
    REQUIRE(roundTrip.asymmetricFragmentTransform.matrix() == 
            original.asymmetricFragmentTransform.matrix());
    REQUIRE(roundTrip.index == original.index);
    REQUIRE(roundTrip.state.charge == original.state.charge);
    REQUIRE(roundTrip.state.multiplicity == original.state.multiplicity);
    REQUIRE(roundTrip.asymmetricUnitIndices == original.asymmetricUnitIndices);
    REQUIRE(roundTrip.color == original.color);
    REQUIRE(roundTrip.name == original.name);
}

TEST_CASE("Fragment partial deserialization", "[core][fragment][json]") {
    json j = {{"index", FragmentIndex{-1, 0, 0, 0}},
              {"atomIndices", {GenericAtomIndex{1, 0, 0, 0}, GenericAtomIndex{2, 0, 0, 0}}},
              {"atomicNumbers", {1, 6}},
              {"name", "Partial Fragment"}};

    Fragment f = j.get<Fragment>();

    REQUIRE(f.name == QString("Partial Fragment"));
    REQUIRE(f.atomIndices.size() == 2);
    REQUIRE(f.atomicNumbers == Eigen::Vector2i(1, 6));
    REQUIRE(f.positions.cols() == 0);
    REQUIRE(f.index.u == -1);
}

TEST_CASE("Vector serialization", "[core][json]") {
    SECTION("Integer vector") {
        std::vector<int> intVector = {1, 2, 3, 4, 5};
        json intJson = intVector;
        std::vector<int> deserializedIntVector = intJson.get<std::vector<int>>();
        REQUIRE(deserializedIntVector == intVector);
    }

    SECTION("String vector") {
        std::vector<std::string> stringVector = {"one", "two", "three"};
        json stringJson = stringVector;
        std::vector<std::string> deserializedStringVector =
            stringJson.get<std::vector<std::string>>();
        REQUIRE(deserializedStringVector == stringVector);
    }

    SECTION("Double vector") {
        std::vector<double> doubleVector = {1.1, 2.2, 3.3, 4.4};
        json doubleJson = doubleVector;
        std::vector<double> deserializedDoubleVector =
            doubleJson.get<std::vector<double>>();
        
        REQUIRE(deserializedDoubleVector.size() == doubleVector.size());
        for (size_t i = 0; i < doubleVector.size(); ++i) {
            REQUIRE(deserializedDoubleVector[i] == Approx(doubleVector[i]));
        }
    }

    SECTION("GenericAtomIndex vector") {
        std::vector<GenericAtomIndex> indexVector = {
            {1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}};
        json indexJson = indexVector;
        std::vector<GenericAtomIndex> deserializedIndexVector =
            indexJson.get<std::vector<GenericAtomIndex>>();
        
        REQUIRE(deserializedIndexVector.size() == indexVector.size());
        for (size_t i = 0; i < indexVector.size(); ++i) {
            REQUIRE(deserializedIndexVector[i].unique == indexVector[i].unique);
            REQUIRE(deserializedIndexVector[i].x == indexVector[i].x);
            REQUIRE(deserializedIndexVector[i].y == indexVector[i].y);
            REQUIRE(deserializedIndexVector[i].z == indexVector[i].z);
        }
    }

    SECTION("Empty vector") {
        std::vector<int> emptyVector;
        json emptyJson = emptyVector;
        std::vector<int> deserializedEmptyVector = emptyJson.get<std::vector<int>>();
        REQUIRE(deserializedEmptyVector.empty());
    }

    SECTION("Single GenericAtomIndex") {
        GenericAtomIndex singleIndex{100, 200, 300, 400};
        json singleJson = singleIndex;
        GenericAtomIndex deserializedIndex = singleJson.get<GenericAtomIndex>();
        
        REQUIRE(deserializedIndex.unique == singleIndex.unique);
        REQUIRE(deserializedIndex.x == singleIndex.x);
        REQUIRE(deserializedIndex.y == singleIndex.y);
        REQUIRE(deserializedIndex.z == singleIndex.z);
    }
}