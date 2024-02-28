#pragma once
#include "task.h"
#include <QObject>
#include <QMap>
#include <QUuid>

using TaskID = QUuid;


class TaskManager : public QObject {
    Q_OBJECT
public:
    explicit TaskManager(QObject *parent = nullptr);

    TaskID add(Task* task);
    void remove(TaskID taskId);
    Task* get(TaskID taskId) const;

    void runBlocking(TaskID);

signals:
    void taskComplete(TaskID);
    void taskError(TaskID, QString);

private:

    void handleTaskComplete(TaskID);
    void handleTaskError(TaskID, QString);

    QMap<TaskID, Task*> m_tasks;
    int m_currentId{0};
};
