#include "occ_external.h"
#include "taskmanager.h"
#include <QDebug>


int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    TaskManager * manager = new TaskManager();

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

    exe::wfn::Parameters params;
    params.atoms = atoms;
    task->setWavefunctionParameters(params);

    auto id = manager->add(task);

    manager->runBlocking(id);

    return 0;

}

/*
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QDebug>

void asyncTask(QPromise<void>& promise) {
    for (int i = 0; i <= 5; ++i) {
        QThread::sleep(1); // Simulate long-running operation
        qDebug() << "Progress:" << i * 20 << "%";
        promise.setProgressValue(i * 20);
        if (promise.isCanceled()) {
            qDebug() << "Task was canceled";
            return;
        }
    }
    promise.finish(); // Mark the task as completed
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QFutureWatcher<void> watcher;
    QObject::connect(&watcher, &QFutureWatcher<void>::progressValueChanged, [](int progress) {
        qDebug() << "Watcher reports progress:" << progress << "%";
    });
    QObject::connect(&watcher, &QFutureWatcher<void>::finished, []() {
        qDebug() << "Watcher reports: Task finished";
        QCoreApplication::quit(); // Exit the event loop once the task is done
    });


    // Run the task asynchronously
    auto future = QtConcurrent::run([](QPromise<void> &promise) {
        asyncTask(promise);
    });
    // Start watching the future
    watcher.setFuture(future);

    return app.exec(); // Start the event loop
}
*/
