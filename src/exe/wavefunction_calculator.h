#pragma once
#include <QObject>
#include "taskmanager.h"
#include "chemicalstructure.h"
#include "wavefunction_parameters.h"

class WavefunctionCalculator: public QObject {
    Q_OBJECT
public:
    WavefunctionCalculator(QObject * parent = nullptr);
    
    void setTaskManager(TaskManager *);
    void start(wfn::Parameters);

signals:
    void calculationComplete(wfn::Result);

private slots:
    void wavefunctionComplete(QString);

private:
    TaskManager * m_taskManager{nullptr};
    ChemicalStructure * m_structure{nullptr};
};
