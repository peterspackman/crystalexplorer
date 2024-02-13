#include <QtTest>
#include "jobrunner.h"

class TestJobRunner : public QObject {
    Q_OBJECT

public:
    TestJobRunner() : m_runner(new JobRunner(this)) {}


private:
    JobRunner *m_runner{nullptr};

private slots:
    void testSingle();
};

void TestJobRunner::testSingle() {
    QString testString{"before"};
    auto job = makeJob([&testString]() { testString = "after"; });
    QSignalSpy spy(m_runner, &JobRunner::jobFinished);
    m_runner->enqueue(std::move(job));

    // Wait for the signal to be emitted, indicating the job is finished
    spy.wait(1000); // Wait for 1000 ms or until the signal is emitted

    QCOMPARE(testString, QString("after"));

}

QTEST_MAIN(TestJobRunner)
#include "test_jobrunner.moc"
