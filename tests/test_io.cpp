#include <QtTest>
#include <QByteArray>
#include "genericxyzfile.h"

class TestIO: public QObject {
    Q_OBJECT

public:
    TestIO() {}

private slots:

    void testGenericXyzFileCorrectFormat();
    void testGenericXyzFileIncorrectFormat();
    void testGenericXyzFileEmptyFile();
};

void TestIO::testGenericXyzFileCorrectFormat() {
    QString testData = 
        "2\n"
        "x y z e neighbor\n"
        "1.0 2.0 3.0 4.0 5\n"
        "1.1 2.1 3.1 4.1 6\n";

    GenericXYZFile xyzFile;
    bool success = xyzFile.readFromString(testData);
    QVERIFY(success);
    auto columnNames = xyzFile.columnNames();
    QVERIFY(columnNames[0] == "x");
    QVERIFY(columnNames[4] == "neighbor");

    const auto &neighbors = xyzFile.column("neighbor");
    QVERIFY(neighbors.rows() == 2);
    QVERIFY(neighbors(1) == 6.0f);
    // Add more checks to validate the contents of xyzFile
    // ...
}

void TestIO::testGenericXyzFileIncorrectFormat() {
    QString testData = 
        "2\n"
        "x y z e neighbor\n"
        "1.0 2.0 3.0 4.0\n"  // Missing one value
        "1.1 2.1 3.1 4.1 6\n";

    GenericXYZFile xyzFile;
    QVERIFY(!xyzFile.readFromString(testData));  // Expect failure due to incorrect format
}

void TestIO::testGenericXyzFileEmptyFile() {
    QString testData;

    GenericXYZFile xyzFile;
    QVERIFY(!xyzFile.readFromString(testData));  // Expect failure due to empty content
}

QTEST_MAIN(TestIO)
#include "test_io.moc"

