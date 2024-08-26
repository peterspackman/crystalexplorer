#pragma once
#include <QObject>
#include "taskmanager.h"
#include "chemicalstructure.h"
#include "pair_energy_parameters.h"
#include "xtb_energy_calculator.h"

class OccPairTask;

class PairEnergyCalculator: public QObject {
    Q_OBJECT
public:
    PairEnergyCalculator(QObject * parent = nullptr);
    
    void setTaskManager(TaskManager *);
    void start(pair_energy::Parameters);
    void start_batch(const std::vector<pair_energy::Parameters> &);

signals:
    void calculationComplete();

private slots:
    void pairEnergyComplete(pair_energy::Parameters, OccPairTask *);
    void handleXtbTaskComplete(xtb::Parameters, xtb::Result);

private:
    std::vector<GenericAtomIndex> m_atomsA;
    std::vector<GenericAtomIndex> m_atomsB;
    TaskManager * m_taskManager{nullptr};
    ChemicalStructure * m_structure{nullptr};
    MolecularWavefunction * m_wavefunctionA{nullptr};
    MolecularWavefunction * m_wavefunctionB{nullptr};
    XtbEnergyCalculator *m_xtb{nullptr};
    int m_completedTaskCount{0};
    int m_totalTasks{0};
    bool m_complete{false};
    QString m_occExecutable{"occ"};
    QMap<QString, pair_energy::Parameters> m_parameters;
    QProcessEnvironment m_environment;
};
