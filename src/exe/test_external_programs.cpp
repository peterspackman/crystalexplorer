#include "chemicalstructure.h"
#include "mesh.h"
#include <QTimer>
#include <QApplication>
#include <QTreeView>
#include <QDebug>


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Create the main model instance
    ChemicalStructure* model = new ChemicalStructure();

    // Optionally, populate the model with some test data
    QObject* mesh1 = new QObject(model); // Child of the model
    mesh1->setObjectName("Mesh1");

    QObject* wavefunction1 = new QObject(mesh1); // Child of mesh1
    wavefunction1->setObjectName("Wavefunction1");

    Mesh * mesh2 = new Mesh(model); // Another child of the model
    mesh2->setObjectName("Mesh2");

    MeshInstance * in = new MeshInstance(mesh2);
    in->setObjectName("MeshInstance2");

    // Create and set up the view
    QTreeView* treeView = new QTreeView();
    treeView->setModel(model);
    treeView->expandAll(); // Expand all nodes for visibility
    
    QTimer* timer = new QTimer(&app);
    QObject::connect(timer, &QTimer::timeout, [&]() {
        static int count = 1;
        QObject *child = new QObject(model);
    });

    timer->start(1000); // Trigger every 1000 milliseconds

    treeView->show();

    return app.exec();
}
