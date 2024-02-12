#include "jobrunner.h"

JobRunner::JobRunner(QObject *parent) : QObject(parent) {
  m_threadPool = QThreadPool::globalInstance();
}


void JobRunner::enqueue(Job &&job) {
    {
        QMutexLocker locker(&m_queueMutex);
        m_jobQueue.push(std::move(job));
    }
    maybeStartNextJob();
}

void JobRunner::maybeStartNextJob() {
    m_queueMutex.lock();
    if((m_threadPool->activeThreadCount() < m_threadPool->maxThreadCount()) &&
        !m_jobQueue.empty()) {

        auto job = std::move(m_jobQueue.front());
        m_jobQueue.pop();
        m_queueMutex.unlock();
        m_threadPool->start([this, job = std::move(job)]() mutable {
            job.execute();
            emit jobFinished();
        });
    }
    else {
        m_queueMutex.unlock();
    }
}

void JobRunner::clearPendingJobs() {
    QMutexLocker locker(&m_queueMutex);
    JobQueue().swap(m_jobQueue);
}
