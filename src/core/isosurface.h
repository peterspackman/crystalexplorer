#pragma once
#include <QObject>
#include <occ/core/linear_algebra.h>

class Isosurface: public QObject {
  Q_OBJECT

public:
  using VertexList = occ::Mat3N;
  using FacesList = occ::IMat3N;
  enum class Kind {
    VolumeData, // from volume data
    Hirshfeld,    // 1D periodic
    PromoleculeDensity, // 2D periodic
    ElectronDensity // 3D periodic
  };

  Isosurface(const VertexList &vertices, const FacesList &faces, QObject *parent = nullptr);

  [[nodiscard]] Kind kind() const;
  [[nodiscard]] QString description() const;
  void setDescription(const QString &);

  [[nodiscard]] Eigen::Index numberOfFaces() const;
  [[nodiscard]] Eigen::Index numberOfVertices() const;

  [[nodiscard]] bool haveVertexNormals() const;
  [[nodiscard]] double volume() const;


private:

  void updateVolume();

  VertexList m_vertices;
  FacesList m_vertexNormals;
  occ::IMat3N m_faces;

  double m_volume{0.0};
  QString m_description{""};

  Kind m_kind{Kind::VolumeData};
};
