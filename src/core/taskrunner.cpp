#include "taskrunner.h"

TaskRunner::TaskRunner(QObject *parent) : QObject(parent), m_threadPool(QThreadPool::globalInstance()) {

}
