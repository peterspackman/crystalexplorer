#include <QtTest>
#include <QByteArray>
#include <QSignalSpy>

class TestCore: public QObject {
    Q_OBJECT
public:
    TestCore() {}
private slots:
    void testExample();
};

void TestCore::testExample() {
    QVERIFY(4  == 2 * 2);
}

QTEST_MAIN(TestCore)
#include "test_core.moc"

