#pragma once
#include <QAbstractListModel>
#include "mesh.h"
#include "meshinstance.h"

class MeshPropertyModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit MeshPropertyModel(QObject* parent = nullptr);

    void setMesh(Mesh*);
    void setMeshInstance(MeshInstance*);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    const Mesh::ScalarPropertyValues & getPropertyValuesAtIndex(int index) const;

    bool isTransparent() const;


public slots:
    void setSelectedProperty(int index) const;
    void setTransparent(bool) const;

private:
    MeshInstance * m_meshInstance{nullptr};
    Mesh* m_mesh{nullptr};
};