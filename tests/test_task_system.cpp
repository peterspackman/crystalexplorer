#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "task.h"
#include "taskmanager.h"
#include "taskbackend.h"
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

using Catch::Approx;

// Simple test task implementation
class TestTask : public Task {
    Q_OBJECT
public:
    explicit TestTask(QObject *parent = nullptr) : Task(parent) {}

    void start() override {
        auto callable = [this](std::function<void(int, QString)> progress) {
            progress(25, "Quarter done");
            progress(50, "Halfway");
            progress(75, "Almost there");
            progress(100, "Complete");
        };
        run(callable);
    }

    void stop() override {
        m_canceled = true;
    }
};

// Task that simulates work
class WorkTask : public Task {
    Q_OBJECT
public:
    explicit WorkTask(int workItems, QObject *parent = nullptr)
        : Task(parent), m_workItems(workItems) {}

    void start() override {
        auto callable = [this](std::function<void(int, QString)> progress) {
            for (int i = 0; i < m_workItems; ++i) {
                if (isCanceled()) {
                    break;
                }
                int percent = (i * 100) / m_workItems;
                progress(percent, QString("Processing item %1/%2").arg(i+1).arg(m_workItems));
            }
            if (!isCanceled()) {
                progress(100, "Work complete");
            }
        };
        run(callable);
    }

    void stop() override {
        m_canceled = true;
    }

private:
    int m_workItems;
};

// Task that simulates an error
class ErrorTask : public Task {
    Q_OBJECT
public:
    explicit ErrorTask(QString errorMsg = "Test error", QObject *parent = nullptr)
        : Task(parent), m_errorMsg(errorMsg) {}

    void start() override {
        auto callable = [this](std::function<void(int, QString)> progress) {
            progress(10, "Starting work");
            throw std::runtime_error(m_errorMsg.toStdString());
        };
        run(callable);
    }

    void stop() override {}

private:
    QString m_errorMsg;
};

// Task that sets error message instead of throwing
class SoftErrorTask : public Task {
    Q_OBJECT
public:
    explicit SoftErrorTask(QString errorMsg, QObject *parent = nullptr)
        : Task(parent), m_errorMsg(errorMsg) {}

    void start() override {
        auto callable = [this](std::function<void(int, QString)> progress) {
            progress(50, "Halfway");
            setErrorMessage(m_errorMsg);
        };
        run(callable);
    }

    void stop() override {}

private:
    QString m_errorMsg;
};

TEST_CASE("Task properties", "[task][properties]") {
    TestTask task;

    SECTION("Set and get string property") {
        task.setProperty("key1", "value1");
        REQUIRE(task.property("key1").toString() == QString("value1"));
    }

    SECTION("Set and get int property") {
        task.setProperty("key2", 42);
        REQUIRE(task.property("key2").toInt() == 42);
    }

    SECTION("Set and get double property") {
        task.setProperty("key3", 3.14);
        REQUIRE(task.property("key3").toDouble() == Approx(3.14));
    }

    SECTION("Overwrite existing property") {
        task.setProperty("key", "first");
        task.setProperty("key", "second");
        REQUIRE(task.property("key").toString() == QString("second"));
    }

    SECTION("Nonexistent property returns invalid QVariant") {
        REQUIRE_FALSE(task.property("nonexistent").isValid());
    }

    SECTION("Multiple properties") {
        task.setProperty("name", "TestTask");
        task.setProperty("id", 123);
        task.setProperty("enabled", true);

        REQUIRE(task.property("name").toString() == QString("TestTask"));
        REQUIRE(task.property("id").toInt() == 123);
        REQUIRE(task.property("enabled").toBool() == true);
    }

    SECTION("Properties map") {
        task.setProperty("a", 1);
        task.setProperty("b", 2);
        task.setProperty("c", 3);

        const auto& props = task.properties();
        REQUIRE(props.size() == 3);
        REQUIRE(props.contains("a"));
        REQUIRE(props.contains("b"));
        REQUIRE(props.contains("c"));
    }
}

TEST_CASE("Task state management", "[task][state]") {
    TaskBackend *executor = TaskBackendFactory::create();
    TestTask task;
    task.setBackend(executor);

    SECTION("Initial state") {
        REQUIRE_FALSE(task.isFinished());
        REQUIRE_FALSE(task.isRunning());
        REQUIRE_FALSE(task.isCanceled());
        REQUIRE(task.errorMessage().isEmpty());
    }

    SECTION("Error message handling") {
        REQUIRE(task.errorMessage().isEmpty());
        task.setErrorMessage("Test error");
        REQUIRE(task.errorMessage() == QString("Test error"));
    }

    delete executor;
}

TEST_CASE("TaskExecutor factory", "[task][executor][factory]") {
    TaskBackend *executor = TaskBackendFactory::create();

    SECTION("Factory creates non-null executor") {
        REQUIRE(executor != nullptr);
    }

#ifdef CX_HAS_CONCURRENT
    SECTION("Creates concurrent executor when QtConcurrent available") {
        ThreadedBackend *concurrent = dynamic_cast<ThreadedBackend*>(executor);
        REQUIRE(concurrent != nullptr);
    }
#else
    SECTION("Creates sync executor for WASM") {
        SequentialBackend *sync = dynamic_cast<SequentialBackend*>(executor);
        REQUIRE(sync != nullptr);
    }
#endif

    SECTION("Multiple executors can be created") {
        TaskBackend *executor2 = TaskBackendFactory::create();
        REQUIRE(executor2 != nullptr);
        REQUIRE(executor != executor2);
        delete executor2;
    }

    delete executor;
}

TEST_CASE("Task execution signals", "[task][execution][signals]") {
    TaskBackend *executor = TaskBackendFactory::create();

    SECTION("Simple task emits completion signal") {
        TestTask task;
        task.setBackend(executor);

        QSignalSpy completedSpy(&task, &Task::completed);
        QSignalSpy errorSpy(&task, &Task::errorOccurred);

        task.start();

        // Wait for completion (different timing for sync vs async)
        QTRY_VERIFY(completedSpy.count() > 0);
        REQUIRE(completedSpy.count() == 1);
        REQUIRE(errorSpy.count() == 0);
    }

    SECTION("Task with work items reports progress") {
        WorkTask task(10);
        task.setBackend(executor);

        QSignalSpy progressSpy(&task, &Task::progress);
        QSignalSpy progressTextSpy(&task, &Task::progressText);
        QSignalSpy completedSpy(&task, &Task::completed);

        task.start();

        QTRY_VERIFY(completedSpy.count() > 0);
        REQUIRE(completedSpy.count() == 1);

        // Should have received multiple progress updates
        REQUIRE(progressSpy.count() > 0);
        REQUIRE(progressTextSpy.count() > 0);
    }

    SECTION("Error task emits error signal") {
        ErrorTask task("Custom error message");
        task.setBackend(executor);

        QSignalSpy errorSpy(&task, &Task::errorOccurred);
        QSignalSpy completedSpy(&task, &Task::completed);

        task.start();

        QTRY_VERIFY(errorSpy.count() > 0);
        REQUIRE(errorSpy.count() == 1);
        REQUIRE(completedSpy.count() == 0);

        QList<QVariant> arguments = errorSpy.takeFirst();
        QString errorMsg = arguments.at(0).toString();
        REQUIRE(errorMsg.contains("Custom error message"));
    }

    SECTION("Soft error task completes but has error message") {
        SoftErrorTask task("Soft error");
        task.setBackend(executor);

        QSignalSpy completedSpy(&task, &Task::completed);
        QSignalSpy errorSpy(&task, &Task::errorOccurred);

        task.start();

        QTRY_VERIFY(completedSpy.count() > 0 || errorSpy.count() > 0);

        // Task might emit error or complete depending on implementation
        // But error message should be set
        REQUIRE_FALSE(task.errorMessage().isEmpty());
        REQUIRE(task.errorMessage() == QString("Soft error"));
    }

    delete executor;
}

TEST_CASE("TaskManager basic operations", "[task][manager][basic]") {
    TaskManager manager;

    SECTION("Initial state") {
        REQUIRE(manager.numTasks() == 0);
        REQUIRE(manager.numFinished() == 0);
    }

    SECTION("Add single task") {
        TestTask *task = new TestTask();
        TaskID id = manager.add(task, false);

        REQUIRE(manager.numTasks() == 1);
        REQUIRE(manager.get(id) == task);
    }

    SECTION("Add multiple tasks") {
        TestTask *task1 = new TestTask();
        TestTask *task2 = new TestTask();
        TestTask *task3 = new TestTask();

        TaskID id1 = manager.add(task1, false);
        TaskID id2 = manager.add(task2, false);
        TaskID id3 = manager.add(task3, false);

        REQUIRE(manager.numTasks() == 3);
        REQUIRE(manager.get(id1) == task1);
        REQUIRE(manager.get(id2) == task2);
        REQUIRE(manager.get(id3) == task3);
    }

    SECTION("Task IDs are unique") {
        TestTask *task1 = new TestTask();
        TestTask *task2 = new TestTask();

        TaskID id1 = manager.add(task1, false);
        TaskID id2 = manager.add(task2, false);

        REQUIRE(id1 != id2);
    }

    SECTION("Remove task") {
        TestTask *task = new TestTask();
        TaskID id = manager.add(task, false);

        REQUIRE(manager.numTasks() == 1);
        manager.remove(id);
        REQUIRE(manager.numTasks() == 0);
    }

    SECTION("Get returns nullptr for invalid ID") {
        TaskID invalidId = TaskID::createUuid();
        REQUIRE(manager.get(invalidId) == nullptr);
    }
}

TEST_CASE("TaskManager concurrency settings", "[task][manager][concurrency]") {
    TaskManager manager;

    SECTION("Default concurrency") {
        REQUIRE(manager.maximumConcurrency() == 6);
    }

    SECTION("Set maximum concurrency") {
        manager.setMaximumConcurrency(10);
        REQUIRE(manager.maximumConcurrency() == 10);

        manager.setMaximumConcurrency(1);
        REQUIRE(manager.maximumConcurrency() == 1);

        manager.setMaximumConcurrency(100);
        REQUIRE(manager.maximumConcurrency() == 100);
    }

    SECTION("Current concurrency starts at zero") {
        REQUIRE(manager.getCurrentConcurrency() == 0);
    }
}

TEST_CASE("TaskManager signals", "[task][manager][signals]") {
    TaskManager manager;

    SECTION("Task added signal") {
        QSignalSpy addedSpy(&manager, &TaskManager::taskAdded);

        TestTask *task = new TestTask();
        TaskID id = manager.add(task, false);

        REQUIRE(addedSpy.count() == 1);
        QList<QVariant> arguments = addedSpy.takeFirst();
        TaskID emittedId = arguments.at(0).value<TaskID>();
        REQUIRE(emittedId == id);
    }

    SECTION("Task removed signal") {
        QSignalSpy removedSpy(&manager, &TaskManager::taskRemoved);

        TestTask *task = new TestTask();
        TaskID id = manager.add(task, false);
        manager.remove(id);

        REQUIRE(removedSpy.count() == 1);
        QList<QVariant> arguments = removedSpy.takeFirst();
        TaskID emittedId = arguments.at(0).value<TaskID>();
        REQUIRE(emittedId == id);
    }

    SECTION("Task completion signal") {
        QSignalSpy completeSpy(&manager, &TaskManager::taskComplete);

        TestTask *task = new TestTask();
        manager.add(task, true);

        QTRY_VERIFY(completeSpy.count() > 0);
        REQUIRE(completeSpy.count() == 1);
    }

    SECTION("Task error signal") {
        QSignalSpy errorSpy(&manager, &TaskManager::taskError);

        ErrorTask *task = new ErrorTask();
        manager.add(task, true);

        QTRY_VERIFY(errorSpy.count() > 0);
        REQUIRE(errorSpy.count() == 1);

        QList<QVariant> arguments = errorSpy.takeFirst();
        QString errorMsg = arguments.at(1).toString();
        REQUIRE(errorMsg.contains("Test error"));
    }
}

TEST_CASE("TaskManager task execution", "[task][manager][execution]") {
    TaskManager manager;

    SECTION("Single task completes") {
        QSignalSpy completeSpy(&manager, &TaskManager::taskComplete);

        TestTask *task = new TestTask();
        manager.add(task, true);

        QTRY_COMPARE(completeSpy.count(), 1);
        REQUIRE(manager.numFinished() == 1);
    }

    SECTION("Multiple tasks complete") {
        QSignalSpy completeSpy(&manager, &TaskManager::taskComplete);

        TestTask *task1 = new TestTask();
        TestTask *task2 = new TestTask();
        TestTask *task3 = new TestTask();

        manager.add(task1, true);
        manager.add(task2, true);
        manager.add(task3, true);

        QTRY_COMPARE(completeSpy.count(), 3);
        REQUIRE(manager.numFinished() == 3);
        REQUIRE(manager.numTasks() == 3);
    }

    SECTION("Mix of successful and error tasks") {
        QSignalSpy completeSpy(&manager, &TaskManager::taskComplete);
        QSignalSpy errorSpy(&manager, &TaskManager::taskError);

        TestTask *task1 = new TestTask();
        ErrorTask *task2 = new ErrorTask();
        TestTask *task3 = new TestTask();

        manager.add(task1, true);
        manager.add(task2, true);
        manager.add(task3, true);

        QTRY_VERIFY(completeSpy.count() + errorSpy.count() >= 3);

        // Should have 2 completions and 1 error
        REQUIRE(completeSpy.count() == 2);
        REQUIRE(errorSpy.count() == 1);
    }
}

#include "test_task_system.moc"
