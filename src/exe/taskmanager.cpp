#include "taskmanager.h"

TaskManager::TaskManager(QObject *parent) : QObject(parent) {}

TaskID TaskManager::add(Task* task, bool start) {
    auto id = TaskID::createUuid();
    m_tasks.insert(id, task);
    task->setParent(this);

    connect(task, &Task::completed, [this, id]() {
	this->handleTaskComplete(id);
    });

    connect(task, &Task::errorOccurred, [this, id](QString error) {
	this->handleTaskError(id, error);
    });

    connect(task, &Task::stopped, [this, id]() {
	this->handleTaskError(id, "Stopped by user");
    });

    m_taskCount++;
    emit taskAdded(id);
    if(start) task->start();
    return id;
}

void TaskManager::remove(TaskID taskId) {
    if (m_tasks.contains(taskId)) {
	emit taskRemoved(taskId); // emit before we actually remove it

	Task* task = m_tasks.value(taskId);
	m_tasks.remove(taskId);
	task->deleteLater(); // Safely delete the task

	if(task->isFinished()) {
	    m_completeCount--;
	}
	m_taskCount--;
    }
}

Task* TaskManager::get(TaskID taskId) const {
    if (m_tasks.contains(taskId)) {
	return m_tasks.value(taskId);
    }
    return nullptr;
}


void TaskManager::runBlocking(TaskID id) {
    Task * task = get(id);
    if(task == nullptr) {
	qDebug() << "No such task found: " << id;
	return;
    }
    qDebug() << "Starting task" << id;

    QEventLoop loop;
    QObject::connect(task, &Task::completed, &loop, &QEventLoop::quit);
    QObject::connect(task, &Task::progressText, [](QString text) {
	qDebug() << "@" << text;
    });
    QObject::connect(task, &Task::progress, [](int progress) {
	qDebug() << "@prog" << progress;
    });
    task->start();
    loop.exec();
}

void TaskManager::handleTaskComplete(TaskID id) {
    m_completeCount++;
    emit taskComplete(id);
}

void TaskManager::handleTaskError(TaskID id, QString err) {
    m_completeCount++;
    emit taskError(id, err);
}


int TaskManager::numFinished() const {
    return m_completeCount;
}

int TaskManager::numTasks() const {
    return m_taskCount;
}
