#pragma once
#include <QObject>
#include <QtConcurrent>

class Task : public QObject {
    Q_OBJECT
public:
    explicit Task(QObject * parent) : QObject(parent) {}
    virtual void start() = 0;
    virtual void stop() = 0; // For cancellation

protected:
    template<typename Callable>
    void run(Callable &taskCallable) {
	using Watcher = QFutureWatcher<void>;
	Watcher * watcher = new Watcher();

	QObject::connect(
	    watcher, &Watcher::finished, this, &Task::completed);
	QObject::connect(
	    watcher, &Watcher::canceled, this, &Task::canceled);

	QObject::connect(
	    watcher, &Watcher::progressValueChanged, this, &Task::progress);

	QObject::connect(
	    watcher, &Watcher::finished, watcher, &Watcher::deleteLater);

	auto future = QtConcurrent::run([=](QPromise<void> &promise) {
	    taskCallable(promise);
	});

	watcher->setFuture(future);
    }

private:

signals:
    void progress(int percentage);
    void completed();
    void canceled();
    void errorOccurred(QString);
    void stopped();
};
