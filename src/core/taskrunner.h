#pragma once
#include <QObject>
#include <QThreadPool>
#include <QDebug>


class BackgroundWorker: public QObject {
    Q_OBJECT
public:
    explicit BackgroundWorker(QObject *parent) : QObject(parent) {}

    virtual inline void run() {
	emit finished();
    }

signals:
    void finished();
};

class BackgroundTask: public QRunnable {
public:
    BackgroundTask(BackgroundWorker* worker) : m_worker(worker) {}

    void run() override {
        QMetaObject::invokeMethod(m_worker, "run", Qt::QueuedConnection);
    }

private:
    BackgroundWorker * m_worker;
};


class OccSCFWorker: public BackgroundWorker {
    Q_OBJECT
public:
    OccSCFWorker(QObject *parent) : BackgroundWorker(parent) {}

    inline void run() override {
	qDebug() << "Writing input file";
	qDebug() << "Reading output file";
	emit finished();
    }
};

class TaskRunner : public QObject {
    Q_OBJECT
public:
    using Callback = std::function<void()>;

    explicit TaskRunner(QObject *parent = nullptr);

    inline void start(BackgroundWorker * worker) {
	auto * task = new BackgroundTask(worker);
	m_threadPool->start(task);
    }

    inline void start(BackgroundWorker * worker, Callback callback) {
	QObject::connect(worker, &BackgroundWorker::finished, callback);
	auto * task = new BackgroundTask(worker);
	m_threadPool->start(task);
    }

    ~TaskRunner() {
	qDebug() << "Waiting for all tasks";
	m_threadPool->waitForDone();
    }
signals:
    void taskFinished(BackgroundTask *);
private:
    QThreadPool * m_threadPool{nullptr};
};
