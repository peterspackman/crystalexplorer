#pragma once
#include <QObject>
#include "taskmanager.h"
#include "chemicalstructure.h"
#include "isosurface_parameters.h"

namespace volume {

class IsosurfaceCalculator: public QObject {
    Q_OBJECT
public:
    IsosurfaceCalculator(QObject * parent = nullptr);
    
    void setTaskManager(TaskManager *);
    void start(isosurface::Parameters);

signals:
    void calculationComplete(isosurface::Result);

private slots:
    void onTaskComplete();

private:
    TaskManager * m_taskManager{nullptr};
    ChemicalStructure * m_structure{nullptr};
};

}
