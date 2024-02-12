#pragma once
#include <QObject>

class TaskSequence : public QObject {
  Q_OBJECT
public:
  virtual bool isFinished() const = 0;
  virtual int numSteps() const = 0;
  virtual int currentStep() const = 0;
  virtual void nextStep() = 0;

  bool hasMultipleSteps() const { return numSteps() > 1; }

signals:
  void stepComplete();
  void finished();
};

class TaskSequenceExecutor : public QObject {
  Q_OBJECT
public:
  TaskSequenceExecutor();

  void execute(TaskSequence *);

signals:
  void stepComplete(int, int);
  void allStepsComplete(int, int);

private slots:
  void taskStepComplete();
  void taskFinished();

private:
  TaskSequence *m_taskSequence{nullptr};
};
