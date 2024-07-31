#pragma once
#include "chemicalstructure.h"
#include "taskmanager.h"
#include "xtb_parameters.h"
#include <QObject>

class XtbEnergyCalculator : public QObject {
  Q_OBJECT
public:
  XtbEnergyCalculator(QObject *parent = nullptr);

  void setTaskManager(TaskManager *);
  void start(xtb::Parameters);

signals:
  void calculationComplete(xtb::Parameters, xtb::Result);

private slots:
  void handleFinishedTask(xtb::Parameters, QString name, QString jsonFilename, QString moldenFilename);

private:
  TaskManager *m_taskManager{nullptr};
  QString m_xtbExecutable{"xtb"};
  QProcessEnvironment m_environment;

  bool m_complete{false};
  bool m_deleteWorkingFiles{false};
  int completedTaskCount{0};
};
