#pragma once
#include <QAbstractListModel>
#include "mesh.h"

class MeshPropertyModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit MeshPropertyModel(QObject* parent = nullptr);

    void setMesh(Mesh* mesh);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    const Mesh::ScalarPropertyValues & getPropertyValuesAtIndex(int index) const;


public slots:
    void setSelectedProperty(int index) const;

private:
    Mesh* m_mesh = nullptr;
};
