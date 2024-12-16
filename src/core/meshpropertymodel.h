#pragma once
#include "mesh.h"
#include "meshinstance.h"
#include <QAbstractListModel>
#include <QMap>

class MeshPropertyModel : public QAbstractListModel {
  Q_OBJECT
public:
  struct PropertyStatistics {
    double lower{0.0};
    double upper{0.0};
    double mean{0.0};
  };

  enum MeshDataRoles {
    PropertyNameRole = Qt::UserRole + 1,
    PropertyUnitsRole,
    PropertyDescriptionRole,
    PropertyColorMapRole,
    VolumeRole,
    AreaRole,
    GlobularityRole,
    AsphericityRole,
    TransparentRole,
    TransparencyRole,
    FingerprintableRole,
    Transparen
  };

  explicit MeshPropertyModel(QObject *parent = nullptr);
  void setMesh(Mesh *);
  void setMeshInstance(MeshInstance *);
  Mesh *getMesh();
  MeshInstance *getMeshInstance();

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;

  PropertyStatistics getSelectedPropertyStatistics() const;
  Mesh::ScalarPropertyRange getSelectedPropertyRange() const;
  void setSelectedPropertyRange(Mesh::ScalarPropertyRange);

  void setSelectedPropertyColorMap(const QString &);
  QString getSelectedPropertyColorMap() const;

  QString getSelectedProperty() const;

  bool isTransparent() const;
  float getTransparency() const;
  double volume() const;
  double area() const;
  double globularity() const;
  double asphericity() const;
  bool isValid() const;
  bool isFingerprintable() const;

signals:
  void propertySelectionChanged(QString);
  void meshSelectionChanged();

public slots:
  void setSelectedProperty(QString);
  void setTransparent(bool);
  void setTransparency(float);

protected:
  QHash<int, QByteArray> roleNames() const override;

private:
  MeshInstance *m_meshInstance{nullptr};
  Mesh *m_mesh{nullptr};
  bool m_blockedWhileResetting{false};

  QMap<QString, isosurface::SurfacePropertyDescription> m_propertyDescriptions;
  QMap<QString, isosurface::SurfaceDescription> m_surfaceDescriptions;
  QMap<QString, double> m_defaultIsovalues;
};
