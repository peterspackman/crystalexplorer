#pragma once
#include <QAbstractListModel>
#include "mesh.h"
#include "meshinstance.h"

class MeshPropertyModel : public QAbstractListModel {
    Q_OBJECT

public:
    struct PropertyStatistics {
	double lower{0.0};
	double upper{0.0};
	double mean{0.0};
    };

    enum MeshDataRoles {
        VolumeRole = Qt::UserRole + 1,
        AreaRole,
        GlobularityRole,
        AsphericityRole,
        TransparentRole,
	FingerprintableRole
    };

    explicit MeshPropertyModel(QObject* parent = nullptr);

    void setMesh(Mesh*);
    void setMeshInstance(MeshInstance*);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    PropertyStatistics getSelectedPropertyStatistics() const;
    Mesh::ScalarPropertyRange getSelectedPropertyRange() const;
    void setSelectedPropertyRange(Mesh::ScalarPropertyRange);
    QString getSelectedProperty() const;

    bool isTransparent() const;

    double volume() const;
    double area() const;
    double globularity() const;
    double asphericity() const;

    bool isValid() const;

    bool isFingerprintable() const;

signals:
    void propertySelectionChanged(QString);

public slots:
    void setSelectedProperty(QString);
    void setTransparent(bool);

private:
    MeshInstance * m_meshInstance{nullptr};
    Mesh* m_mesh{nullptr};
    bool m_blockedWhileResetting{false};


protected:
    QHash<int, QByteArray> roleNames() const override;
};
