#pragma once
#include <QMap>
#include <QStringList>
#include <QVector3D>
#include <QVector>
#include <QtOpenGL>

#include "atomid.h"
#include "jobparameters.h"
#include "linerenderer.h"
#include "qeigen.h"
#include "spacegroup.h"
#include "surfaceproperty.h"
#include "unitcell.h"

const GLfloat TRANSPARENCY_ALPHA = 0.85f;
const GLfloat MASKED_COLOR[] = {0.24f, 0.24f, 0.24f, 0.8f};
const GLfloat VOID_BACK_COLOR[] = {0.24f, 0.24f, 0.24f, 0.8f};
const int NO_DOMAIN = -1;

// Parameters which affect the shape of the highlight arrow
const GLfloat HIGHLIGHT_RADIUS = 0.08f;
const GLfloat HIGHLIGHT_FACTOR = 3.0f;
const GLfloat HIGHLIGHT_LENGTH = HIGHLIGHT_RADIUS * HIGHLIGHT_FACTOR;
const GLint HIGHLIGHT_NSLICES = 12;
const GLint HIGHLIGHT_NRINGS = 1;
const GLint HIGHLIGHT_NSTACKS = 1;
// const GLfloat HIGHLIGHT_DIFFUSE_COLOR[] = {1.0, 0.0, 0.0, 0.5};
// const GLfloat HIGHLIGHT_AMBIENT_COLOR[] = {0.1, 0.0, 0.0, 0.5};

// Used in comparison of vertices
const GLfloat VERTEX_TOL = 0.0001f;

typedef QPair<int, int> FaceEdge;
typedef QPair<int, int> VertexEdge;
typedef QPair<FaceEdge, VertexEdge> SurfaceEdge;

struct TriangleIndex {
  GLuint i, j, k;

  inline TriangleIndex sorted() const {
    TriangleIndex result{*this};
    if (result.i > result.j)
      std::swap(result.i, result.j);
    if (result.i > result.k)
      std::swap(result.i, result.k);
    if (result.j > result.k)
      std::swap(result.j, result.k);
    return result;
  }

  inline bool contains(GLuint x) const {
    return (i == x) || (j == x) || (k == x);
  }

  inline int common(const TriangleIndex &rhs) const {
    int result{0};
    const auto l = this->sorted();
    const auto r = rhs.sorted();

    if (l.i == r.i)
      result++;
    else if (l.i == r.j)
      result++;
    else if (l.i == r.k)
      result++;

    if (l.j == r.i)
      result++;
    else if (l.j == r.j)
      result++;
    else if (l.j == r.k)
      result++;

    if (l.k == r.i)
      result++;
    else if (l.k == r.j)
      result++;
    else if (l.k == r.k)
      result++;
    return result;
  }
};

class Surface {
  friend QDataStream &operator<<(QDataStream &, const Surface &);
  friend QDataStream &operator>>(QDataStream &, Surface &);

public:
  using SurfaceColor = QVector4D;

  Surface();
  Surface(const Surface &); // copy constructor
  IsosurfaceDetails::Type type() const;
  ResolutionDetails::Level resolution() const;
  QString resolutionDescription() const;
  QString molecularOrbitalDescription() const;
  double isovalue() const;
  const JobParameters &jobParameters() { return m_jobParams; }
  void addVertex(float, float, float);
  void addFace(int, int, int);
  void addVertexNormal(float, float, float);
  void addProperty(QString, QVector<float>);
  void addInsideAtom(int, int, int, int);
  void addOutsideAtom(int, int, int, int);
  void addDiFaceAtom(int);
  void addDeFaceAtom(int);
  AtomId insideAtomIdForFace(int) const;
  AtomId outsideAtomIdForFace(int) const;
  int outsideAtomIndexForFace(int) const;
  const QVector<AtomId> &insideAtoms() const;
  const QVector<AtomId> &outsideAtoms() const;
  const auto &faceAreas() const { return m_faceAreas; }
  QVector<AtomId> diAtoms() const;
  QVector<AtomId> deAtoms() const;
  void save(const QString &) const;
  int faceIndexForVertex(int) const;

  void postReadingInit(const JobParameters &);
  bool isVoidSurface() const;
  bool isHirshfeldBased() const;
  bool isFingerprintable() const;
  void setNumberOfCaps(int n) { m_numCaps = n; }
  bool isCapped() const;
  void setCapsVisible(bool state) { m_drawCaps = state; }
  bool capsVisible() const { return m_drawCaps; }
  QString surfaceName() const;
  QString surfaceDescription() const;
  QString symmetryDescription() const;
  void setVisible(bool visibility) { m_visible = visibility; }
  void toggleVisible() { m_visible = !m_visible; }
  bool isVisible() const { return m_visible; }
  int numberOfFaces() const;
  bool isTransparent() const { return m_transparent; }
  void setTransparent(bool);

  void setShowInterior(bool);

  float areaOfFace(int) const;
  float area() const { return m_area; }
  float volume() const { return m_volume; }
  float globularity() const { return m_globularity; }
  float asphericity() const { return m_asphericity; }

  // Properties
  int currentPropertyIndex() const { return m_currentProperty; }
  int numberOfProperties() const { return m_properties.size(); }
  QStringList listOfProperties() const;
  QVector<IsosurfacePropertyDetails::Type> listOfPropertyTypes() const;
  const SurfaceProperty *currentProperty() const;
  const SurfaceProperty *propertyAtIndex(int i) const {
    return &m_properties[i];
  }
  bool hasProperty(IsosurfacePropertyDetails::Type) const;
  bool setCurrentProperty(int);
  float valueForPropertyAtFace(int, int) const;
  float valueForCurrentPropertyAtFace(int) const;
  float valueForPropertyAtVertex(int, int) const;
  float valueForCurrentPropertyAtVertex(int) const;
  void setCurrentPropertyRange(float, float);
  void setRangeForProperty(int, float, float);
  void updateNoneProperty();
  void addAdditionalProperty(QString, QVector<float>);
  void addFaceProperty(QString, QVector<float>);

  void drawFaceHighlights(LineRenderer *);

  // statistics
  QStringList statisticsLabels();

  // symmetry
  bool isParent() const { return _parent == nullptr; }
  const Surface *parent() const { return isParent() ? this : _parent; }
  QVector<Surface *> clones() const { return _clones; }
  void reportDeletionToParent() const;
  void cloneInit(const Surface *, bool preserve = false);
  void symmetryTransform(const Surface *, const SpaceGroup &, const UnitCell &,
                         const SymopId &, const Vector3q &);
  QVector<float> relativeShift() const { return _relativeShift; }
  SymopId symopId() const;

  // Drawing
  int numberOfFacesToDraw() const;
  void draw(int, bool drawAsMesh = false);
  void drawTwoSidedFacesForVoid(int basename, bool invertNormals = false);
  void resetMaskedFaces(bool state = false);
  void unmaskFace(int);
  void maskFace(int);
  void resetFaceHighlights();
  void highlightFace(int);

  QVector3D pos(int faceIndex, bool fromMiddleOfFace = false) const;
  QVector3D posClosestToExternalPosition(QVector3D externalPosition) const;
  QPair<QVector3D, QVector3D>
  positionsOfMinimumDistance(Surface *surface) const;
  QVector3D centroid() const;

  void setNonePropertyColor(QColor color);
  bool hasMaskedFaces() const { return _hasMaskedFaces; }

  // Surface Cleaning
  void clean();

  // VRML
  void addVRMLScriptToTextStream(QTextStream &);

  // Povray
  void exportToPovrayTextStream(QTextStream &, QString name, QString finish,
                                QString filter);

  // Domains
  void calculateDomains();
  bool hasCalculatedDomains() const;
  QVector<QColor> domainColors() const;
  QVector<double> domainVolumes() const;
  QVector<double> domainSurfaceAreas() const;

  // Surface Patches (see also fragment patches)
  void highlightFragmentPatchForFace(int);
  void highlightDiDePatchForFace(int);
  void highlightDiPatchForFace(int);
  void highlightDePatchForFace(int);
  void highlightCurvednessPatchForFace(int, float);

  // Fragment Patches
  QVector<QColor> colorsOfFragmentPatches();
  QVector<double> areasOfFragmentPatches();
  QVector<float>
      propertySummedOverFragmentPatches(IsosurfacePropertyDetails::Type);

  inline const QVector<QVector3D> &vertices() const { return m_vertices; }
  inline const QVector<QVector3D> &vertexNormals() const {
    return m_vertexNormals;
  }
  inline const QVector<SurfaceColor> &vertexColors() const {
    return m_diffuseColorsForCurrentProperty;
  }
  inline const QVector<TriangleIndex> &faces() const { return m_indices; }
  inline GLenum frontFace() const { return _frontFace; }
  inline void setFrontFace(GLenum face) { _frontFace = face; }

  void flipVertexNormals();
  bool faceMasked(int) const;

  void update();

private:
  void init();
  int numberOfVertices() const;
  double volumeContribution(int) const;

  // Descriptions
  QString generalSurfaceDescription() const;
  QString prefixedMolecularOrbitalDescription() const;

  // Properties
  void addNoneProperty();
  int defaultPropertyForSurfaceType(IsosurfaceDetails::Type) const;
  SurfaceProperty *getProperty(IsosurfacePropertyDetails::Type);
  float valueForPropertyTypeAtFace(int, IsosurfacePropertyDetails::Type) const;
  int propertyIndex(IsosurfacePropertyDetails::Type) const;

  // Updates
  void updateDerivedParameters();
  void updateColorsForCurrentProperty();
  void updateTransparencyForCurrentProperty();
  void updateFaceAreasAndNormals();
  void updateClosestAtomsPerFace();
  void updateArea();
  void updateVolume();
  void updateGlobularity();
  void updateAsphericity();
  void updateVertexToFaceMapping();

  // Domains
  void addDomainProperty();
  QSet<int> facesNeighboringFace(int) const;
  void mergeDomains();
  void sortDomains();
  void assignDomainsToFaces();
  void calculateNaiveDomains();
  bool isCapDomain(int) const;
  bool domainsHaveCommonPoint(QSet<int>, QSet<int>) const;
  bool verticesAreCoincident(int, int) const;
  double domainVolume(int) const;
  double domainSurfaceArea(int) const;

  // Surface patches
  void calculateNeighbors();
  bool meetsPatchCondition(int, float);
  bool hasNeighborsTable() const;
  QVector<int> neighbors(int) const;

  // Drawing
  bool faceHighlighted(int) const;
  void setFaceHighlightAmbientDiffuse(QColor);

  // Symmetry
  void addClone(Surface *);
  void removeClone(const Surface *);
  void transformVertices(Matrix3q, Vector3q);
  void transformNormals(Matrix3q);

  // Surface Cleaning
  int simplifyByEdgeCollapse();
  SurfaceEdge findFailingEdge() const;
  int findFaceSharingEdge(VertexEdge, int) const;
  bool faceHasVertexEdge(int, VertexEdge) const;
  bool isValidEdge(SurfaceEdge) const;
  bool isValidVertexEdge(VertexEdge) const;
  bool isValidFaceEdge(FaceEdge) const;
  void collapseEdge(SurfaceEdge);
  VertexEdge failingEdgeOfFace(int, double) const;
  VertexEdge shortestEdgeOfFace(int) const;
  VertexEdge longestEdgeOfFace(int) const;
  double edgeLength(VertexEdge) const;
  double edgeLength(int, int) const;
  void deleteFaces(FaceEdge);
  void collapseVertexEdge(VertexEdge);
  int countTJunctions() const;
  bool hasTrianglesSharingEdge(int, int, int) const;

  // Fragment Patches
  int fragmentIndexOfTriangle(int) const;

  QString m_surfaceName;
  float m_area{0};
  float m_volume{0};
  float m_globularity{0};
  float m_asphericity{0};
  int m_numCaps{0};

  QVector<QVector3D> m_vertices;
  QVector<QVector3D> m_vertexNormals;
  QVector<TriangleIndex> m_indices;
  QVector<QVector<int>> m_facesUsingVertex;
  QVector<double> m_faceAreas;
  QVector<QVector3D> m_faceNormals;
  QVector<bool> m_faceMaskFlags;
  QVector<bool> m_faceHighlightFlags;
  QVector<SurfaceColor> m_diffuseColorsForCurrentProperty;
  QVector<SurfaceColor> m_ambientColorsForCurrentProperty;
  QVector<SurfaceProperty> m_properties;
  int m_currentProperty;
  JobParameters m_jobParams;
  QVector<AtomId> m_atomsInsideSurface;
  QVector<AtomId> m_atomsOutsideSurface;
  QVector<int> m_insideAtomForFace;
  QVector<int> m_outsideAtomForFace;
  QSet<int> m_diAtoms;
  QSet<int> m_deAtoms;

  bool m_visible;
  bool m_drawCaps;
  bool m_transparent;
  bool _showInterior;

  GLfloat _faceHighlightDiffuse[4];
  GLfloat _faceHighlightAmbient[4];

  bool _hasMaskedFaces;

  // For neighbors
  QMap<int, QVector<int>> _neighbors;

  // For domains
  QVector<int> _domainForFace;
  QVector<QSet<int>> _domains;

  // For symmetry
  mutable Surface *_parent;
  QVector<Surface *> _clones;
  GLenum _frontFace;
  SymopId _symopId;
  QString _symopString;
  QVector<float> _relativeShift;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &, const Surface &);
QDataStream &operator>>(QDataStream &, Surface &);
