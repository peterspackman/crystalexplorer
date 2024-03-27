#pragma once
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <Eigen/Dense>
#include "isosurface_parameters.h"
#include <ankerl/unordered_dense.h>



class Mesh : public QObject {
    Q_OBJECT

public:
    using VertexList = Eigen::Matrix<double, 3, Eigen::Dynamic>;
    using FaceList = Eigen::Matrix<int, 3, Eigen::Dynamic>;
    using ScalarPropertyValues = Eigen::Matrix<float, Eigen::Dynamic, 1>;
    using ScalarProperties = ankerl::unordered_dense::map<QString, ScalarPropertyValues>;

    enum class NormalSetting{
	Flat,
	Average
    };

    Mesh(QObject *parent = nullptr);

    Mesh(Eigen::Ref<const VertexList> vertices, Eigen::Ref<const FaceList> faces,
	    QObject *parent = nullptr);

    Mesh(Eigen::Ref<const VertexList> vertices,
	    QObject *parent = nullptr);

    [[nodiscard]] QString description() const;
    void setDescription(const QString &);

    [[nodiscard]] const auto &vertices() const { return m_vertices; }
    [[nodiscard]] inline auto numberOfVertices() const { return m_vertices.cols(); }

    [[nodiscard]] const auto &faces() const { return m_faces; }
    [[nodiscard]] inline auto numberOfFaces() const { return m_faces.cols(); }

    //normals
    [[nodiscard]] inline bool haveVertexNormals() const {
	return m_vertexNormals.cols() == m_vertices.cols();
    }

    void setVertexNormals(Eigen::Ref<const VertexList>);
    [[nodiscard]] inline const auto &vertexNormals() const {
	return m_vertexNormals; 
    }

    [[nodiscard]] VertexList computeFaceNormals() const;
    [[nodiscard]] VertexList computeVertexNormals(NormalSetting s = NormalSetting::Flat) const;

    // vertex properties
    [[nodiscard]] QStringList availableVertexProperties() const;
    [[nodiscard]] const ScalarProperties &vertexProperties() const;
    [[nodiscard]] bool haveVertexProperty(const QString &) const;
    void setVertexProperty(const QString &name, const ScalarPropertyValues &values);
    [[nodiscard]] const ScalarPropertyValues &vertexProperty(const QString &) const;
    [[nodiscard]] inline int currentVertexPropertyIndex() const { return m_currentPropertyIndex; }
    inline void setCurrentVertexPropertyIndex(int idx) { m_currentPropertyIndex = idx; }

    //face properties
    [[nodiscard]] QStringList availableFaceProperties() const;
    [[nodiscard]] const ScalarProperties &faceProperties() const;
    [[nodiscard]] bool haveFaceProperty(const QString &) const;
    void setFaceProperty(const QString &name, const ScalarPropertyValues &values);
    [[nodiscard]] const ScalarPropertyValues &faceProperty(const QString &) const;

    [[nodiscard]] inline const auto &kind() const { return m_kind; }
    inline void setKind(isosurface::Kind kind) { m_kind = kind; }
    [[nodiscard]] inline bool isTransparent() const { return m_transparent; }

    static Mesh *newFromJson(const QJsonObject &, QObject *parent=nullptr);
    static Mesh *newFromJsonFile(const QString &, QObject *parent=nullptr);

signals:
    void propertyChanged();

private:
    void updateVertexFaceMapping();

    QString m_description{"Mesh"};
    VertexList m_vertices;
    FaceList m_faces;
    VertexList m_vertexNormals;

    std::vector<std::vector<int>> m_facesUsingVertex;

    ScalarProperties m_vertexProperties;
    ScalarProperties m_faceProperties;

    bool m_transparent{false};
    int m_currentPropertyIndex{-1};

    QString m_selectedProperty;
    ScalarPropertyValues m_emptyProperty;
    isosurface::Kind m_kind{isosurface::Kind::Promolecule};
};

class MeshInstance : public QObject {
public:
    enum class DisplayStyle {
	NoDisplay,
	Mesh,
	PointCloud,
    };

    MeshInstance(Mesh *parent,
	    const Eigen::Affine3d &transform = Eigen::Affine3d::Identity());

    inline void setDisplayStyle(DisplayStyle style) { m_displayStyle = style; }

    [[nodiscard]] inline const DisplayStyle &displayStyle() const { return m_displayStyle; }

    [[nodiscard]] inline const Mesh *mesh() const { return m_mesh; }
    inline Mesh *mesh() { return m_mesh; }

    [[nodiscard]] inline Mesh::VertexList vertices() const {
	if (!m_mesh)
	    return {};
	return m_transform * m_mesh->vertices();
    }

    [[nodiscard]] inline const auto &transform() const {
	return m_transform; 
    }
    inline void setTransform(const Eigen::Affine3d &transform) {
	m_transform = transform;
    }

private:
    Mesh *m_mesh{nullptr};
    DisplayStyle m_displayStyle{DisplayStyle::NoDisplay};
    Eigen::Affine3d m_transform;
};
