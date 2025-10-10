#include "taskmanager.h"
#include "taskbackend.h"

TaskManager::TaskManager(QObject *parent) : QObject(parent) {
    // Create shared backend for all tasks
    // Backend is stateless so safe to share
    m_backend = TaskBackendFactory::create();
}

TaskManager::~TaskManager() {
    delete m_backend;
}

TaskID TaskManager::add(Task *task, bool start) {
  auto id = TaskID::createUuid();
  m_tasks.insert(id, task);
  task->setParent(this);

  // Set backend for this task (shared backend is safe since it's stateless)
  task->setBackend(m_backend);

  connect(task, &Task::completed,
          [this, id]() { this->handleTaskComplete(id); });

  connect(task, &Task::errorOccurred,
          [this, id](QString error) { this->handleTaskError(id, error); });

  connect(task, &Task::stopped,
          [this, id]() { this->handleTaskError(id, "Stopped by user"); });

  m_taskCount++;
  emit taskAdded(id);

  if (start) {
    if (getCurrentConcurrency() + getTaskThreadCount(task) <=
        m_maxConcurrentTasks) {
      bool nowBusy = m_currentConcurrentTasks == 0;
      m_currentConcurrentTasks += getTaskThreadCount(task);
      task->start();
      if (nowBusy) {
        emit busyStateChanged(true);
      }
    } else {
      m_pendingTasks.enqueue(id);
    }
  }

  return id;
}

void TaskManager::remove(TaskID taskId) {
  if (m_tasks.contains(taskId)) {
    emit taskRemoved(taskId);
    Task *task = m_tasks.value(taskId);
    m_tasks.remove(taskId);
    if (task->isRunning()) {
      m_currentConcurrentTasks -= getTaskThreadCount(task);
      if (m_currentConcurrentTasks == 0) {
        emit busyStateChanged(false);
      }
    }
    task->deleteLater();
    if (task->isFinished()) {
      m_completeCount--;
    }
    m_taskCount--;
    m_pendingTasks.removeAll(taskId);
    startNextTask();
  }
}

Task *TaskManager::get(TaskID taskId) const {
  return m_tasks.value(taskId, nullptr);
}

void TaskManager::handleTaskComplete(TaskID id) {
  Task *task = get(id);
  if (task) {
    m_currentConcurrentTasks -= getTaskThreadCount(task);
    if (m_currentConcurrentTasks == 0) {
      emit busyStateChanged(false);
    }
  }
  m_completeCount++;
  emit taskComplete(id);
  startNextTask();
}

void TaskManager::handleTaskError(TaskID id, QString err) {
  Task *task = get(id);
  if (task) {
    m_currentConcurrentTasks -= getTaskThreadCount(task);
    if (m_currentConcurrentTasks == 0) {
      emit busyStateChanged(false);
    }
  }
  m_completeCount++;
  emit taskError(id, err);
  startNextTask();
}

int TaskManager::numFinished() const { return m_completeCount; }

int TaskManager::numTasks() const { return m_taskCount; }

void TaskManager::setMaximumConcurrency(int max) {
  m_maxConcurrentTasks = max;
  startNextTask();
}

int TaskManager::getCurrentConcurrency() const {
  return m_currentConcurrentTasks;
}

void TaskManager::startNextTask() {
  while (!m_pendingTasks.isEmpty()) {
    TaskID nextTaskId = m_pendingTasks.head();
    Task *nextTask = get(nextTaskId);
    if (nextTask && getCurrentConcurrency() + getTaskThreadCount(nextTask) <=
                        m_maxConcurrentTasks) {
      m_pendingTasks.dequeue();
      bool nowBusy = m_currentConcurrentTasks == 0;
      m_currentConcurrentTasks += getTaskThreadCount(nextTask);
      if (nowBusy) {
        emit busyStateChanged(true);
      }
      nextTask->start();
    } else {
      break;
    }
  }
}

int TaskManager::getTaskThreadCount(Task *task) const {
  if (!task)
    return 1;
  return task->properties().value("threads", 1).toInt();
}
