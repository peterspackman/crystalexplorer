#pragma once
#include "chemicalstructure.h"
#include "taskmanager.h"
#include "xtb_parameters.h"
#include "xtb.h"
#include <QObject>
#include <QProcessEnvironment>

class Task;
class XtbTask;

class XtbEnergyCalculator : public QObject {
  Q_OBJECT
public:
  XtbEnergyCalculator(QObject *parent = nullptr);

  // Creates and configures an XTB task (doesn't add to TaskManager)
  // Caller should connect to signals then add to TaskManager
  XtbTask* createTask(xtb::Parameters);

private:
  QString m_xtbExecutable{"xtb"};
  QProcessEnvironment m_environment;
  bool m_deleteWorkingFiles{false};
};
