#pragma once
#include <QObject>
#include <QtConcurrent>
#include <QMap>

class Task : public QObject {
    Q_OBJECT
public:
    explicit Task(QObject * parent) : QObject(parent) {}
    virtual void start() = 0;
    virtual void stop() = 0; // For cancellation
    
    void setProperty(const QString &, const QVariant &);
    QVariant property(const QString &) const;
    inline const auto &properties() const { return m_properties; }
    inline const auto &errorMessage() const { return m_errorMessage; }
    inline void setErrorMessage(const QString &msg) { m_errorMessage = msg; }

    inline bool isFinished() const { return m_finished; }

protected:
    template<typename Callable>
    void run(Callable &taskCallable) {
	using Watcher = QFutureWatcher<void>;
	Watcher * watcher = new Watcher();

	QObject::connect(
	    watcher, &Watcher::finished, [&]() {
	    if(m_errorMessage.isEmpty()) {
		emit completed();
	    }
	    else {
		emit errorOccurred(m_errorMessage);
	    }
	    m_finished = true;
	});

	QObject::connect(
	    watcher, &Watcher::canceled, this, &Task::canceled);

	QObject::connect(
	    watcher, &Watcher::progressValueChanged, this, &Task::progress);

	QObject::connect(
	    watcher, &Watcher::progressTextChanged, this, &Task::progressText);

	QObject::connect(
	    watcher, &Watcher::finished, watcher, &Watcher::deleteLater);

	auto future = QtConcurrent::run([=](QPromise<void> &promise) {
	    taskCallable(promise);
	});

	watcher->setFuture(future);
    }

private:
    QMap<QString, QVariant> m_properties;
    QString m_errorMessage;
    bool m_finished{false};

signals:
    void progress(int percentage);
    void progressText(QString desc);
    void completed();
    void canceled();
    void errorOccurred(QString);
    void stopped();
};
