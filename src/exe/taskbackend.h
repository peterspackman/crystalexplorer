#pragma once
#include <QObject>
#include <functional>

/**
 * @brief Abstract interface for task execution backends
 *
 * Backends are stateless - they only know HOW to execute work,
 * not WHAT work is being done or which tasks are running.
 *
 * When work completes, the backend calls the provided completion
 * callback. This ensures each task gets its own completion notification
 * rather than all tasks sharing a broadcast signal.
 */
class TaskBackend {
public:
    using ProgressCallback = std::function<void(int, QString)>;
    using WorkCallable = std::function<void(ProgressCallback)>;
    using CompletionCallback = std::function<void()>;

    virtual ~TaskBackend() = default;

    /**
     * @brief Execute work asynchronously (or synchronously for sequential backend)
     * @param work The work to execute (receives progress callback)
     * @param onProgress Callback to report progress (may be called from worker thread)
     * @param onComplete Callback when work finishes (may be called from worker thread)
     */
    virtual void execute(
        WorkCallable work,
        ProgressCallback onProgress,
        CompletionCallback onComplete
    ) = 0;

    /**
     * @brief Request cancellation of running work
     */
    virtual void cancel() = 0;
};

/**
 * @brief Sequential backend - executes work on main thread
 *
 * Used for WASM and single-threaded environments
 */
class SequentialBackend : public TaskBackend {
public:
    void execute(
        WorkCallable work,
        ProgressCallback onProgress,
        CompletionCallback onComplete
    ) override;

    void cancel() override;

private:
    bool m_cancelled{false};
};

#ifdef CX_HAS_CONCURRENT
#include <QtConcurrent>

/**
 * @brief Threaded backend - executes work in Qt thread pool
 *
 * Used for desktop platforms with thread support
 */
class ThreadedBackend : public TaskBackend {
public:
    void execute(
        WorkCallable work,
        ProgressCallback onProgress,
        CompletionCallback onComplete
    ) override;

    void cancel() override;

private:
    QFuture<void> m_future;
};
#endif

/**
 * @brief Factory for creating appropriate backend for platform
 */
class TaskBackendFactory {
public:
    static TaskBackend* create();
};
