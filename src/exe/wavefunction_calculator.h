#pragma once
#include "chemicalstructure.h"
#include "taskmanager.h"
#include "wavefunction_parameters.h"
#include <QObject>

class WavefunctionCalculator : public QObject {
  Q_OBJECT
public:
  WavefunctionCalculator(QObject *parent = nullptr);

  void setTaskManager(TaskManager *);
  void start(wfn::Parameters);
  void start_batch(const std::vector<wfn::Parameters> &);
  MolecularWavefunction *getWavefunction() const;

signals:
  void calculationComplete();

private slots:
  void wavefunctionComplete(wfn::Parameters, QString filename, QString name);

private:
  TaskManager *m_taskManager{nullptr};
  ChemicalStructure *m_structure{nullptr};
  MolecularWavefunction *m_wavefunction{nullptr};
  QString m_occExecutable{"occ"};
  QProcessEnvironment m_environment;

  bool m_complete{false};
  int completedTaskCount{0};
};
