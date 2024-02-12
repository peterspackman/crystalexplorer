#include "tasksequence.h"
#include <QtConcurrent/QtConcurrent>

TaskSequenceExecutor::TaskSequenceExecutor() {}

void TaskSequenceExecutor::execute(TaskSequence *taskSequence) {
  connect(taskSequence, &TaskSequence::stepComplete, this,
          &TaskSequenceExecutor::taskStepComplete);
  connect(taskSequence, &TaskSequence::finished, this,
          &TaskSequenceExecutor::taskFinished);
  m_taskSequence = taskSequence;
  auto runTasks = [&]() {
    while (!m_taskSequence->isFinished()) {
      m_taskSequence->nextStep();
    }
  };
  QThreadPool *threadPool = QThreadPool::globalInstance();
  threadPool->start(runTasks);
}

void TaskSequenceExecutor::taskStepComplete() {
  emit stepComplete(m_taskSequence->currentStep(), m_taskSequence->numSteps());
}

void TaskSequenceExecutor::taskFinished() {
  disconnect(m_taskSequence, &TaskSequence::stepComplete, this,
             &TaskSequenceExecutor::taskStepComplete);
  disconnect(m_taskSequence, &TaskSequence::finished, this,
             &TaskSequenceExecutor::taskFinished);
  emit allStepsComplete(m_taskSequence->currentStep(),
                        m_taskSequence->numSteps());
  m_taskSequence = nullptr;
}
