#include "task.h"

void Task::setProperty(const QString &key, const QVariant &value) {
    m_properties[key] = value;
}

QVariant Task::property(const QString &key) const {
    return m_properties.value(key);
}


