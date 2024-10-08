#include "fragment.h"
#include "fragment_index.h"
#include "generic_atom_index.h"
#include <QtTest>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class TestCore : public QObject {
  Q_OBJECT
public:
  TestCore() {}
private slots:
  // FragmentIndex
  void testFragmentIndexEquality();
  void testFragmentIndexLessThan();
  void testFragmentIndexGreaterThan();
  void testFragmentIndexToJson();
  void testFragmentIndexFromJson();
  void testFragmentIndexRoundTrip();
  void testFragmentIndexEdgeCases();

  // GenericAtomIndex
  void testGenericAtomIndexEquality();
  void testGenericAtomIndexLessThan();
  void testGenericAtomIndexGreaterThan();
  void testGenericAtomIndexToJson();
  void testGenericAtomIndexFromJson();
  void testGenericAtomIndexRoundTrip();
  void testGenericAtomIndexEdgeCases();

  // New Fragment serialization tests
  void testFragmentBasicSerialization();
  void testFragmentEmptySerialization();
  void testFragmentComplexSerialization();
  void testFragmentRoundTrip();
  void testFragmentPartialDeserialization();

  // utility methods
  void testVectorSerialization();
};

void TestCore::testFragmentIndexEquality() {
  FragmentIndex a{1, 2, 3, 4};
  FragmentIndex b{1, 2, 3, 4};
  FragmentIndex c{2, 2, 3, 4};

  QVERIFY(a == b);
  QVERIFY(!(a == c));
}

void TestCore::testFragmentIndexLessThan() {
  FragmentIndex a{1, 2, 3, 4};
  FragmentIndex b{2, 2, 3, 4};
  FragmentIndex c{1, 3, 3, 4};
  FragmentIndex d{1, 2, 4, 4};
  FragmentIndex e{1, 2, 3, 5};

  QVERIFY(a < b);
  QVERIFY(a < c);
  QVERIFY(a < d);
  QVERIFY(a < e);
  QVERIFY(!(b < a));
}

void TestCore::testFragmentIndexGreaterThan() {
  FragmentIndex a{2, 2, 3, 4};
  FragmentIndex b{1, 2, 3, 4};
  FragmentIndex c{2, 1, 3, 4};
  FragmentIndex d{2, 2, 2, 4};
  FragmentIndex e{2, 2, 3, 3};

  QVERIFY(a > b);
  QVERIFY(a > c);
  QVERIFY(a > d);
  QVERIFY(a > e);
  QVERIFY(!(b > a));
}

void TestCore::testFragmentIndexToJson() {
  FragmentIndex index{1, 2, 3, 4};
  json j = index;

  QCOMPARE(j["u"].get<int>(), 1);
  QCOMPARE(j["h"].get<int>(), 2);
  QCOMPARE(j["k"].get<int>(), 3);
  QCOMPARE(j["l"].get<int>(), 4);
}

void TestCore::testFragmentIndexFromJson() {
  json j = {{"u", 5}, {"h", 6}, {"k", 7}, {"l", 8}};

  FragmentIndex index = j.get<FragmentIndex>();

  QCOMPARE(index.u, 5);
  QCOMPARE(index.h, 6);
  QCOMPARE(index.k, 7);
  QCOMPARE(index.l, 8);
}

void TestCore::testFragmentIndexRoundTrip() {
  FragmentIndex original{13, 14, 15, 16};
  json j = original;
  FragmentIndex roundTrip = j.get<FragmentIndex>();

  QCOMPARE(original.u, roundTrip.u);
  QCOMPARE(original.h, roundTrip.h);
  QCOMPARE(original.k, roundTrip.k);
  QCOMPARE(original.l, roundTrip.l);
}

void TestCore::testFragmentIndexEdgeCases() {
  // Test with zero values
  FragmentIndex zero{0, 0, 0, 0};
  json zeroJson = zero;
  FragmentIndex zeroRoundTrip = zeroJson.get<FragmentIndex>();
  QCOMPARE(zero.u, zeroRoundTrip.u);
  QCOMPARE(zero.h, zeroRoundTrip.h);
  QCOMPARE(zero.k, zeroRoundTrip.k);
  QCOMPARE(zero.l, zeroRoundTrip.l);

  // Test with negative values
  FragmentIndex negative{-1, -2, -3, -4};
  json negativeJson = negative;
  FragmentIndex negativeRoundTrip = negativeJson.get<FragmentIndex>();
  QCOMPARE(negative.u, negativeRoundTrip.u);
  QCOMPARE(negative.h, negativeRoundTrip.h);
  QCOMPARE(negative.k, negativeRoundTrip.k);
  QCOMPARE(negative.l, negativeRoundTrip.l);

  // Test with large values
  FragmentIndex large{INT_MAX, INT_MAX - 1, INT_MAX - 2, INT_MAX - 3};
  json largeJson = large;
  FragmentIndex largeRoundTrip = largeJson.get<FragmentIndex>();
  QCOMPARE(large.u, largeRoundTrip.u);
  QCOMPARE(large.h, largeRoundTrip.h);
  QCOMPARE(large.k, largeRoundTrip.k);
  QCOMPARE(large.l, largeRoundTrip.l);
}

void TestCore::testGenericAtomIndexEquality() {
  GenericAtomIndex a{1, 2, 3, 4};
  GenericAtomIndex b{1, 2, 3, 4};
  GenericAtomIndex c{2, 2, 3, 4};

  QVERIFY(a == b);
  QVERIFY(!(a == c));
}

void TestCore::testGenericAtomIndexLessThan() {
  GenericAtomIndex a{1, 2, 3, 4};
  GenericAtomIndex b{2, 2, 3, 4};
  GenericAtomIndex c{1, 3, 3, 4};
  GenericAtomIndex d{1, 2, 4, 4};
  GenericAtomIndex e{1, 2, 3, 5};

  QVERIFY(a < b);
  QVERIFY(a < c);
  QVERIFY(a < d);
  QVERIFY(a < e);
  QVERIFY(!(b < a));
}

void TestCore::testGenericAtomIndexGreaterThan() {
  GenericAtomIndex a{2, 2, 3, 4};
  GenericAtomIndex b{1, 2, 3, 4};
  GenericAtomIndex c{2, 1, 3, 4};
  GenericAtomIndex d{2, 2, 2, 4};
  GenericAtomIndex e{2, 2, 3, 3};

  QVERIFY(a > b);
  QVERIFY(a > c);
  QVERIFY(a > d);
  QVERIFY(a > e);
  QVERIFY(!(b > a));
}

void TestCore::testGenericAtomIndexToJson() {
  GenericAtomIndex index{1, 2, 3, 4};
  json j = index;

  QCOMPARE(j["unique"].get<int>(), 1);
  QCOMPARE(j["x"].get<int>(), 2);
  QCOMPARE(j["y"].get<int>(), 3);
  QCOMPARE(j["z"].get<int>(), 4);
}

void TestCore::testGenericAtomIndexFromJson() {
  json j = {{"unique", 5}, {"x", 6}, {"y", 7}, {"z", 8}};

  GenericAtomIndex index = j.get<GenericAtomIndex>();

  QCOMPARE(index.unique, 5);
  QCOMPARE(index.x, 6);
  QCOMPARE(index.y, 7);
  QCOMPARE(index.z, 8);
}

void TestCore::testGenericAtomIndexRoundTrip() {
  GenericAtomIndex original{13, 14, 15, 16};
  json j = original;
  GenericAtomIndex roundTrip = j.get<GenericAtomIndex>();

  QCOMPARE(original.unique, roundTrip.unique);
  QCOMPARE(original.x, roundTrip.x);
  QCOMPARE(original.y, roundTrip.y);
  QCOMPARE(original.z, roundTrip.z);
}

void TestCore::testGenericAtomIndexEdgeCases() {
  // Test with zero values
  GenericAtomIndex zero{0, 0, 0, 0};
  json zeroJson = zero;
  GenericAtomIndex zeroRoundTrip = zeroJson.get<GenericAtomIndex>();
  QCOMPARE(zero.unique, zeroRoundTrip.unique);
  QCOMPARE(zero.x, zeroRoundTrip.x);
  QCOMPARE(zero.y, zeroRoundTrip.y);
  QCOMPARE(zero.z, zeroRoundTrip.z);

  // Test with negative values
  GenericAtomIndex negative{-1, -2, -3, -4};
  json negativeJson = negative;
  GenericAtomIndex negativeRoundTrip = negativeJson.get<GenericAtomIndex>();
  QCOMPARE(negative.unique, negativeRoundTrip.unique);
  QCOMPARE(negative.x, negativeRoundTrip.x);
  QCOMPARE(negative.y, negativeRoundTrip.y);
  QCOMPARE(negative.z, negativeRoundTrip.z);

  // Test with large values
  GenericAtomIndex large{INT_MAX, INT_MAX - 1, INT_MAX - 2, INT_MAX - 3};
  json largeJson = large;
  GenericAtomIndex largeRoundTrip = largeJson.get<GenericAtomIndex>();
  QCOMPARE(large.unique, largeRoundTrip.unique);
  QCOMPARE(large.x, largeRoundTrip.x);
  QCOMPARE(large.y, largeRoundTrip.y);
  QCOMPARE(large.z, largeRoundTrip.z);
}

void TestCore::testFragmentBasicSerialization() {
  Fragment f;
  f.atomIndices = {GenericAtomIndex{1, 0, 0, 0}, GenericAtomIndex{2, 0, 0, 0}};
  f.atomicNumbers = Eigen::Vector2i(1, 6);
  f.positions = Eigen::Matrix3d::Identity();
  f.index = FragmentIndex{1, 0, 0, 0};
  f.name = "Test Fragment";

  json j = f;

  QCOMPARE(j["atomIndices"].size(), 2);
  QCOMPARE(j["atomicNumbers"].size(), 2);
  QCOMPARE(j["positions"].size(), 3);
  QCOMPARE(j["index"]["u"].get<int>(), 1);
  QCOMPARE(j["name"].get<std::string>(), "Test Fragment");
}

void TestCore::testFragmentEmptySerialization() {
  Fragment f;
  json j = f;

  QCOMPARE(j["atomIndices"].size(), 0);
  QCOMPARE(j["atomicNumbers"].size(), 0);
  QCOMPARE(j["positions"].size(), 3);
  QCOMPARE(j["positions"][0].size(), 0);
  QCOMPARE(j["name"].get<std::string>(), "Fragment?");
}

void TestCore::testFragmentComplexSerialization() {
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

  QCOMPARE(j["asymmetricFragmentIndex"]["u"].get<int>(), 2);
  QCOMPARE(j["asymmetricFragmentTransform"].size(), 4);
  QCOMPARE(j["state"]["charge"].get<int>(), -1);
  QCOMPARE(j["state"]["multiplicity"].get<int>(), 2);
  QCOMPARE(j["asymmetricUnitIndices"].size(), 2);
  QCOMPARE(j["color"]["r"].get<int>(), 255);
  QCOMPARE(j["color"]["g"].get<int>(), 0);
  QCOMPARE(j["color"]["b"].get<int>(), 0);
}

void TestCore::testFragmentRoundTrip() {
  Fragment original;
  original.atomIndices = {GenericAtomIndex{1, 0, 0, 0},
                          GenericAtomIndex{2, 0, 0, 0}};
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
  QCOMPARE(roundTrip.atomIndices.size(), original.atomIndices.size());
  QCOMPARE(roundTrip.atomicNumbers, original.atomicNumbers);
  QCOMPARE(roundTrip.positions, original.positions);
  QCOMPARE(roundTrip.asymmetricFragmentIndex, original.asymmetricFragmentIndex);
  QCOMPARE(roundTrip.asymmetricFragmentTransform.matrix(),
           original.asymmetricFragmentTransform.matrix());
  QCOMPARE(roundTrip.index, original.index);
  QCOMPARE(roundTrip.state.charge, original.state.charge);
  QCOMPARE(roundTrip.state.multiplicity, original.state.multiplicity);
  QCOMPARE(roundTrip.asymmetricUnitIndices, original.asymmetricUnitIndices);
  QCOMPARE(roundTrip.color, original.color);
  QCOMPARE(roundTrip.name, original.name);
}

void TestCore::testFragmentPartialDeserialization() {
  json j = {{"index", FragmentIndex{-1, 0, 0, 0}},
            {"atomIndices", {GenericAtomIndex{1, 0, 0, 0}, GenericAtomIndex{2, 0, 0, 0}}},
            {"atomicNumbers", {1, 6}},
            {"name", "Partial Fragment"}};

  Fragment f = j.get<Fragment>();

  QCOMPARE(f.name, QString("Partial Fragment"));
  QCOMPARE(f.atomIndices.size(), 2);
  QCOMPARE(f.atomicNumbers, Eigen::Vector2i(1, 6));
  QCOMPARE(f.positions.cols(), 0);
  QCOMPARE(f.index.u, -1);
}

void TestCore::testVectorSerialization() {
  // Test with int vector
  std::vector<int> intVector = {1, 2, 3, 4, 5};
  json intJson = intVector;
  std::vector<int> deserializedIntVector = intJson.get<std::vector<int>>();
  QCOMPARE(deserializedIntVector, intVector);

  // Test with string vector
  std::vector<std::string> stringVector = {"one", "two", "three"};
  json stringJson = stringVector;
  std::vector<std::string> deserializedStringVector =
      stringJson.get<std::vector<std::string>>();
  QCOMPARE(deserializedStringVector, stringVector);

  // Test with double vector
  std::vector<double> doubleVector = {1.1, 2.2, 3.3, 4.4};
  json doubleJson = doubleVector;
  std::vector<double> deserializedDoubleVector =
      doubleJson.get<std::vector<double>>();
  QCOMPARE(deserializedDoubleVector, doubleVector);

  // Test with GenericAtomIndex vector
  std::vector<GenericAtomIndex> indexVector = {
      {1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}};
  json indexJson = indexVector;
  std::vector<GenericAtomIndex> deserializedIndexVector =
      indexJson.get<std::vector<GenericAtomIndex>>();
  QCOMPARE(deserializedIndexVector.size(), indexVector.size());
  for (size_t i = 0; i < indexVector.size(); ++i) {
    QCOMPARE(deserializedIndexVector[i].unique, indexVector[i].unique);
    QCOMPARE(deserializedIndexVector[i].x, indexVector[i].x);
    QCOMPARE(deserializedIndexVector[i].y, indexVector[i].y);
    QCOMPARE(deserializedIndexVector[i].z, indexVector[i].z);
  }

  // Test empty vector
  std::vector<int> emptyVector;
  json emptyJson = emptyVector;
  std::vector<int> deserializedEmptyVector = emptyJson.get<std::vector<int>>();
  QVERIFY(deserializedEmptyVector.empty());

  // Test single GenericAtomIndex serialization and deserialization
  GenericAtomIndex singleIndex{100, 200, 300, 400};
  json singleJson = singleIndex;
  GenericAtomIndex deserializedIndex = singleJson.get<GenericAtomIndex>();
  QCOMPARE(deserializedIndex.unique, singleIndex.unique);
  QCOMPARE(deserializedIndex.x, singleIndex.x);
  QCOMPARE(deserializedIndex.y, singleIndex.y);
  QCOMPARE(deserializedIndex.z, singleIndex.z);
}

QTEST_MAIN(TestCore)
#include "test_core.moc"
