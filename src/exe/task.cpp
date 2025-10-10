#include "task.h"
#include "taskbackend.h"
#include <QVariant>

void Task::setBackend(TaskBackend *backend) {
    m_backend = backend;

    // Record backend type in provenance
#ifdef CX_HAS_CONCURRENT
    if (dynamic_cast<ThreadedBackend*>(backend)) {
        m_provenance.backendType = "threaded";
    } else
#endif
    {
        m_provenance.backendType = "sequential";
    }
}

void Task::setProperty(const QString &key, const QVariant &value) {
    m_properties[key] = value;
}

QVariant Task::property(const QString &key) const {
    return m_properties.value(key);
}


