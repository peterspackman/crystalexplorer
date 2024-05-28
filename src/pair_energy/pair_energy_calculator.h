#pragma once
#include <QObject>
#include "taskmanager.h"
#include "chemicalstructure.h"
#include "pair_energy_parameters.h"

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

private:
    std::vector<GenericAtomIndex> m_atomsA;
    std::vector<GenericAtomIndex> m_atomsB;
    TaskManager * m_taskManager{nullptr};
    ChemicalStructure * m_structure{nullptr};
    MolecularWavefunction * m_wavefunctionA{nullptr};
    MolecularWavefunction * m_wavefunctionB{nullptr};
    int completedTaskCount{0};
    bool m_complete{false};
};
