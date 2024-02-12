#include <QString>
#include <cmath>

#include "graphics.h"
#include "mathconstants.h"
#include "surface.h"
#include "surfacegenerationdialog.h" // for defn of IsosurfaceDetails::TypeLabels

#include "settings.h"

#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <Eigen/LU>
#include <Eigen/QR>

#include <QDebug>

Surface::Surface() { init(); }

Surface::Surface(const Surface &surface) // copy constructor
{
  if (&surface != this) {

    init();

    m_surfaceName = surface.m_surfaceName;
    m_numCaps = surface.m_numCaps;

    m_vertices = surface.m_vertices;
    m_vertexNormals = surface.m_vertexNormals;

    m_indices = surface.m_indices;

    m_jobParams = surface.m_jobParams;

    m_atomsInsideSurface = surface.m_atomsInsideSurface;
    m_atomsOutsideSurface = surface.m_atomsOutsideSurface;
    m_insideAtomForFace = surface.m_insideAtomForFace;
    m_outsideAtomForFace = surface.m_outsideAtomForFace;

    m_diAtoms = surface.m_diAtoms;
    m_deAtoms = surface.m_deAtoms;

    m_visible = surface.m_visible;
    m_drawCaps = surface.m_drawCaps;
    m_transparent = surface.m_transparent;

    _showInterior = surface._showInterior;

    m_facesUsingVertex = surface.m_facesUsingVertex;

    m_faceAreas = surface.m_faceAreas;
    m_faceNormals = surface.m_faceNormals;
    m_volume = surface.m_volume;
    m_area = surface.m_area;
    m_globularity = surface.m_globularity;
    m_asphericity = surface.m_asphericity;
    m_faceMaskFlags = surface.m_faceMaskFlags;
    m_faceHighlightFlags = surface.m_faceHighlightFlags;

    m_properties = surface.m_properties;
    setCurrentProperty(surface.m_currentProperty);

    // _parent set in init()
    // _frontFace set in init() and overridden in Surface::symmetryTransform if
    // cloning
    _frontFace = surface._frontFace;
    // _symopString set in init() and overridden in Surface::symmetryTransform
    // if cloning
    _symopId = surface._symopId;
    _relativeShift = surface._relativeShift;

    _domainForFace = surface._domainForFace;
    _domains = surface._domains;
  }
}

void Surface::init() {
  _parent = nullptr;

  m_numCaps = 0;
  m_area = m_volume = m_globularity = m_asphericity = 0.0;

  m_visible = true;
  m_drawCaps = true;
  m_transparent = false;

  m_currentProperty = -1;

  _hasMaskedFaces = false;

  _showInterior = false;

  _frontFace = GL_CCW;
  _symopId = NOSYMOP;
  _symopString = QString();
  _relativeShift = QVector<float>() << 0.0 << 0.0 << 0.0;

  // The following aren't initialized because they are based on Qt types and can
  // remain "empty"
  // _domainForFace, _domains, _neighbors
}

IsosurfaceDetails::Type Surface::type() const {
  return m_jobParams.surfaceType;
}

ResolutionDetails::Level Surface::resolution() const { return m_jobParams.resolution; }

QString Surface::resolutionDescription() const {
  return ResolutionDetails::name(resolution());
}

double Surface::isovalue() const { return m_jobParams.isovalue; }

QString Surface::surfaceName() const {
  return IsosurfaceDetails::getAttributes(m_jobParams.surfaceType).label;
}

QString Surface::molecularOrbitalDescription() const {
  QString orbital = orbitalLabels[m_jobParams.molecularOrbitalType];
  QString plusOrMinus =
      (m_jobParams.molecularOrbitalType == HOMO) ? QString("-") : QString("+");
  int level = m_jobParams.molecularOrbitalLevel;
  QString levelString =
      (level == 0) ? "" : QString("%1%2").arg(plusOrMinus).arg(level);
  return orbital + levelString;
}

QString Surface::prefixedMolecularOrbitalDescription() const {
  return QString("MO: ") + molecularOrbitalDescription();
}

QString Surface::generalSurfaceDescription() const {
  return QString("Isovalue: %1, Quality: %2")
      .arg(isovalue())
      .arg(resolutionDescription());
}

QString Surface::symmetryDescription() const {
  const int WIDTH = 2;

  QString description;
  if (_symopId != NOSYMOP) {
    QString shift1 = description = QString("+ { %1 } [%2,%3,%4]")
                                       .arg(_symopString)
                                       .arg(_relativeShift[0], WIDTH, 'f', 2)
                                       .arg(_relativeShift[1], WIDTH, 'f', 2)
                                       .arg(_relativeShift[2], WIDTH, 'f', 2);
  }
  return description;
}

QString Surface::surfaceDescription() const {
  QString description;
  switch (m_jobParams.surfaceType) {
  case IsosurfaceDetails::Type::Orbital:
    description = prefixedMolecularOrbitalDescription() + ", " +
                  generalSurfaceDescription();
    break;
  default:
    description = generalSurfaceDescription();
    break;
  }
  return description;
}

void Surface::addVertex(float x, float y, float z) {

  m_vertices.append(QVector3D(x, y, z));
}

int Surface::numberOfVertices() const { return m_vertices.size(); }

void Surface::addFace(int i0, int i1, int i2) {
  Q_ASSERT(qMax(qMax(i0, i1), i2) < numberOfVertices());
  m_indices.push_back({static_cast<unsigned int>(i0),
                       static_cast<unsigned int>(i1),
                       static_cast<unsigned int>(i2)});

  m_faceMaskFlags.append(false);
  m_faceHighlightFlags.append(false);
}

void Surface::addVertexNormal(float x, float y, float z) {
  m_vertexNormals.push_back(QVector3D(x, y, z));
}

// Ignorant of parent/clone relationship
// If you want to add property to parents AND clones (most likely)
// then use 'addAdditionalProperty'
void Surface::addProperty(QString propertyString,
                          QVector<float> propertyValues) {
  if (IsosurfacePropertyDetails::typeFromTontoName(propertyString) !=
      IsosurfacePropertyDetails::Type::Unknown) { // don't append properties
                                                  // CrystalExplorer doesn't
                                                  // know about
    m_properties.append(SurfaceProperty(propertyString, propertyValues));
  }
}

void Surface::addAdditionalProperty(QString propertyString,
                                    QVector<float> propertyValues) {
  addProperty(propertyString, propertyValues);

  if (isParent()) {
    foreach (Surface *clone, clones()) {
      clone->addProperty(propertyString, propertyValues);
    }
  }
}

// Similar to addProperty but takes a list of property values assigned to faces
// and assigns those values to the three vertices of the triangle face.
void Surface::addFaceProperty(QString propertyString,
                              QVector<float> faceValues) {
  QVector<float> propertyValues;

  for (int v = 0; v < numberOfVertices(); ++v) {
    propertyValues.append(-1.0);
  }

  size_t f = 0;
  for (const auto &face : m_indices) {
    float faceValue = faceValues[f];
    propertyValues[face.i] = faceValue;
    propertyValues[face.j] = faceValue;
    propertyValues[face.k] = faceValue;
    f++;
  }
  addAdditionalProperty(propertyString, propertyValues);
}

void Surface::addNoneProperty() {
  QVector<float> propertyValues;
  for (int i = 0; i < numberOfVertices(); ++i) {
    propertyValues << 0.0;
  }
  m_properties.prepend(SurfaceProperty("none", propertyValues));
}

void Surface::addDomainProperty() {
  QString propertyString = "domain";

  QVector<float> propertyValues;
  for (int i = 0; i < numberOfVertices(); ++i) {
    propertyValues << 0.0;
  }

  for (int f = 0; f < numberOfFaces(); ++f) {
    propertyValues[m_indices[f].i] = _domainForFace[f];
    propertyValues[m_indices[f].j] = _domainForFace[f];
    propertyValues[m_indices[f].k] = _domainForFace[f];
  }

  if (isParent()) {
    addProperty(propertyString, propertyValues);
    foreach (Surface *clone, clones()) {
      clone->addProperty(propertyString, propertyValues);
    }
  } else {
    _parent->addDomainProperty();
  }
}

void Surface::addInsideAtom(int atomIndex, int h1, int h2, int h3) {
  AtomId insideAtom;
  insideAtom.unitCellIndex = atomIndex;
  insideAtom.shift = {h1, h2, h3};
  m_atomsInsideSurface.push_back(insideAtom);
}

void Surface::addOutsideAtom(int atomIndex, int h1, int h2, int h3) {
  AtomId outsideAtom;
  outsideAtom.unitCellIndex = atomIndex;
  outsideAtom.shift = {h1, h2, h3};
  m_atomsOutsideSurface.push_back(outsideAtom);
}

void Surface::addDiFaceAtom(int diAtom) {
  Q_ASSERT(diAtom < m_atomsInsideSurface.size());
  m_diAtoms << diAtom;
  m_insideAtomForFace << diAtom;
}

void Surface::addDeFaceAtom(int deAtom) {
  Q_ASSERT(deAtom < m_atomsOutsideSurface.size());
  m_deAtoms << deAtom;
  m_outsideAtomForFace << deAtom;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Important Note:
// The following five methods: insideAtoms, outsideAtoms, insideAtomIdForFace,
// outsideAtomIdForFace, outsideAtomForFace
// are all based on the inside and outside atoms of the PARENT not the clone.
// They are still useful if you only care about the elements associated with the
// atoms.
// See FingerprintPlot::includeAreaFilteredByElement as an example.
// BUT if you actually need accurate AtomId's then these routines will not help
// you.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const QVector<AtomId> &Surface::insideAtoms() const {
  if (isParent()) {
    return m_atomsInsideSurface;
  } else {
    return parent()->insideAtoms();
  }
}

const QVector<AtomId> &Surface::outsideAtoms() const {
  if (isParent()) {
    return m_atomsOutsideSurface;
  } else {
    return parent()->outsideAtoms();
  }
}

AtomId Surface::insideAtomIdForFace(int face) const {
  if (isParent()) {
    return m_atomsInsideSurface[m_insideAtomForFace[face]];
  } else {
    return parent()->insideAtomIdForFace(face);
  }
}

AtomId Surface::outsideAtomIdForFace(int face) const {
  if (isParent()) {
    return m_atomsOutsideSurface[m_outsideAtomForFace[face]];
  } else {
    return parent()->outsideAtomIdForFace(face);
  }
}

int Surface::outsideAtomIndexForFace(int face) const {
  if (isParent()) {
    return m_outsideAtomForFace[face];
  } else {
    return parent()->outsideAtomIndexForFace(face);
  }
}

QVector<AtomId> Surface::diAtoms() const {
  QVector<AtomId> result;

  if (isParent()) {
    foreach (int index, m_diAtoms) {
      result.append(m_atomsInsideSurface[index]);
    }
  } else {
    result = parent()->diAtoms();
  }
  return result;
}

QVector<AtomId> Surface::deAtoms() const {
  QVector<AtomId> result;

  if (isParent()) {
    foreach (int index, m_deAtoms) {
      result.append(m_atomsOutsideSurface[index]);
    }
  } else {
    result = parent()->deAtoms();
  }
  return result;
}

void Surface::postReadingInit(const JobParameters &jobParams) {
  Q_ASSERT(numberOfVertices() > 0);
  Q_ASSERT(m_indices.size() > 0);
  Q_ASSERT(m_vertexNormals.size() == numberOfVertices());
  Q_ASSERT(m_atomsInsideSurface.size() > 0);

  m_jobParams = jobParams;
  updateDerivedParameters();
  addNoneProperty();
  setCurrentProperty(defaultPropertyForSurfaceType(m_jobParams.surfaceType));
}

void Surface::updateDerivedParameters() {
  // vertex to Face mapping doesn't appear to be used - remove?
  updateVertexToFaceMapping();
  updateFaceAreasAndNormals();
  updateArea();
  updateVolume();
  updateGlobularity();
  updateAsphericity();
}

void Surface::updateVertexToFaceMapping() {
  if (m_facesUsingVertex.size() > 0) {
    m_facesUsingVertex.clear();
  }
  m_facesUsingVertex.reserve(m_vertices.size());
  for (int v = 0; v < m_vertices.size(); ++v) {
    m_facesUsingVertex.push_back({});
  }

  for (int f = 0; f < m_indices.size(); ++f) {
    m_facesUsingVertex[m_indices[f].i].append(f);
    m_facesUsingVertex[m_indices[f].j].append(f);
    m_facesUsingVertex[m_indices[f].k].append(f);
  }
}

int Surface::defaultPropertyForSurfaceType(
    IsosurfaceDetails::Type surfaceType) const {
  IsosurfacePropertyDetails::Type propertyType;

  // if the surface was generated with none property then default property
  // depends on the surface type
  if (m_jobParams.requestedPropertyType ==
      IsosurfacePropertyDetails::Type::None) {
    switch (surfaceType) {
    case IsosurfaceDetails::Type::CrystalVoid:
      propertyType = IsosurfacePropertyDetails::Type::None;
      break;
    case IsosurfaceDetails::Type::SpinDensity:
      propertyType = IsosurfacePropertyDetails::Type::SpinDensity;
      break;
    case IsosurfaceDetails::Type::Orbital:
      propertyType = IsosurfacePropertyDetails::Type::Orbital;
      break;
    case IsosurfaceDetails::Type::DeformationDensity:
      propertyType = IsosurfacePropertyDetails::Type::DeformationDensity;
      break;
    case IsosurfaceDetails::Type::ElectricPotential:
      propertyType = IsosurfacePropertyDetails::Type::ElectricPotential;
      break;
    default: // All surfaces default to dnorm except orbital surface
      propertyType = IsosurfacePropertyDetails::Type::DistanceNorm;
      break;
    }
  } else { // default to the requested property
    propertyType = m_jobParams.requestedPropertyType;
  }

  int indexOfProperty = 0; // Default to the first property (i.e. none)
  for (int i = 0; i < m_properties.size(); ++i) {
    if (m_properties[i].type() == propertyType) {
      indexOfProperty = i;
      break;
    }
  }
  return indexOfProperty;
}

void Surface::updateFaceAreasAndNormals() {

  m_faceAreas.clear();
  m_faceNormals.clear();

  for (int f = 0; f < numberOfFaces(); ++f) {
    const auto &face = m_indices[f];
    const QVector3D &v0 = m_vertices[face.i];
    const QVector3D &v1 = m_vertices[face.j];
    const QVector3D &v2 = m_vertices[face.k];

    m_faceAreas.append(0.5 *
                       QVector3D::crossProduct(v0 - v1, v1 - v2).length());
    m_faceNormals.append(QVector3D::normal(v0, v1, v2));
  }
}

void Surface::updateColorsForCurrentProperty() {
  // Clear diffuse and ambient colors
  m_diffuseColorsForCurrentProperty.resize(numberOfVertices());
  m_ambientColorsForCurrentProperty.resize(numberOfVertices());

  // Set diffuse and ambient colors
  const GLfloat alpha = (m_transparent) ? TRANSPARENCY_ALPHA : 1.0;

  const float colorScale = 1.0f / 255;

  for (int v = 0; v < numberOfVertices(); v++) {

    SurfaceColor &diffuseColor = m_diffuseColorsForCurrentProperty[v];
    SurfaceColor &ambientColor = m_ambientColorsForCurrentProperty[v];
    QColor color = m_properties[m_currentProperty].colorAtVertex(v);
    diffuseColor[0] = (GLfloat)color.red() * colorScale;
    diffuseColor[1] = (GLfloat)color.green() * colorScale;
    diffuseColor[2] = (GLfloat)color.blue() * colorScale;
    diffuseColor[3] = alpha;

    ambientColor[0] = (GLfloat)color.red() * colorScale;
    ambientColor[1] = (GLfloat)color.green() * colorScale;
    ambientColor[2] = (GLfloat)color.blue() * colorScale;
    ambientColor[3] = alpha;
  }
}

void Surface::updateTransparencyForCurrentProperty() {
  const GLfloat alpha = (m_transparent) ? TRANSPARENCY_ALPHA : 1.0;

  for (int i = 0; i < m_diffuseColorsForCurrentProperty.size(); ++i) {
    auto &diffuseColor = m_diffuseColorsForCurrentProperty[i];
    diffuseColor[3] = alpha;

    auto &ambientColor = m_ambientColorsForCurrentProperty[i];
    ambientColor[3] = alpha;
  }
}

void Surface::setNonePropertyColor(QColor color) {
  for (int i = 0; i < m_properties.size(); ++i) {
    if (m_properties[i].type() == IsosurfacePropertyDetails::Type::None) {
      m_properties[i].setNonePropertyColor(color);
    }
  }

  if (m_properties[m_currentProperty].type() ==
      IsosurfacePropertyDetails::Type::None) {
    updateColorsForCurrentProperty();
  }
}

void Surface::setTransparent(bool transparency) {
  m_transparent = transparency;
  updateTransparencyForCurrentProperty();

  if (isParent()) {
    foreach (Surface *s, clones()) {
      s->setTransparent(transparency);
    }
  }
}

void Surface::updateArea() {
  m_area = 0.0;
  int nFaces =
      numberOfFaces() - m_numCaps; // caps don't contribute to the surface area
  for (int f = 0; f < nFaces; f++) {
    m_area += areaOfFace(f);
  }
}

float Surface::areaOfFace(int faceIndex) const {
  Q_ASSERT(faceIndex >= 0 && faceIndex < m_faceAreas.size());
  return m_faceAreas[faceIndex];
}

QVector3D Surface::pos(int faceIndex, bool fromMiddleOfFace) const {
  QVector3D v0 = m_vertices[m_indices[faceIndex].i];
  if (fromMiddleOfFace) {
    // A bit of extra work
    const QVector3D &v1 = m_vertices[m_indices[faceIndex].j];
    const QVector3D &v2 = m_vertices[m_indices[faceIndex].k];
    QVector3D a = v2 - v1;
    QVector3D b = v1 - v0;
    return (v0 + (a + b) / 3);
  } else {
    // Quick and simple
    return v0;
  }
}

QVector3D Surface::posClosestToExternalPosition(QVector3D externalPos) const {
  int closestFace = 0;
  // Use lengthSquared, as this should avoid square-root and be faster.
  float minDistance = (externalPos - pos(closestFace)).lengthSquared();
  for (int f = 0; f < numberOfFaces(); f++) {
    float distance = (externalPos - pos(f)).lengthSquared();
    if (minDistance > distance) {
      minDistance = distance;
      closestFace = f;
    }
  }
  return pos(closestFace);
}

QPair<QVector3D, QVector3D>
Surface::positionsOfMinimumDistance(Surface *surface) const {
  QVector3D pos1 = pos(0);
  QVector3D pos2 = surface->pos(0);
  // Use lengthSquared, as this should avoid square-root and be faster.
  float minDistance = (pos1 - pos2).lengthSquared();
  for (int f = 0; f < surface->numberOfFaces(); f++) {
    QVector3D pos = posClosestToExternalPosition(surface->pos(f));
    float distance = (pos - surface->pos(f)).lengthSquared();
    if (minDistance > distance) {
      minDistance = distance;
      pos1 = pos;
      pos2 = surface->pos(f);
    }
  }
  return QPair<QVector3D, QVector3D>(pos1, pos2);
}

void Surface::updateVolume() {
  m_volume = 0.0;
  for (int f = 0; f < numberOfFaces(); f++) {
    m_volume += volumeContribution(f);
  }
  // Depending on definition of normals can get an apparent '-ve' volume. Fix
  // this.
  m_volume = fabs(m_volume);
}

double Surface::volumeContribution(int face) const {
  const QVector3D &v0 = m_vertices[m_indices[face].i];
  double contribution =
      areaOfFace(face) * QVector3D::dotProduct(m_faceNormals[face], v0) / 3.0;
  return contribution;
}

void Surface::updateAsphericity() {
  const int nVertices = numberOfVertices();

  QVector3D surfaceCentroid = QVector3D(0, 0, 0);
  for (int v = 0; v < nVertices; v++) {
    surfaceCentroid += m_vertices[v];
    ;
  }
  surfaceCentroid /= nVertices;

  double xx, xy, xz, yy, yz, zz;
  xx = xy = xz = yy = yz = zz = 0.0;
  for (int v = 0; v < nVertices; v++) {
    const QVector3D &vertex = m_vertices[v];
    double dx = vertex.x() - surfaceCentroid.x();
    double dy = vertex.y() - surfaceCentroid.y();
    double dz = vertex.z() - surfaceCentroid.z();

    xx += dx * dx;
    xy += dx * dy;
    xz += dx * dz;
    yy += dy * dy;
    yz += dy * dz;
    zz += dz * dz;
  }

  // Create matrix m, calculate eigenvalues and store them in e
  Eigen::Matrix3d m;
  m << xx, xy, xz, xy, yy, yz, xz, yz, zz;
  Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> solver(m, false);
  Eigen::Vector3d e = solver.eigenvalues();

  double first_term = 0.0;
  double second_term = 0.0;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (i == j) {
        continue;
      }
      first_term += pow((e[i] - e[j]), 2);
    }
    second_term += e[i];
  }
  m_asphericity = (0.25 * first_term) / pow(second_term, 2);
}

void Surface::updateGlobularity() {
  Q_ASSERT(m_volume != 0.0);
  Q_ASSERT(m_area != 0.0);

  m_globularity = (pow(36 * PI, 1.0 / 3.0) * pow(m_volume, 2.0 / 3.0)) / m_area;
}

bool Surface::isVoidSurface() const {
  return type() == IsosurfaceDetails::Type::CrystalVoid;
}

bool Surface::isHirshfeldBased() const {
  return type() == IsosurfaceDetails::Type::Hirshfeld;
}

bool Surface::isFingerprintable() const {
  return isHirshfeldBased() &&
         (resolution() == ResolutionDetails::Level::High || resolution() == ResolutionDetails::Level::VeryHigh);
}

bool Surface::isCapped() const { return (m_numCaps > 0); }

int Surface::numberOfFaces() const { return m_indices.size(); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Properties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 The current property is only changed if the new property is not the same as the
 current property.
 Returns: true if the property changed and false otherwise.
 */
bool Surface::setCurrentProperty(int property) {
  if (property == m_currentProperty) {
    return false;
  }

  Q_ASSERT(property >= 0 && property < m_properties.size());

  m_currentProperty = property;
  updateColorsForCurrentProperty();

  if (isParent()) {
    foreach (Surface *s, clones()) {
      s->setCurrentProperty(property);
    }
  }
  return true;
}

void Surface::updateNoneProperty() {
  for (int i = 0; i < m_properties.size(); ++i) {
    if (m_properties[i].type() == IsosurfacePropertyDetails::Type::None) {
      m_properties[i].resetNonePropertyColor();
    }
  }

  if (m_properties[m_currentProperty].type() ==
      IsosurfacePropertyDetails::Type::None) {
    updateColorsForCurrentProperty();
  }
}

void Surface::setCurrentPropertyRange(float minValue, float maxValue) {
  setRangeForProperty(m_currentProperty, minValue, maxValue);
  if (isParent()) {
    foreach (Surface *s, clones()) {
      if (s->currentPropertyIndex() != m_currentProperty) {
        s->setCurrentProperty(m_currentProperty);
      }
      s->setRangeForProperty(m_currentProperty, minValue, maxValue);
    }
  }
}

void Surface::setRangeForProperty(int propertyIndex, float minValue,
                                  float maxValue) {
  // Commented out to allow us to set a symmetry range for the ESP product
  // property
  // SurfaceProperty* property = &_properties[propertyIndex];
  // if (minValue < property->min()) {
  //	minValue = property->min();
  //}
  // if (maxValue > property->max()) {
  //	maxValue = property->max();
  //}
  m_properties[propertyIndex].updateColors(minValue, maxValue);

  if (propertyIndex == m_currentProperty) {
    updateColorsForCurrentProperty();
  }
}

QStringList Surface::listOfProperties() const {
  QStringList propertyStrings;
  for (int i = 0; i < m_properties.size(); ++i) {
    propertyStrings << m_properties[i].propertyName();
  }
  return propertyStrings;
}

QVector<IsosurfacePropertyDetails::Type> Surface::listOfPropertyTypes() const {
  QVector<IsosurfacePropertyDetails::Type> propertyTypes;
  for (int i = 0; i < m_properties.size(); ++i) {
    propertyTypes << m_properties[i].type();
  }
  return propertyTypes;
}

const SurfaceProperty *Surface::currentProperty() const {
  Q_ASSERT(m_currentProperty < m_properties.size());

  if (m_currentProperty > -1) {
    return &m_properties[m_currentProperty];
  }
  return nullptr;
}

bool Surface::hasProperty(IsosurfacePropertyDetails::Type propertyType) const {
  for (const auto &property : m_properties) {
    if (property.type() == propertyType) {
      return true;
    }
  }
  return false;
}

SurfaceProperty *
Surface::getProperty(IsosurfacePropertyDetails::Type propertyType) {
  for (int i = 0; i < m_properties.size(); ++i) {
    if (m_properties[i].type() == propertyType) {
      return &m_properties[i];
    }
  }
  return nullptr;
}

float Surface::valueForPropertyTypeAtFace(
    int face, IsosurfacePropertyDetails::Type propertyType) const {
  int propIndex = propertyIndex(propertyType);
  Q_ASSERT(propIndex != -1);
  return valueForPropertyAtFace(face, propIndex);
}

int Surface::propertyIndex(IsosurfacePropertyDetails::Type propertyType) const {
  int result = -1;
  for (int i = 0; i < m_properties.size(); ++i) {
    if (m_properties[i].type() == propertyType) {
      result = i;
      break;
    }
  }
  return result;
}

// Calculates the mean value of the property given by propertyIndex for the
// three vertices of a given face
float Surface::valueForPropertyAtFace(int face, int propertyIndex) const {
  Q_ASSERT(propertyIndex >= 0 && propertyIndex < m_properties.size());

  float value = 0.0;
  const auto &f = m_indices[face];
  value += m_properties[propertyIndex].valueAtVertex(f.i);
  value += m_properties[propertyIndex].valueAtVertex(f.j);
  value += m_properties[propertyIndex].valueAtVertex(f.k);
  return value / 3;
}

/*
 \brief Calculates the mean value of the current property for the three vertices
 of a given face
 */
float Surface::valueForCurrentPropertyAtFace(int face) const {
  return valueForPropertyAtFace(face, m_currentProperty);
}

float Surface::valueForPropertyAtVertex(int vertex, int propertyIndex) const {
  Q_ASSERT(propertyIndex >= 0 && propertyIndex < m_properties.size());

  return m_properties[propertyIndex].valueAtVertex(vertex);
}

float Surface::valueForCurrentPropertyAtVertex(int vertex) const {
  return valueForPropertyAtVertex(vertex, m_currentProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Domains
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Surface::calculateDomains() {
  if (isParent()) {
    calculateNaiveDomains();
    mergeDomains();
    sortDomains();
    assignDomainsToFaces();
    addDomainProperty();
  } else {
    _parent->calculateDomains();
  }
}

bool Surface::hasCalculatedDomains() const {
  bool result;
  if (isParent()) {
    result = _domains.size() > 0;
  } else {
    result = _parent->hasCalculatedDomains();
  }
  return result;
}

void Surface::mergeDomains() {
  QVector<QSet<int>> capDomains;

  // Separate cap-domains from void-domains
  for (int c = _domains.size() - 1; c >= 0; --c) {
    if (isCapDomain(c)) {
      capDomains.append(_domains.takeAt(c));
    }
  }

  // Easy case: if only one void-domain, simply add all the cap-domains to it
  if (_domains.size() == 1) {
    foreach (QSet<int> capDomain, capDomains) {
      _domains[0].unite(capDomain);
    }
  } else {

    while (capDomains.size() > 0) {
      QSet<int> capDomain = capDomains.takeFirst();
      bool reassignedDomain = false;
      for (int i = 0; i < _domains.size(); ++i) {
        if (domainsHaveCommonPoint(capDomain, _domains[i])) {
          _domains[i].unite(capDomain);
          reassignedDomain = true;
        }
        qApp->processEvents(); // Prevent the GUI freezing
      }
      Q_ASSERT(reassignedDomain);
    }
  }
}

void Surface::sortDomains() {
  QVector<QSet<int>> sortedDomains;

  while (_domains.size() > 0) {
    double maxVolume = domainVolume(0);
    int domainToRemove = 0;
    for (int d = 1; d < _domains.size(); ++d) {
      double volume = domainVolume(d);
      if (volume > maxVolume) {
        maxVolume = volume;
        domainToRemove = d;
      }
    }
    sortedDomains.append(_domains.takeAt(domainToRemove));
  }
  _domains = sortedDomains;
}

void Surface::assignDomainsToFaces() {
  // Initialize _domainForFace;
  for (int f = 0; f < numberOfFaces(); ++f) {
    _domainForFace.append(0);
  }

  for (int d = 0; d < _domains.size(); ++d) {
    foreach (int face, _domains[d]) {
      _domainForFace[face] = d;
    }
  }
}

bool Surface::domainsHaveCommonPoint(QSet<int> domain1,
                                     QSet<int> domain2) const {
  // Convert domain into a list of vertices at the edge of the domain
  QSet<int> edgeVertices;
  foreach (int face, domain1) {
    int v = m_indices[face].i;
    if (m_facesUsingVertex[v].size() == 1) {
      edgeVertices.insert(v);
    }
    v = m_indices[face].j;
    if (m_facesUsingVertex[v].size() == 1) {
      edgeVertices.insert(v);
    }
    v = m_indices[face].k;
    if (m_facesUsingVertex[v].size() == 1) {
      edgeVertices.insert(v);
    }
  }

  foreach (int face, domain2) {
    int v = m_indices[face].i;
    if (m_facesUsingVertex[v].size()) {
      foreach (int refVertex, edgeVertices) {
        if (verticesAreCoincident(v, refVertex)) {
          return true;
        }
      }
    }
    v = m_indices[face].j;
    if (m_facesUsingVertex[v].size()) {
      foreach (int refVertex, edgeVertices) {
        if (verticesAreCoincident(v, refVertex)) {
          return true;
        }
      }
    }
    v = m_indices[face].k;
    if (m_facesUsingVertex[v].size()) {
      foreach (int refVertex, edgeVertices) {
        if (verticesAreCoincident(v, refVertex)) {
          return true;
        }
      }
    }
  }

  return false;
}

bool Surface::verticesAreCoincident(int v1, int v2) const {
  const auto &vertex1 = m_vertices[v1];
  const auto &vertex2 = m_vertices[v2];

  bool xTest = fabs(vertex1.x() - vertex2.x()) < VERTEX_TOL;
  bool yTest = fabs(vertex1.y() - vertex2.y()) < VERTEX_TOL;
  bool zTest = fabs(vertex1.z() - vertex2.z()) < VERTEX_TOL;

  return xTest && yTest && zTest;
}

bool Surface::isCapDomain(int domainIndex) const {
  QSet<int> domain = _domains[domainIndex];
  foreach (int face, domain) {
    if (face < numberOfFaces() - m_numCaps) {
      return false;
    }
  }
  return true;
}

void Surface::calculateNaiveDomains() {
  // Must process all faces
  QVector<int> facesToProcess;
  for (int f = 0; f < numberOfFaces(); f++) {
    facesToProcess.append(f);
  }

  while (facesToProcess.size() > 0) {

    // start a new domain
    QSet<int> domain;
    QSet<int> domainFacesToProcess;
    domainFacesToProcess.insert(facesToProcess.takeFirst());

    // Fill out domain
    while (domainFacesToProcess.size() > 0) {

      // Take first face from domainFacesToProcess
      int domainFace = domainFacesToProcess.values()[0];
      domainFacesToProcess.remove(domainFace);

      domain.insert(domainFace);

      // Mark domainFace as 'processed'
      facesToProcess.removeOne(domainFace);

      foreach (int face, facesNeighboringFace(domainFace)) {
        if (facesToProcess.contains(face)) {
          domainFacesToProcess.insert(face);
        }
      }
      qApp->processEvents(); // Prevent the GUI freezing
    }

    // Finalize domain
    _domains.append(domain);
  }
}

QSet<int> Surface::facesNeighboringFace(int face) const {
  QSet<int> newFaces;

  const auto &vertices = m_indices[face];
  newFaces.unite(QSet<int>(m_facesUsingVertex[vertices.i].begin(),
                           m_facesUsingVertex[vertices.i].end()));
  newFaces.unite(QSet<int>(m_facesUsingVertex[vertices.j].begin(),
                           m_facesUsingVertex[vertices.j].end()));
  newFaces.unite(QSet<int>(m_facesUsingVertex[vertices.k].begin(),
                           m_facesUsingVertex[vertices.k].end()));
  newFaces.remove(face);
  return newFaces;
}

QVector<QColor> Surface::domainColors() const {
  if (isParent()) {
    QVector<QColor> result;
    for (int d = 0; d < _domains.size(); ++d) {
      QColor color =
          ColorSchemer::color(IsosurfacePropertyDetails::getAttributes(
                                  IsosurfacePropertyDetails::Type::Domain)
                                  .colorScheme,
                              d, 0, _domains.size() - 1);
      result.append(color);
    }
    return result;
  } else {
    return parent()->domainColors();
  }
}

QVector<double> Surface::domainVolumes() const {
  if (isParent()) {
    QVector<double> result;
    for (int d = 0; d < _domains.size(); ++d) {
      result.append(domainVolume(d));
    }
    return result;
  } else {
    return parent()->domainVolumes();
  }
}

double Surface::domainVolume(int domainIndex) const {
  double vol = 0.0;
  foreach (int face, _domains[domainIndex]) {
    vol += volumeContribution(face);
  }
  return fabs(vol);
}

QVector<double> Surface::domainSurfaceAreas() const {
  if (isParent()) {
    QVector<double> result;
    for (int d = 0; d < _domains.size(); ++d) {
      result.append(domainSurfaceArea(d));
    }
    return result;
  } else {
    return parent()->domainSurfaceAreas();
  }
}

double Surface::domainSurfaceArea(int domainIndex) const {
  double area = 0.0;
  foreach (int face, _domains[domainIndex]) {
    if (face < numberOfFaces() - m_numCaps) {
      area += areaOfFace(face);
    }
  }
  return area;
}

int Surface::faceIndexForVertex(int vertex) const {
  Q_ASSERT(vertex > -1 && vertex < m_vertices.size());
  const auto &faces = m_facesUsingVertex[vertex];

  for (int face : m_facesUsingVertex[vertex]) {
    if (m_indices[face].k == static_cast<GLuint>(vertex))
      return face;
  }
  return faces[0];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fragment Patches
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QVector<QColor> Surface::colorsOfFragmentPatches() {
  Q_ASSERT(isHirshfeldBased());

  auto *fragPatchProp =
      getProperty(IsosurfacePropertyDetails::Type::FragmentPatch);
  Q_ASSERT(fragPatchProp);

  QMap<int, QColor> colorMap;

  for (int v = 0; v < numberOfVertices(); ++v) {
    int fragment = int(fragPatchProp->valueAtVertex(v));
    if (!colorMap.contains(fragment)) {
      colorMap[fragment] = fragPatchProp->colorAtVertex(v);
    }
  }

  auto keys = colorMap.keys();
  std::sort(keys.begin(), keys.end());
  QVector<QColor> colors;
  foreach (int key, keys) {
    colors.append(colorMap[key]);
  }
  return colors;
}

QVector<double> Surface::areasOfFragmentPatches() {
  Q_ASSERT(isHirshfeldBased());

  QMap<int, double> areaMap;
  for (int f = 0; f < numberOfFaces(); ++f) {
    int fragmentIndex = fragmentIndexOfTriangle(f);
    if (fragmentIndex == -1) {
      continue;
    }

    if (!areaMap.contains(fragmentIndex)) {
      areaMap[fragmentIndex] = areaOfFace(f);
    } else {
      areaMap[fragmentIndex] += areaOfFace(f);
    }
  }

  auto keys = areaMap.keys();
  std::sort(keys.begin(), keys.end());
  QVector<double> areas;
  foreach (int key, keys) {
    areas.append(areaMap[key]);
  }
  return areas;
}

int Surface::fragmentIndexOfTriangle(int face) const {
  Q_ASSERT(isHirshfeldBased());
  Q_ASSERT(hasProperty(IsosurfacePropertyDetails::Type::FragmentPatch));
  int v0 = m_indices[face].i;
  int v1 = m_indices[face].j;
  int v2 = m_indices[face].k;
  int f0 = -1;
  int f1 = -1;
  int f2 = -1;
  for (const auto &prop : m_properties) {
    if (prop.type() == IsosurfacePropertyDetails::Type::FragmentPatch) {
      f0 = prop.valueAtVertex(v0);
      f1 = prop.valueAtVertex(v1);
      f2 = prop.valueAtVertex(v2);
      break;
    }
  }

  if (f0 == f1 && f0 == f2) { // all 3 the same
    return f0;
  } else if (f0 == f1) { // 2 the same
    return f0;
  } else if (f0 == f2) { // 2 the same
    return f0;
  } else if (f1 == f2) { // 2 the same
    return f1;
  } else { // all 3 *different*
    return -1;
  }
}

QVector<float> Surface::propertySummedOverFragmentPatches(
    IsosurfacePropertyDetails::Type propertyType) {
  Q_ASSERT(hasProperty(propertyType));
  Q_ASSERT(hasProperty(IsosurfacePropertyDetails::Type::FragmentPatch));

  QVector<float> propertySum;
  QVector<float> patchArea;

  int minFragIndex =
      int(getProperty(IsosurfacePropertyDetails::Type::FragmentPatch)->min());
  int maxFragIndex =
      int(getProperty(IsosurfacePropertyDetails::Type::FragmentPatch)->max());
  int numFragmentPatches = maxFragIndex - minFragIndex + 1;

  for (int i = 0; i < numFragmentPatches; ++i) {
    propertySum.append(0.0);
    patchArea.append(0.0);
  }

  for (int f = 0; f < numberOfFaces(); ++f) {
    int fragIndex = fragmentIndexOfTriangle(f);
    if (fragIndex == -1) {
      continue;
    }

    double faceArea = m_faceAreas[f];
    float faceValue = valueForPropertyTypeAtFace(f, propertyType);

    propertySum[fragIndex] += faceArea * faceValue;
    patchArea[fragIndex] += faceArea;
  }

  for (int i = 0; i < numFragmentPatches; ++i) {
    propertySum[i] /= patchArea[i];
  }

  return propertySum;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Cloning
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SymopId Surface::symopId() const {
  if (isParent()) {
    return (SymopId)0;
  } else {
    return _symopId;
  }
}

void Surface::reportDeletionToParent() const {
  if (!isParent()) {
    Surface *p = const_cast<Surface *>(parent());
    p->removeClone(this);
  }
}

void Surface::addClone(Surface *clone) { _clones.append(clone); }

void Surface::removeClone(const Surface *clone) {
  _clones.removeOne(const_cast<Surface *>(clone));
}

// _relativeShift and _frontFace already set by init of parent
void Surface::cloneInit(const Surface *parentSurface, bool preserveSymopId) {
  if (!preserveSymopId) {
    _symopId = 0;
    _symopString = "x,y,z";
  }
  _parent = const_cast<Surface *>(parentSurface);
  _parent->addClone(this);
}

// Note this routine doesn't transform the lists of inside and outside atoms.
// Reason: It's not possible at the current time to transform them since
// CrystalExplorer receives incomplete symmetry information
// Incomplete in the sense that we only know one symop that maps the asymmetric
// atoms to their symmetry related unit cell atoms.
// However depending on the spacegroup, more that one symop may exist.
//
// Therefore clones differ from their parents in that they don't record their
// inside and outside atoms but rely on the parent for that
// information. As an examples see:
// (i) insideAtomIdForFace (ii) outsideAtomIdForFace (iii) outsideAtomForFace
// (iv) insideAtoms (v) outsideAtoms
void Surface::symmetryTransform(const Surface *parentSurface,
                                const SpaceGroup &spaceGroup,
                                const UnitCell &unitCell,
                                const SymopId &symopId, const Vector3q &shift) {
  cloneInit(parentSurface);

  _symopId = symopId;
  _symopString = spaceGroup.symopAsString(_symopId);

  for (int i = 0; i < 3; ++i) {
    _relativeShift[i] = shift[i];
  }

  // Calculate rotation and translation in cartesian frame
  Matrix3q rotMatrix = spaceGroup.rotationMatrixForSymop(symopId);
  Matrix3q cartRotMatrix =
      unitCell.directCellMatrix() * rotMatrix * unitCell.inverseCellMatrix();
  Vector3q cartTranslation = unitCell.directCellMatrix() * shift;

  // Set direction of face (clockwise/counterclockwise) depending on
  // proper/improper rotation so it's drawn correctly
  if (rotMatrix.determinant() == -1) {
    if (parentSurface->_frontFace == GL_CW) {
      _frontFace = GL_CCW;
    } else {
      _frontFace = GL_CW;
    }
  } else {
    _frontFace = parentSurface->_frontFace;
  }

  // Apply cartesian rotation and translation to vertices
  transformVertices(cartRotMatrix, cartTranslation);

  // Apply symop rotation to normals
  transformNormals(rotMatrix);

  // Clear inside and outside atoms
  m_atomsInsideSurface.clear();
  m_atomsOutsideSurface.clear();
}

// Expects the rotation matrix and translation in cartesian frame
void Surface::transformVertices(Matrix3q rotationMatrix,
                                Vector3q translationVector) {
  for (int i = 0; i < m_vertices.size(); ++i) {
    auto &vertex = m_vertices[i];
    Vector3q pos(vertex[0], vertex[1], vertex[2]);
    Vector3q newPos = rotationMatrix * pos + translationVector;

    vertex[0] = newPos(0);
    vertex[1] = newPos(1);
    vertex[2] = newPos(2);
  }
}

// Expects rotation matrix in crystal space
void Surface::transformNormals(Matrix3q rotationMatrix) {
  for (int i = 0; i < m_vertices.size(); ++i) {
    QVector3D &normal = m_vertexNormals[i];
    Vector3q oldNorm(normal[0], normal[1], normal[2]);
    Vector3q newNorm = rotationMatrix * oldNorm;
    newNorm.normalize();

    normal[0] = newNorm[0];
    normal[1] = newNorm[1];
    normal[2] = newNorm[2];
  }
}

void Surface::flipVertexNormals() {
  for (auto &normal : m_vertexNormals) {
    normal *= -1;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Patches
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Surface::highlightFragmentPatchForFace(int faceIndex) {
  resetMaskedFaces();

  int clickedFragIndex = fragmentIndexOfTriangle(faceIndex);
  if (clickedFragIndex == -1) {
    return;
  }

  for (int f = 0; f < numberOfFaces(); ++f) {
    int fragIndex = fragmentIndexOfTriangle(f);
    if (fragIndex != clickedFragIndex) {
      maskFace(f);
    }
  }
}

void Surface::highlightDiDePatchForFace(int faceIndex) {
  resetMaskedFaces();
  AtomId clickFaceDiAtomId = insideAtomIdForFace(faceIndex);
  AtomId clickFaceDeAtomId = outsideAtomIdForFace(faceIndex);
  for (int f = 0; f < numberOfFaces(); ++f) {
    AtomId diFaceAtomId = insideAtomIdForFace(f);
    AtomId deFaceAtomId = outsideAtomIdForFace(f);
    if (diFaceAtomId != clickFaceDiAtomId ||
        deFaceAtomId != clickFaceDeAtomId) {
      maskFace(f);
    }
  }
}

void Surface::highlightDiPatchForFace(int faceIndex) {
  resetMaskedFaces();
  AtomId clickFaceAtomId = insideAtomIdForFace(faceIndex);
  for (int f = 0; f < numberOfFaces(); ++f) {
    AtomId faceAtomId = insideAtomIdForFace(f);
    if (faceAtomId != clickFaceAtomId) {
      maskFace(f);
    }
  }
}

void Surface::highlightDePatchForFace(int faceIndex) {
  resetMaskedFaces();
  AtomId clickFaceAtomId = outsideAtomIdForFace(faceIndex);
  for (int f = 0; f < numberOfFaces(); ++f) {
    AtomId faceAtomId = outsideAtomIdForFace(f);
    if (faceAtomId != clickFaceAtomId) {
      maskFace(f);
    }
  }
}

void Surface::highlightCurvednessPatchForFace(int faceIndex, float threshold) {
  QStack<int> stack;
  QVector<int> processed;

  // Don't proceed if initial face (faceIndex) doesn't meet patch condition
  if (!meetsPatchCondition(faceIndex, threshold)) {
    return;
  }

  // Calculate face neighbor table if it doesn't exist
  if (!hasNeighborsTable()) {
    auto p = const_cast<Surface *>(parent());
    p->calculateNeighbors();
  }

  // mask all faces
  resetMaskedFaces(true);

  // Spread out from initial face (usually one clicked by the user)
  // to identify all the faces in the patch
  // To be a patch face, a face must
  // (i) neighbor an already identified patch face
  // (ii) meet the patch condition (see method meetsPatchCondition)
  // When found patch faces are unmasked
  stack.push(faceIndex);
  while (!stack.isEmpty()) {
    int face = stack.pop();

    foreach (int newFace, neighbors(face)) {
      if (!processed.contains(newFace) &&
          meetsPatchCondition(newFace, threshold)) {
        stack.push(newFace);
      }
    }

    processed.append(face);
    unmaskFace(face);
  }
}

void Surface::calculateNeighbors() {
  for (int f = 0; f < numberOfFaces(); ++f) {
    if (_neighbors[f].size() == 3) {
      continue;
    }
    const auto &face1 = m_indices[f];
    for (int f2 = f + 1; f2 < numberOfFaces(); ++f2) {
      const auto &face2 = m_indices[f2];
      int match = face2.common(face1);

      Q_ASSERT(match != 3);
      // two matching vertices means face shares an edge (they are neighbors)
      if (match == 2) {
        _neighbors[f].append(f2);
        _neighbors[f2].append(f);
      }
    }
  }
}

bool Surface::meetsPatchCondition(int face, float threshold) {
  const int CURVEDNESS_IND = 5;

  float v1 = m_properties[CURVEDNESS_IND].valueAtVertex(m_indices[face].i);
  float v2 = m_properties[CURVEDNESS_IND].valueAtVertex(m_indices[face].j);
  float v3 = m_properties[CURVEDNESS_IND].valueAtVertex(m_indices[face].k);

  return (v1 < threshold && v2 < threshold && v3 < threshold);
}

bool Surface::hasNeighborsTable() const {
  if (isParent()) {
    return !_neighbors.isEmpty();
  } else {
    return parent()->hasNeighborsTable();
  }
}

QVector<int> Surface::neighbors(int face) const {
  if (isParent()) {
    return _neighbors[face];
  } else {
    return parent()->neighbors(face);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Drawing Code
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int Surface::numberOfFacesToDraw() const {
  int nFacesToDraw = numberOfFaces();
  if (isCapped() && !m_drawCaps) {
    nFacesToDraw -= m_numCaps;
  }
  return nFacesToDraw;
}

void Surface::resetMaskedFaces(bool state) {
  std::fill(m_faceMaskFlags.begin(), m_faceMaskFlags.end(), state);
  _hasMaskedFaces = state;

  if (isParent()) {
    foreach (Surface *s, clones()) {
      s->resetMaskedFaces(state);
    }
  }
}

void Surface::unmaskFace(int f) {
  m_faceMaskFlags[f] = false;
  // we don't know if there are any other masked faces so
  // we leave _hasMaskedFaces unchanged unlike in Surface::maskFace(int f)

  if (isParent()) {
    foreach (Surface *s, clones()) {
      s->unmaskFace(f);
    }
  }
}

void Surface::maskFace(int f) {
  m_faceMaskFlags[f] = true;
  _hasMaskedFaces = true;

  if (isParent()) {
    foreach (Surface *s, clones()) {
      s->maskFace(f);
    }
  }
}

bool Surface::faceMasked(int f) const {
  Q_ASSERT(f < m_faceMaskFlags.size());
  return m_faceMaskFlags[f];
}

void Surface::resetFaceHighlights() {
  std::fill(m_faceHighlightFlags.begin(), m_faceHighlightFlags.end(), false);

  if (isParent()) {
    foreach (Surface *s, clones()) {
      s->resetFaceHighlights();
    }
  }
}

void Surface::highlightFace(int f) {
  m_faceHighlightFlags[f] = true;
  if (isParent()) {
    foreach (Surface *s, clones()) {
      s->highlightFace(f);
    }
  }
}

bool Surface::faceHighlighted(int f) const {
  Q_ASSERT(f < m_faceHighlightFlags.size());
  return m_faceHighlightFlags[f];
}

void Surface::setShowInterior(bool show) { _showInterior = show; }

void Surface::setFaceHighlightAmbientDiffuse(QColor color) {
  float red = color.red() / 255.0;
  float green = color.green() / 255.0;
  float blue = color.blue() / 255.0;

  const float alpha = 0.5f;
  const float factor = 0.1f;

  _faceHighlightDiffuse[0] = red;
  _faceHighlightDiffuse[1] = green;
  _faceHighlightDiffuse[2] = blue;
  _faceHighlightDiffuse[3] = alpha;
  _faceHighlightAmbient[0] = factor * red;
  _faceHighlightAmbient[1] = factor * green;
  _faceHighlightAmbient[2] = factor * blue;
  _faceHighlightAmbient[3] = alpha;
}

void Surface::drawFaceHighlights(LineRenderer *lines) {
  QColor color(
      settings::readSetting(settings::keys::FACE_HIGHLIGHT_COLOR).toString());
  for (int f = 0; f < numberOfFacesToDraw(); ++f) {
    if (faceHighlighted(f)) {
      const auto &face = m_indices[f];
      int v = m_indices[f].i;
      QVector3D faceCentroid =
          m_vertices[face.i] + m_vertices[face.j] + m_vertices[face.k];
      faceCentroid /= 3.0;
      const QVector3D &norm = m_vertexNormals[v];
      cx::graphics::addLineToLineRenderer(*lines, faceCentroid, faceCentroid + norm,
                                      1.0, color);
    }
  }
}

void Surface::save(const QString &filename) const {
  QFile file(filename);
  if (file.open(QIODevice::WriteOnly)) {
    QTextStream ts(&file);
    ts << "ply" << Qt::endl;
    ts << "format ascii 1.0" << Qt::endl;
    ts << "comment exported from CrystalExplorer" << Qt::endl;
    ts << "element vertex " << numberOfVertices() << Qt::endl;
    ts << "property float x" << Qt::endl;
    ts << "property float y" << Qt::endl;
    ts << "property float z" << Qt::endl;
    ts << "property float nx" << Qt::endl;
    ts << "property float ny" << Qt::endl;
    ts << "property float nz" << Qt::endl;
    ts << "property float red" << Qt::endl;
    ts << "property float green" << Qt::endl;
    ts << "property float blue" << Qt::endl;
    ts << "element face " << numberOfFaces() << Qt::endl;
    ts << "property list uchar int vertex_index" << Qt::endl;
    ts << "end_header" << Qt::endl;
    size_t idx = 0;
    for (const auto &vert : m_vertices) {
      const QVector3D &normal = m_vertexNormals[idx];
      const QVector4D &color = m_diffuseColorsForCurrentProperty[idx];
      ts << vert.x() << " " << vert.y() << " " << vert.z() << " " << normal.x()
         << " " << normal.y() << " " << normal.z() << " " << color.x() << " "
         << color.y() << " " << color.z() << Qt::endl;
      idx++;
    }
    for (const auto &face : m_indices) {
      ts << 3 << " " << face.i << " " << face.j << " " << face.k << Qt::endl;
    }
    file.close();
  } else {
    qDebug() << "Failed to open file";
  }
}

QVector3D Surface::centroid() const {
  QVector3D sum(0, 0, 0);
  for (int v = 0; v < numberOfVertices(); v++) {
    sum += QVector3D(m_vertices[v][0], m_vertices[v][1], m_vertices[v][2]);
  }
  return (sum / numberOfVertices());
}

/////////////////////////////////////////////////
// Surface Cleaning
/////////////////////////////////////////////////
void Surface::clean() {
  qDebug() << "Num T-junctions: " << countTJunctions();

  int numEdgeCollapses = simplifyByEdgeCollapse();
  qDebug() << "Edge collapses: " << numEdgeCollapses;

  updateDerivedParameters();
  qDebug() << "Num T-junctions: " << countTJunctions();
}

int Surface::countTJunctions() const {
  int count = 0;
  for (int f = 0; f < m_indices.size(); f++) {

    if (!hasTrianglesSharingEdge(f, m_indices[f].i, m_indices[f].j)) {
      count++;
    }

    if (!hasTrianglesSharingEdge(f, m_indices[f].j, m_indices[f].k)) {
      count++;
    }

    if (!hasTrianglesSharingEdge(f, m_indices[f].k, m_indices[f].i)) {
      count++;
    }
  }

  return count;
}

bool Surface::hasTrianglesSharingEdge(int face, int v0, int v1) const {
  Q_UNUSED(face);
  QSet<int> facesForVertex0(m_facesUsingVertex[v0].begin(),
                            m_facesUsingVertex[v0].end());
  QSet<int> facesForVertex1(m_facesUsingVertex[v1].begin(),
                            m_facesUsingVertex[v1].end());
  facesForVertex0.intersect(facesForVertex1);
  int numFaces = facesForVertex0.size();
  return numFaces == 2;
}

int Surface::simplifyByEdgeCollapse() {
  int numEdgeCollapses = 0;
  SurfaceEdge edge = findFailingEdge();
  while (isValidEdge(edge)) {
    collapseEdge(edge);
    numEdgeCollapses++;
    edge = findFailingEdge();
  }
  return numEdgeCollapses;
}

SurfaceEdge Surface::findFailingEdge() const {
  const double EDGE_TOL = 0.05;

  FaceEdge fe;
  fe.first = fe.second = -1;
  VertexEdge ve;
  ve.first = ve.second = -1;

  for (int i = 0; i < m_indices.size(); ++i) {
    ve = failingEdgeOfFace(i, EDGE_TOL);
    if (isValidVertexEdge(ve)) {
      int edgeSharingFace = findFaceSharingEdge(ve, i);
      Q_ASSERT(edgeSharingFace != -1);
      fe.first = i;
      fe.second = edgeSharingFace;
      break;
    }
  }

  return SurfaceEdge(fe, ve);
}

int Surface::findFaceSharingEdge(VertexEdge vertexEdge, int faceToSkip) const {
  for (int i = 0; i < m_indices.size(); ++i) {
    if (i == faceToSkip) {
      continue;
    }

    if (faceHasVertexEdge(i, vertexEdge)) {
      return i;
    }
  }
  return -1;
}

bool Surface::faceHasVertexEdge(int face, VertexEdge vertexEdge) const {
  const auto &indices = m_indices[face];
  return indices.contains(vertexEdge.first) &&
         indices.contains(vertexEdge.second);
}

VertexEdge Surface::failingEdgeOfFace(int faceIndex, double tolerance) const {
  VertexEdge shortEdge = shortestEdgeOfFace(faceIndex);
  if (edgeLength(shortEdge) < tolerance) {
    return shortEdge;
  }
  return VertexEdge(-1, -1);
}

VertexEdge Surface::shortestEdgeOfFace(int face) const {
  int i0 = m_indices[face].i;
  int i1 = m_indices[face].j;
  int i2 = m_indices[face].k;

  double edge01 = edgeLength(i0, i1);
  double edge02 = edgeLength(i0, i2);
  double edge12 = edgeLength(i1, i2);

  VertexEdge ve;
  if (edge01 < edge02) {
    if (edge01 < edge12) {
      ve = qMakePair(i0, i1);
    } else {
      ve = qMakePair(i1, i2);
    }
  } else {
    if (edge02 < edge12) {
      ve = qMakePair(i0, i2);
    } else {
      ve = qMakePair(i1, i2);
    }
  }
  return ve;
}

VertexEdge Surface::longestEdgeOfFace(int face) const {
  int i0 = m_indices[face].i;
  int i1 = m_indices[face].j;
  int i2 = m_indices[face].k;

  double edge01 = edgeLength(i0, i1);
  double edge02 = edgeLength(i0, i2);
  double edge12 = edgeLength(i1, i2);

  VertexEdge ve;
  if (edge01 > edge02) {
    if (edge01 > edge12) {
      ve = qMakePair(i0, i1);
    } else {
      ve = qMakePair(i1, i2);
    }
  } else {
    if (edge02 > edge12) {
      ve = qMakePair(i0, i2);
    } else {
      ve = qMakePair(i1, i2);
    }
  }
  return ve;
}

double Surface::edgeLength(VertexEdge vertexEdge) const {
  return edgeLength(vertexEdge.first, vertexEdge.second);
}

double Surface::edgeLength(int vertexIndex1, int vertexIndex2) const {
  double dx = m_vertices[vertexIndex1][0] - m_vertices[vertexIndex2][0];
  double dy = m_vertices[vertexIndex1][1] - m_vertices[vertexIndex2][1];
  double dz = m_vertices[vertexIndex1][2] - m_vertices[vertexIndex2][2];

  return sqrt(dx * dx + dy * dy + dz * dz);
}

bool Surface::isValidEdge(SurfaceEdge e) const {
  return isValidFaceEdge((FaceEdge)e.first);
}

bool Surface::isValidVertexEdge(VertexEdge ve) const {
  return ve.first != -1 && ve.second != -1;
}

bool Surface::isValidFaceEdge(FaceEdge fe) const {
  return fe.first != -1 && fe.second != -1;
}

void Surface::collapseEdge(SurfaceEdge edge) {
  // delete faces stored in face edge
  deleteFaces(edge.first);

  // merge vertices
  collapseVertexEdge(edge.second);
}

void Surface::deleteFaces(FaceEdge fe) {
  if (fe.first > fe.second) {
    m_indices.removeAt(fe.first);
    m_indices.removeAt(fe.second);
  } else {
    m_indices.removeAt(fe.second);
    m_indices.removeAt(fe.first);
  }
}

void Surface::collapseVertexEdge(VertexEdge ve) {
  GLuint first = static_cast<GLuint>(ve.first);
  GLuint second = static_cast<GLuint>(ve.second);
  for (int i = 0; i < m_indices.size(); ++i) {
    auto &face = m_indices[i];
    if (face.i == second) {
      face.i = first;
    }
    if (face.j == second) {
      face.j = first;
    }
    if (face.k == second) {
      face.k = first;
    }
  }

  // Average the positions of the vertices
  for (int i = 0; i < 3; ++i) {
    m_vertices[first][i] = (m_vertices[first][i] + m_vertices[second][i]) / 2.0;
  }

  // Fix up property values
  foreach (SurfaceProperty property, m_properties) {
    property.mergeValues(ve.first, ve.second);
  }
}

/////////////////////////////////////////////////
// VRML
/////////////////////////////////////////////////
void Surface::addVRMLScriptToTextStream(QTextStream &ts) {
  // General parameters
  ts << "Transform {\n";
  ts << "    children [\n";
  ts << "        Shape {\n";
  ts << "            appearance Appearance {\n";
  ts << "                material Material {\n";
  ts << "                    ambientIntensity 0.050\n";
  ts << "                    diffuseColor     0.800 0.800 0.800\n";
  ts << "                    emissiveColor    0.000 0.000 0.000\n";
  ts << "                    shininess        1.000\n";
  ts << "                    specularColor    0.300 0.300 0.300\n";
  ts << "                    transparency     0.000\n";
  ts << "                }\n";
  ts << "            }\n";
  ts << "            geometry IndexedFaceSet {\n";
  ts << "                solid FALSE\n";
  ts << "                convex FALSE\n";
  ts << "                normalPerVertex TRUE\n";
  ts << "                colorPerVertex TRUE\n";

  // Vertices
  QVector3D center = centroid();
  ts << "                coord Coordinate {\n";
  ts << "                    point [\n";
  for (int v = 0; v < m_vertices.size(); v++) {
    ts << m_vertices[v][0] - center.x() << " " << m_vertices[v][1] - center.y()
       << " " << m_vertices[v][2] - center.z() << ",  ";
    if ((v + 1) % 3 == 0) {
      ts << "\n";
    }
  }
  ts << "                    ]\n";
  ts << "                 }\n";

  // Normals
  ts << "                normal Normal {\n";
  ts << "                    vector [\n";
  for (int v = 0; v < m_vertices.size(); v++) {
    ts << m_vertexNormals[v][0] << " " << m_vertexNormals[v][1] << " "
       << m_vertexNormals[v][2] << ",  ";
    if ((v + 1) % 3 == 0) {
      ts << "\n";
    }
  }
  ts << "                    ]\n";
  ts << "                 }\n";

  // Colors
  const float colorScale = 1.0f / 255;
  ts << "                color Color {\n";
  ts << "                    color [\n";
  for (int v = 0; v < m_vertices.size(); v++) {
    QColor color = m_properties[m_currentProperty].colorAtVertex(v);
    float r = color.red() * colorScale;
    float g = color.green() * colorScale;
    float b = color.blue() * colorScale;
    ts << r << " " << g << " " << b << ",  ";
    if ((v + 1) % 3 == 0) {
      ts << "\n";
    }
  }
  ts << "                    ]\n";
  ts << "                 }\n";

  // Triangles
  ts << "                 coordIndex [\n";
  for (int f = 0; f < numberOfFacesToDraw(); f++) {
    ts << m_indices[f].i << ", " << m_indices[f].j << ", " << m_indices[f].k
       << ", -1, ";
    if ((f + 1) % 3 == 0) {
      ts << "\n";
    }
  }
  ts << "                 ]\n";

  // Footer
  ts << "            }\n";
  ts << "        }\n";
  ts << "    ]\n";
  ts << "}\n";
}

void Surface::exportToPovrayTextStream(QTextStream &ts, QString surfaceName,
                                       QString surfaceFilter,
                                       QString surfaceFinish) {
  const float colorScale = 1.0f / 255;

  ts << "#declare " + surfaceName + " = mesh {\n";
  for (int f = 0; f < numberOfFacesToDraw(); f++) {
    ts << "smooth_triangle {\n";
    std::array<GLuint, 3> idxs = {m_indices[f].i, m_indices[f].j,
                                  m_indices[f].k};
    for (int k = 0; k < 3; k++) {
      int v = idxs[k];
      float v0 = m_vertices[v][0];
      float v1 = m_vertices[v][1];
      float v2 = m_vertices[v][2];
      float n0 = m_vertexNormals[v][0];
      float n1 = m_vertexNormals[v][1];
      float n2 = m_vertexNormals[v][2];
      ts << "   <" << v0 << "," << v1 << "," << v2 << ">, <" << n0 << "," << n1
         << "," << n2 << ">";
      if (k < 2) {
        ts << ",";
      }
      ts << "\n";
    }
    for (int k = 0; k < 3; k++) {
      int v = idxs[k];
      QColor color = m_properties[m_currentProperty].colorAtVertex(v);
      float r = colorScale * color.red();
      float g = colorScale * color.green();
      float b = colorScale * color.blue();
      ts << "#declare t" << k << " = texture{pigment{rgbt <" << r << "," << g
         << "," << b << "," << surfaceFilter << ">} finish {" << surfaceFinish
         << "}}\n";
    }
    ts << "texture_list {t0 t1 t2}\n";
    ts << "}\n";
  }
  ts << "}\n";
}

QStringList Surface::statisticsLabels() {
  return QStringList(propertyStatisticsNames.values());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QDataStream &operator<<(QDataStream &ds, const TriangleIndex &t) {
  ds << t.i;
  ds << t.j;
  ds << t.k;
  return ds;
}

QDataStream &operator>>(QDataStream &ds, TriangleIndex &t) {
  ds >> t.i;
  ds >> t.j;
  ds >> t.k;
  return ds;
}

QDataStream &operator<<(QDataStream &ds, const Surface &surface) {
  ds << surface.m_surfaceName;
  ds << surface.m_numCaps;

  ds << surface.m_vertices.size();
  for (const auto &vertex : surface.m_vertices) {
    ds << vertex;
  }

  ds << surface.m_vertexNormals.size();

  for (const auto &normal : surface.m_vertexNormals) {
    ds << normal;
  }

  ds << surface.m_indices;

  ds << surface.m_properties;
  ds << surface.m_currentProperty;

  ds << surface.m_jobParams;

  ds << surface.m_atomsInsideSurface;
  ds << surface.m_atomsOutsideSurface;
  ds << surface.m_insideAtomForFace;
  ds << surface.m_outsideAtomForFace;
  ds << surface.m_diAtoms;
  ds << surface.m_deAtoms;

  ds << surface.m_visible << surface.m_drawCaps << surface.m_transparent;

  ds << surface._showInterior;

  ds << surface._frontFace;
  ds << surface._symopId;
  ds << surface._relativeShift;
  ds << surface._symopString;

  ds << surface._domainForFace;
  ds << surface._domains;

  ds << surface.isParent();

  return ds;
}


void Surface::update() {
  updateVertexToFaceMapping();
  updateFaceAreasAndNormals();
  updateArea();
  updateVolume();
  updateGlobularity();
  updateAsphericity();
}

QDataStream &operator>>(QDataStream &ds, Surface &surface) {
  ds >> surface.m_surfaceName;
  ds >> surface.m_numCaps;

  qsizetype nVertices;
  ds >> nVertices;
  for (int i = 0; i < nVertices; ++i) {
    QVector3D vertex;
    ds >> vertex;
    surface.m_vertices.append(vertex);
  }

  qsizetype nNormals;
  ds >> nNormals;
  for (int i = 0; i < nNormals; ++i) {
    QVector3D normal;
    ds >> normal;
    surface.m_vertexNormals.append(normal);
  }

  ds >> surface.m_indices;

  ds >> surface.m_properties;
  int currentProperty;
  ds >> currentProperty;
  surface.m_currentProperty = -1;
  surface.setCurrentProperty(currentProperty);

  ds >> surface.m_jobParams;

  ds >> surface.m_atomsInsideSurface;
  ds >> surface.m_atomsOutsideSurface;
  ds >> surface.m_insideAtomForFace;
  ds >> surface.m_outsideAtomForFace;
  ds >> surface.m_diAtoms;
  ds >> surface.m_deAtoms;

  ds >> surface.m_visible >> surface.m_drawCaps >> surface.m_transparent;

  ds >> surface._showInterior;

  ds >> surface._frontFace;
  ds >> surface._symopId;
  ds >> surface._relativeShift;
  ds >> surface._symopString;

  ds >> surface._domainForFace;
  ds >> surface._domains;

  bool isParent;
  ds >> isParent;
  surface._parent = isParent ? nullptr : (Surface *)1;
  // NB _parent and _clones set outside of the reading since they are pointers

  surface.updateVertexToFaceMapping();
  surface.updateFaceAreasAndNormals();
  surface.updateArea();
  surface.updateVolume();
  surface.updateGlobularity();
  surface.updateAsphericity();

  for (int i = 0; i < surface.m_indices.size(); ++i) {
    surface.m_faceMaskFlags.append(false);
    surface.m_faceHighlightFlags.append(false);
  }

  return ds;
}
