#pragma once
#include <QObject>
#include "taskmanager.h"
#include "chemicalstructure.h"

namespace volume {

struct IsosurfaceParameters {
    float isovalue{0.002};
    float resolution{0.2};
    ChemicalStructure *structure{nullptr};
};

struct IsosurfaceResult {
    bool success{false};
};

class IsosurfaceCalculator: public QObject {
    Q_OBJECT
public:
    IsosurfaceCalculator(QObject * parent = nullptr);
    
    void setTaskManager(TaskManager *);

    void start(IsosurfaceParameters);

signals:
    void calculationComplete(IsosurfaceResult);

private slots:
    void onTaskComplete();

private:
    TaskManager * m_taskManager{nullptr};
    ChemicalStructure * m_structure{nullptr};
};

}
