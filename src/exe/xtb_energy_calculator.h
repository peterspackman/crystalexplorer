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
  xtb::Result getResult(int index = 0) const;

signals:
  void calculationComplete();

private slots:
  void calculationComplete(xtb::Parameters, QString filename, QString name);

private:
  TaskManager *m_taskManager{nullptr};
  ChemicalStructure *m_structure{nullptr};
  std::vector<xtb::Result> m_results;
  QString m_xtbExecutable{"xtb"};
  QProcessEnvironment m_environment;

  bool m_complete{false};
  int completedTaskCount{0};
};
