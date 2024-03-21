#include "occwavefunctiontask.h"
#include "occsurfacetask.h"
#include "tonto.h"
#include "taskmanager.h"
#include "taskmanagerwidget.h"
#include "mocktask.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    TaskManager* taskManager = new TaskManager();
    TaskManagerWidget* w = new TaskManagerWidget(taskManager);

    qDebug() << "OCC";

    exe::AtomList atoms{
	{"O", "H", "H"},
	{
	    QVector3D(-0.7021961, -0.0560603, 0.0099423),
	    QVector3D(-1.0221932, 0.8467758, -0.0114887),
	    QVector3D(0.2575211, 0.0421215, 0.0052190)
	}
	
    };

    OccWavefunctionTask * task = new OccWavefunctionTask();
    task->setProperty("name", "Water wavefunction");
    task->setProperty("basename", "water");

    exe::wfn::Parameters params;
    params.atoms = atoms;
    task->setWavefunctionParameters(params);

    auto id = taskManager->add(task);

    OccSurfaceTask * surface_task = new OccSurfaceTask();
    surface_task->setProperty("name", "Water promolecule");
    surface_task->setProperty("inputFile", "water.xyz");

    auto idsurf = taskManager->add(surface_task);

    Task* mockTask = new MockTask(taskManager);
    mockTask->setProperty("name", "startup task");
    TaskID taskId = taskManager->add(mockTask);

    Task* tontoTask = new TontoCifProcessingTask(taskManager);
    tontoTask->setProperty("name", "Tonto task");
    TaskID taskId2 = taskManager->add(tontoTask);

    w->show();
    return app.exec();
}

