#pragma once
#include <QObject>
#include <QMap>
#include <QVariant>
#include <QDateTime>
#include <functional>
#include "taskbackend.h"

/**
 * @brief Task provenance - tracks execution history and metadata
 */
struct TaskProvenance {
    QDateTime createdAt;
    QDateTime startedAt;
    QDateTime completedAt;
    QString backendType;      // "sequential" or "threaded"
    QString executionHost;    // For future distributed execution
    int retryCount{0};
    QMap<QString, QVariant> metadata;
};

class Task : public QObject {
    Q_OBJECT
public:
    explicit Task(QObject * parent) : QObject(parent) {
        m_provenance.createdAt = QDateTime::currentDateTime();
    }
    virtual ~Task() = default;

    virtual void start() = 0;
    virtual void stop() = 0; // For cancellation

    void setProperty(const QString &, const QVariant &);
    QVariant property(const QString &) const;
    inline const auto &properties() const { return m_properties; }
    inline const auto &errorMessage() const { return m_errorMessage; }
    inline void setErrorMessage(const QString &msg) { m_errorMessage = msg; }

    inline bool isFinished() const { return m_finished; }
    inline bool isRunning() const { return m_running; }
    inline bool isCanceled() const { return m_canceled; }

    inline const TaskProvenance& provenance() const { return m_provenance; }

    /**
     * @brief Get wall time (execution duration) in milliseconds
     * @return Duration in milliseconds, or -1 if task hasn't started/completed
     */
    qint64 wallTimeMs() const {
        if (!m_provenance.startedAt.isValid() || !m_provenance.completedAt.isValid()) {
            return -1;
        }
        return m_provenance.startedAt.msecsTo(m_provenance.completedAt);
    }

    /**
     * @brief Get wall time (execution duration) in seconds
     * @return Duration in seconds, or -1 if task hasn't started/completed
     */
    double wallTimeSec() const {
        qint64 ms = wallTimeMs();
        return ms >= 0 ? ms / 1000.0 : -1.0;
    }

    // Called by TaskManager to set backend
    void setBackend(TaskBackend *backend);

protected:
    /**
     * @brief Run a task using the provided backend
     * @param taskCallable Callable taking progress callback function(int percentage, QString text)
     */
    template<typename Callable>
    void run(Callable taskCallable) {
        if (!m_backend) {
            m_errorMessage = "No backend set for task";
            emit errorOccurred(m_errorMessage);
            return;
        }

        m_running = true;
        m_provenance.startedAt = QDateTime::currentDateTime();

        // Progress callback - posts to main thread if needed
        auto onProgress = [this](int percent, QString message) {
            QMetaObject::invokeMethod(this, [this, percent, message]() {
                emit progress(percent);
                emit progressText(message);
            }, Qt::AutoConnection);
        };

        // Completion callback - posts to main thread if needed
        auto onComplete = [this]() {
            QMetaObject::invokeMethod(this, [this]() {
                m_provenance.completedAt = QDateTime::currentDateTime();
                m_finished = true;
                m_running = false;

                // Set wallTime property
                double wallTime = wallTimeSec();
                setProperty("wallTime", wallTime);

                if (m_errorMessage.isEmpty()) {
                    qDebug() << "[TASK COMPLETE]" << property("name").toString()
                             << "Wall time:" << wallTime << "seconds";
                    emit completed();
                } else {
                    emit errorOccurred(m_errorMessage);
                }
            }, Qt::AutoConnection);
        };

        // Work callable - wraps user callable with error handling
        auto work = [taskCallable, this](std::function<void(int, QString)> progressCallback) {
            try {
                taskCallable(progressCallback);
            } catch (const std::exception &e) {
                m_errorMessage = QString::fromStdString(e.what());
            } catch (...) {
                m_errorMessage = "Unknown error during task execution";
            }
        };

        // Backend executes work and calls completion when done
        m_backend->execute(work, onProgress, onComplete);
    }

protected:
    TaskBackend *m_backend{nullptr};
    QMap<QString, QVariant> m_properties;
    TaskProvenance m_provenance;
    QString m_errorMessage;
    bool m_finished{false};
    bool m_running{false};
    bool m_canceled{false};

private:
    friend class TaskManager;

signals:
    void progress(int percentage);
    void progressText(QString desc);
    void completed();
    void canceled();
    void errorOccurred(QString);
    void stopped();
};
