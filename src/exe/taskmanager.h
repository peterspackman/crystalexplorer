#pragma once
#include "task.h"
#include <QObject>
#include <QQueue>
#include <QMap>
#include <QUuid>

using TaskID = QUuid;

class TaskBackend;

class TaskManager : public QObject {
    Q_OBJECT
public:
    explicit TaskManager(QObject *parent = nullptr);
    ~TaskManager();

    TaskID add(Task* task, bool start=true);
    void remove(TaskID taskId);
    Task* get(TaskID taskId) const;

    int numFinished() const;
    int numTasks() const;

    void setMaximumConcurrency(int);
    inline int maximumConcurrency() const { return m_maxConcurrentTasks; }
    int getCurrentConcurrency() const;

signals:
    void taskComplete(TaskID);
    void taskError(TaskID, QString);
    void taskAdded(TaskID);
    void taskRemoved(TaskID);
    void busyStateChanged(bool);

private:
    int getTaskThreadCount(Task* task) const;
    void handleTaskComplete(TaskID);
    void handleTaskError(TaskID, QString);
    void startNextTask();

    TaskBackend *m_backend{nullptr};
    QMap<TaskID, Task*> m_tasks;
    QQueue<TaskID> m_pendingTasks;

    int m_currentId{0};
    int m_completeCount{0};
    int m_taskCount{0};
    int m_maxConcurrentTasks{6};
    int m_currentConcurrentTasks{0};
};
