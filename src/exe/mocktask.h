#pragma once
#include "task.h"
#include <QTimer>
#include <QRandomGenerator>

class MockTask : public Task {
    Q_OBJECT

public:
    explicit MockTask(QObject *parent = nullptr) : Task(parent), m_timer(new QTimer(this)) {
        connect(m_timer, &QTimer::timeout, this, &MockTask::simulateProgress);
    }

    void start() override {
        m_timer->start(QRandomGenerator::global()->bounded(40, 100));
    }

    void stop() override {
        m_timer->stop();
        emit stopped();
    }

private:
    QTimer *m_timer;
    int m_progress = 0;

    void simulateProgress() {
        m_progress += QRandomGenerator::global()->bounded(1, 4);
	if(m_progress == 69) {
	    m_timer->stop();
	    emit errorOccurred("Test error");
	}
        if (m_progress >= 100) {
            m_progress = 100;
            m_timer->stop();
            emit progress(m_progress);
            emit completed();
        } else {
            emit progress(m_progress);
            emit progressText(QString("Progress: %1%").arg(m_progress));
        }
    }
};


