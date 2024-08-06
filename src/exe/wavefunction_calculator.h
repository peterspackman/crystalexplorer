#pragma once
#include "chemicalstructure.h"
#include "taskmanager.h"
#include "wavefunction_parameters.h"
#include "xtb_energy_calculator.h"
#include <QObject>

class WavefunctionCalculator : public QObject {
  Q_OBJECT
public:
  WavefunctionCalculator(QObject *parent = nullptr);

  void setTaskManager(TaskManager *);

  // 'normal' QM wavefunctions
  void start(wfn::Parameters);
  void start_batch(const std::vector<wfn::Parameters> &);

  // XTB
  void start(xtb::Parameters);
  void start_batch(const std::vector<xtb::Parameters> &);

  MolecularWavefunction *getWavefunction() const;

signals:
  void calculationComplete();

private slots:
  void handleTaskComplete(wfn::Parameters, QString filename, QString name);
  void handleXtbTaskComplete(xtb::Parameters, xtb::Result);

private:
  Task * makeOccTask(wfn::Parameters);
  Task * makeOrcaTask(wfn::Parameters);

  TaskManager *m_taskManager{nullptr};
  XtbEnergyCalculator *m_xtb{nullptr};
  ChemicalStructure *m_structure{nullptr};
  MolecularWavefunction *m_wavefunction{nullptr};
  QString m_occExecutable{"occ"}, m_orcaExecutable{"orca"};
  QList<QString> m_workingFiles;
  QProcessEnvironment m_environment;

  bool m_complete{false};
  bool m_deleteWorkingFiles{false};
  int m_completedTaskCount{0};
  int m_totalTasks{0};
};
