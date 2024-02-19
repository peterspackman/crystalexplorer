#include <QtTest>
#include <QByteArray>
#include <QSignalSpy>
#include "interactions.h"

class TestCore: public QObject {
    Q_OBJECT
public:
    TestCore() {}
private slots:
    void testDimerInteractions();
};

void TestCore::testDimerInteractions() {
    QString label("coulomb");
    const double coulomb{1.5}, second{12.3};

    DimerPair ab(0, 1);
    DimerPair ba(1, 0);

    QVERIFY(ab == ba);

    auto * interactions = new DimerInteractions();

    QVERIFY(!interactions->haveValuesForDimer(ab));

    interactions->setValue(ab, coulomb, label);

    QVERIFY(interactions->haveValuesForDimer(ab));
    QVERIFY(interactions->valueForDimer(ab, label) == coulomb);

    {
	const auto &vals = interactions->valuesForDimer(ab);
	auto kv = vals.find(label);
	QVERIFY(kv != vals.end());
	QVERIFY(kv->second == coulomb);
    }

    interactions->setValue(ba, second, label);
    QVERIFY(interactions->valueForDimer(ba, label) == second);
    QVERIFY(interactions->valueForDimer(ab, label) == second);

    interactions->clearValue(ab, label);
    QVERIFY(!interactions->haveValuesForDimer(ab));
}

QTEST_MAIN(TestCore)
#include "test_core.moc"

