#include "taskbackend.h"

void SequentialBackend::execute(
    WorkCallable work,
    ProgressCallback onProgress,
    CompletionCallback onComplete
) {
    m_cancelled = false;

    try {
        work(onProgress);
        if (!m_cancelled) {
            onComplete();
        }
    } catch (const std::exception &e) {
        // Error handling - completion callback should still be called
        // Task will check for error messages
        onComplete();
    } catch (...) {
        onComplete();
    }
}

void SequentialBackend::cancel() {
    m_cancelled = true;
}

#ifdef CX_HAS_CONCURRENT
void ThreadedBackend::execute(
    WorkCallable work,
    ProgressCallback onProgress,
    CompletionCallback onComplete
) {
    // Run work in thread pool
    m_future = QtConcurrent::run([work, onProgress, onComplete]() {
        try {
            // Execute the work
            work(onProgress);
        } catch (...) {
            // Errors are handled by caller - we still call completion
        }

        // Call completion callback when work finishes
        // This is called from worker thread, but callback will typically
        // use QMetaObject::invokeMethod to post to main thread
        onComplete();
    });
}

void ThreadedBackend::cancel() {
    if (m_future.isRunning()) {
        m_future.cancel();
    }
}
#endif

TaskBackend* TaskBackendFactory::create() {
#ifdef CX_HAS_CONCURRENT
    return new ThreadedBackend();
#else
    return new SequentialBackend();
#endif
}
