#pragma once
#include "mesh.h"
#include "generic_atom_index.h"
#include "fragment.h"
#include <QVector3D>
#include <QMatrix3x3>

using MeshTransform = Eigen::Isometry3d;



class MeshInstance : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibilityChanged)
    Q_PROPERTY(bool transparent READ isTransparent WRITE setTransparent NOTIFY transparencyChanged)
    Q_PROPERTY(QString selectedProperty READ getSelectedProperty WRITE setSelectedProperty NOTIFY selectedPropertyChanged)

    struct NearestPointResult {
        int idx_this{-1};
        int idx_other{-1};
        double distance{std::numeric_limits<double>::max()};
    };

public:
    explicit MeshInstance(Mesh *parent, const MeshTransform &transform = MeshTransform::Identity());

    [[nodiscard]] const Mesh *mesh() const;
    [[nodiscard]] Mesh *mesh();

    [[nodiscard]] Eigen::Vector3d vertex(int index) const;
    [[nodiscard]] QVector3D vertexVector3D(int index) const;
    [[nodiscard]] Mesh::VertexList vertices() const;

    [[nodiscard]] Eigen::Vector3d centroid() const;

    [[nodiscard]] Eigen::Vector3d vertexNormal(int index) const;
    [[nodiscard]] QVector3D vertexNormalVector3D(int index) const;
    [[nodiscard]] Mesh::VertexList vertexNormals() const;

    [[nodiscard]] const MeshTransform &transform() const;
    void setTransform(const MeshTransform &transform);

    [[nodiscard]] QMatrix3x3 rotationMatrix() const;
    [[nodiscard]] QVector3D translationVector() const;

    [[nodiscard]] NearestPointResult nearestPoint(const Fragment &other) const;
    [[nodiscard]] NearestPointResult nearestPoint(const occ::Vec3 &point) const;
    [[nodiscard]] NearestPointResult nearestPoint(const MeshInstance *other) const;


    [[nodiscard]] bool isVisible() const;
    void setVisible(bool visible);

    [[nodiscard]] bool isTransparent() const;
    void setTransparent(bool);

    [[nodiscard]] const QString &getSelectedProperty() const;
    bool setSelectedProperty(const QString &);

    float valueForSelectedPropertyAt(size_t index) const;

    static MeshInstance * newInstanceFromSelectedAtoms(Mesh *, const std::vector<GenericAtomIndex> &);

signals:
    // TODO connect these per mesh instance, for now
    void visibilityChanged();
    void transparencyChanged();
    void selectedPropertyChanged();

private:
    bool m_visible{true};
    bool m_transparent{false};
    QString m_selectedProperty;
    Mesh *m_mesh{nullptr};
    MeshTransform m_transform;
};
