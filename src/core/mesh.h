#pragma once
#include "generic_atom_index.h"
#include "isosurface_parameters.h"
#include "json.h"
#include <Eigen/Dense>
#include <QMap>
#include <QObject>
#include <ankerl/unordered_dense.h>

class Mesh : public QObject {
  Q_OBJECT
  Q_PROPERTY(
      bool visible READ isVisible WRITE setVisible NOTIFY visibilityChanged)

  Q_PROPERTY(float transparency READ getTransparency WRITE setTransparency
                 NOTIFY transparencyChanged)
  Q_PROPERTY(bool transparent READ isTransparent WRITE setTransparent NOTIFY
                 transparencyChanged)
  Q_PROPERTY(QString selectedProperty READ getSelectedProperty WRITE
                 setSelectedProperty NOTIFY selectedPropertyChanged)

public:
  struct Attributes {
    float isovalue{0.0};
    isosurface::Kind kind;
    float separation{0.2};
  };

  struct ScalarPropertyRange {
    float lower{0.0};
    float upper{1.0};
    float middle{0.0}; // will only be used for some property color schemes
  };

  using VertexList = Eigen::Matrix<double, 3, Eigen::Dynamic>;
  using FaceList = Eigen::Matrix<int, 3, Eigen::Dynamic>;
  using ScalarPropertyValues = Eigen::Matrix<float, Eigen::Dynamic, 1>;
  using ScalarProperties =
      ankerl::unordered_dense::map<QString, ScalarPropertyValues>;
  using ScalarPropertyRanges =
      ankerl::unordered_dense::map<QString, ScalarPropertyRange>;

  enum class NormalSetting { Flat, Average };

  Mesh(QObject *parent = nullptr);

  Mesh(Eigen::Ref<const VertexList> vertices, Eigen::Ref<const FaceList> faces,
       QObject *parent = nullptr);

  Mesh(Eigen::Ref<const VertexList> vertices, QObject *parent = nullptr);

  [[nodiscard]] QString description() const;
  void setDescription(const QString &);

  [[nodiscard]] inline Eigen::Vector3d vertex(int index) const {
    return m_vertices.col(index);
  }
  [[nodiscard]] inline const auto &vertices() const { return m_vertices; }
  [[nodiscard]] inline auto numberOfVertices() const {
    return m_vertices.cols();
  }

  [[nodiscard]] inline Eigen::Vector3d centroid() const { return m_centroid; }

  [[nodiscard]] const auto &faces() const { return m_faces; }
  [[nodiscard]] inline auto numberOfFaces() const { return m_faces.cols(); }

  [[nodiscard]] inline const auto &vertexToFace() const {
    return m_facesUsingVertex;
  }
  // normals
  [[nodiscard]] inline bool haveVertexNormals() const {
    return m_vertexNormals.cols() == m_vertices.cols();
  }

  void setVertexNormals(Eigen::Ref<const VertexList>);
  [[nodiscard]] inline const auto &vertexNormals() const {
    return m_vertexNormals;
  }
  [[nodiscard]] inline Eigen::Vector3d vertexNormal(int index) const {
    return m_vertexNormals.col(index);
  }

  [[nodiscard]] VertexList computeFaceNormals() const;
  [[nodiscard]] VertexList
  computeVertexNormals(NormalSetting s = NormalSetting::Flat) const;

  [[nodiscard]] ScalarPropertyValues
  averagedFaceProperty(const QString &) const;

  // vertex properties
  [[nodiscard]] QStringList availableVertexProperties() const;
  [[nodiscard]] const ScalarProperties &vertexProperties() const;
  [[nodiscard]] bool haveVertexProperty(const QString &) const;
  void setVertexProperty(const QString &name,
                         const ScalarPropertyValues &values);
  [[nodiscard]] const ScalarPropertyValues &
  vertexProperty(const QString &) const;

  [[nodiscard]] ScalarPropertyRange vertexPropertyRange(const QString &) const;
  void setVertexPropertyRange(const QString &, ScalarPropertyRange);

  // face properties
  [[nodiscard]] QStringList availableFaceProperties() const;
  [[nodiscard]] const ScalarProperties &faceProperties() const;
  [[nodiscard]] bool haveFaceProperty(const QString &) const;
  void setFaceProperty(const QString &name, const ScalarPropertyValues &values);
  [[nodiscard]] const ScalarPropertyValues &faceProperty(const QString &) const;

  [[nodiscard]] inline const auto &kind() const { return m_attr.kind; }
  [[nodiscard]] inline float isovalue() const { return m_attr.isovalue; }
  [[nodiscard]] inline const auto &attributes() const { return m_attr; }

  void setAttributes(Attributes attr);

  static Mesh *newFromJson(const nlohmann::json &, QObject *parent = nullptr);
  static Mesh *newFromJsonFile(const QString &, QObject *parent = nullptr);

  [[nodiscard]] bool isVisible() const;
  void setVisible(bool visible);

  [[nodiscard]] double volume() const;
  [[nodiscard]] double surfaceArea() const;
  [[nodiscard]] double globularity() const;
  [[nodiscard]] inline double asphericity() const { return m_asphericity; }

  const QString &getSelectedProperty() const;
  bool setSelectedProperty(const QString &);

  [[nodiscard]] bool isTransparent() const;
  void setTransparent(bool);
  [[nodiscard]] float getTransparency() const;
  void setTransparency(float);
  size_t rendererIndex() const;
  void setRendererIndex(size_t idx);

  void setAtomsInside(const std::vector<GenericAtomIndex> &idxs);
  [[nodiscard]] const std::vector<GenericAtomIndex> &atomsInside() const;

  void setAtomsOutside(const std::vector<GenericAtomIndex> &idxs);
  [[nodiscard]] const std::vector<GenericAtomIndex> &atomsOutside() const;
  
  // Geometric queries
  [[nodiscard]] bool containsPoint(const occ::Vec3& point) const;
  [[nodiscard]] std::pair<occ::Vec3, occ::Vec3> boundingBox() const; // returns (min, max)
  [[nodiscard]] std::vector<GenericAtomIndex> findAtomsInside(const class ChemicalStructure* structure) const;

  bool haveChildMatchingTransform(const Eigen::Isometry3d &transform) const;

  inline const auto &faceAreas() const { return m_faceAreas; }
  inline const auto &vertexAreas() const { return m_vertexAreas; }

  void resetFaceMask(bool value = false);
  [[nodiscard]] inline const auto &faceMask() const { return m_faceMask; }
  [[nodiscard]] inline auto &faceMask() { return m_faceMask; }

  void resetVertexMask(bool value = false);
  [[nodiscard]] inline const auto &vertexMask() const { return m_vertexMask; }
  [[nodiscard]] inline auto &vertexMask() { return m_vertexMask; }

  inline void resetVertexHighlights() { m_vertexHighlights.clear(); }
  inline void highlightVertex(int v) { m_vertexHighlights.insert(v); }
  [[nodiscard]] const auto &vertexHighlights() const {
    return m_vertexHighlights;
  }

  static Mesh *combine(const QList<Mesh *> &meshes);

  nlohmann::json toJson() const;
  bool fromJson(const nlohmann::json &);

signals:
  void visibilityChanged();
  void transparencyChanged();
  void selectedPropertyChanged();

private:
  [[nodiscard]] ScalarPropertyValues computeVertexAreas() const;
  void updateVertexFaceMapping();
  void updateFaceProperties();
  void updateAsphericity();

  double m_volume{0.0}, m_surfaceArea{0.0}, m_asphericity{0.0},
      m_globularity{0.0};

  Attributes m_attr;

  bool m_visible{true};
  QString m_description{"Mesh"};
  VertexList m_vertices;
  FaceList m_faces;
  VertexList m_vertexNormals;

  VertexList m_faceNormals;
  ScalarPropertyValues m_faceAreas;
  ScalarPropertyValues m_vertexAreas;
  ScalarPropertyValues m_faceVolumeContributions;

  std::vector<std::vector<int>> m_facesUsingVertex;

  std::vector<GenericAtomIndex> m_atomsInside;
  std::vector<GenericAtomIndex> m_atomsOutside;

  Eigen::Vector3d m_centroid;
  ScalarProperties m_vertexProperties;
  ScalarPropertyRanges m_vertexPropertyRanges;
  ScalarProperties m_faceProperties;

  Eigen::Matrix<bool, Eigen::Dynamic, 1> m_faceMask;
  ankerl::unordered_dense::set<int> m_vertexHighlights;

  Eigen::Matrix<bool, Eigen::Dynamic, 1> m_vertexMask;

  bool m_transparent{false};
  float m_transparency{0.8};
  size_t m_rendererIndex{0};

  QString m_selectedProperty;
  ScalarPropertyValues m_emptyProperty;
  isosurface::Parameters m_params;
};
