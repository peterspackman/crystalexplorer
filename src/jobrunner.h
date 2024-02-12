#pragma once

#include <QFuture>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include <queue>


class Job {
public:
    using JobFunction = std::function<void()>;
    using Callback = std::function<void()>;

    explicit Job(JobFunction func) : m_func(std::move(func)) {}

    void execute() {
        if(m_func) {
            m_func();
        }
        if(m_callback) {
            m_callback();
        }
    }

    void setCallback(Callback callback) {
        m_callback = std::move(callback);
    }

private:
    JobFunction m_func;
    Callback m_callback;

};


class JobRunner : public QObject {
    Q_OBJECT
public:
    explicit JobRunner(QObject *parent = nullptr);

    void enqueue(Job&&);

signals:
    void jobFinished();
private:
    using JobQueue = std::queue<Job>;

    QThreadPool *m_threadPool;
    JobQueue m_jobQueue;
    QMutex m_queueMutex;

    void maybeStartNextJob();
    void clearPendingJobs();
};


template<typename Func, typename Callback = std::function<void()>>
Job makeJob(Func&& jobFunction, Callback&& callbackFunction = Callback()) {
    auto job = Job(std::forward<Func>(jobFunction));
    if (callbackFunction) {
        job.setCallback(std::forward<Callback>(callbackFunction));
    }
    return job;
}
