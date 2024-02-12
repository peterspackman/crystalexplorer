#include <QtDebug>
#include <QMessageBox>

#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <Eigen/LU>
#include <Eigen/QR>

#include "atom.h"
#include "elementdata.h"
#include "qeigen.h"

/* Ref: D. B. Owen, Handbook of Statistical Tables, (1962) pages 202-203 */
const char *Atom::thermalEllipsoidProbabilityStrings[] = {"20%", "50%", "99%"};
const float Atom::thermalEllipsoidProbabilityScaleFactors[] = {1.0026f, 1.5382f,
                                                               3.3682f};

QString Atom::defaultSymbol = QStringLiteral("Xx");

Atom::Atom() : m_frac_pos(0, 0, 0) {}

Atom::Atom(QString siteLabel, QString elementSymbol, float x, float y, float z,
           int disorderGroup, float occupancy) {
  m_site_label = siteLabel;
  m_frac_pos = {x, y, z};
  m_disorder_group = disorderGroup;
  m_occupancy = occupancy;

  Element *element = ElementData::elementFromSymbol(elementSymbol);
  if (element == nullptr) { // we have a problem - unknown element
    qDebug() << "Warning: element data not known/loaded for symbol" << elementSymbol;
  } else
    m_atomicNumber = element->number();
}

float Atom::covRadius() const {
  Element *el = ElementData::elementFromAtomicNumber(m_atomicNumber);
  if (el != nullptr) {
    return el->covRadius();
  }
  return 0.0;
}

float Atom::vdwRadius() const {
  Element *el = ElementData::elementFromAtomicNumber(m_atomicNumber);
  if (el != nullptr) {
    return el->vdwRadius();
  }
  return 0.0;
}

const QColor &Atom::color() const {
  if (m_use_custom_color) {
    return m_custom_color;
  }
  Element *el = ElementData::elementFromAtomicNumber(m_atomicNumber);
  if (el != nullptr) {
    return el->color();
  }
  return m_custom_color; // default to black
}

bool Atom::isHydrogen() const { return (m_atomicNumber == 1); }

float Atom::distanceToAtom(const Atom &other) const {
  return (m_pos - other.m_pos).norm();
}

void Atom::addAdp(QVector<float> adp) {
  Q_ASSERT(adp.size() == 6);
  m_adp = adp;
  calcThermalTensorAmplitudesRotations();
}

QString Atom::description(AtomDescription defaultAtomDescription) const {
  QString result;

  switch (defaultAtomDescription) {
  case AtomDescription::SiteLabel:
    result = label();
    break;
  case AtomDescription::UnitCellShift:
    result = QString("(%1,%2,%3)")
                 .arg(m_uc_shift.h)
                 .arg(m_uc_shift.k)
                 .arg(m_uc_shift.l);
    break;
  case AtomDescription::Hybrid:
    result = label() + QString("(%1,%2,%3) %4 *")
                           .arg(m_uc_shift.h)
                           .arg(m_uc_shift.k)
                           .arg(m_uc_shift.l)
                           .arg(m_uc_atom_idx);
    break;
    break;
  case AtomDescription::Coordinates:
    result = QString("(%1,%2,%3)").arg(fx()).arg(fy()).arg(fz());
    break;
  case AtomDescription::CartesianInfo:
    result = generalInfoDescription(x(), y(), z());
    break;
  case AtomDescription::FractionalInfo:
    result = generalInfoDescription(fx(), fy(), fz());
    break;
  }
  return result;
}

const QString &Atom::symbol() const {
  Element *el = ElementData::elementFromAtomicNumber(m_atomicNumber);
  if (el != nullptr)
    return el->symbol();
  return defaultSymbol;
}

Element *Atom::element() const {
  return ElementData::elementFromAtomicNumber(m_atomicNumber);
}

QString Atom::generalInfoDescription(double x, double y, double z) const {
  const int WIDTH = 5;
  const int PRECISION = 4;

  QString result;

  QString xString = QString("%1").arg(x, WIDTH, 'f', PRECISION);
  QString yString = QString("%1").arg(y, WIDTH, 'f', PRECISION);
  QString zString = QString("%1").arg(z, WIDTH, 'f', PRECISION);
  QString occString = QString("%1").arg(m_occupancy, 4, 'f', 3);
  result = QString("%1\t%2\t%3\t%4\t%5\t%6")
               .arg(label())
               .arg(symbol())
               .arg(xString)
               .arg(yString)
               .arg(zString)
               .arg(occString);

  return result;
}

void Atom::setEllipsoidProbability(QString probability) {
  for (int i = 0; i < numThermalEllipsoidSettings; i++) {
    if (thermalEllipsoidProbabilityStrings[i] == probability) {
      m_ellipsoid_p_scale_fac = thermalEllipsoidProbabilityScaleFactors[i];
    }
  }
}

void Atom::calcThermalTensorAmplitudesRotations() {
  const float THRESHOLD = 0.00001f;

  // Form thermal tensor matrix from the anharmonic displacement parameters
  // (adp)
  Matrix3q m;
  m << m_adp[0], m_adp[3], m_adp[4], m_adp[3], m_adp[1], m_adp[5], m_adp[4],
      m_adp[5], m_adp[2];

  // Calculate eigenvalues.  Include "true" to calculate eigenvectors as well.
  Eigen::SelfAdjointEigenSolver<Matrix3q> solver(m, Eigen::ComputeEigenvectors);

  // Eigenvalues are proportional to square of vibrational amplitudes
  Vector3q eValues = solver.eigenvalues();

  // Eigenvectors are the rotation matrices.  It seems the correct rotation is
  // the transpose.
  Matrix3q eVectors = solver.eigenvectors().transpose();

  // Make sure this is a rotation, and not an inversion as well
  float det = eVectors.determinant();
  if (det < 0) {
    eVectors *= -1.0;
  }

  // Vibrational amplitudes = sqrt(U) = sqrt(eValues)
  float d0 = sqrt(fabs(eValues(0)));
  float d1 = sqrt(fabs(eValues(1)));
  float d2 = sqrt(fabs(eValues(2)));
  Vector3q amps(d0, d1, d2);

  m_isotropic_thermal_ellipsoid =
      (fabs(d0 - d1) < THRESHOLD) && (fabs(d0 - d2) < THRESHOLD);
  _thermalTensorAmplitudesRotations.first = amps;
  _thermalTensorAmplitudesRotations.second = eVectors;
}

void Atom::displace(const Shift &shift, Matrix3q directCellMatrix) {
  m_uc_shift = shift;
  updateFractionalCoordinates();
  evaluateCartesianCoordinates(directCellMatrix);
}

void Atom::updateFractionalCoordinates() {
  m_frac_pos[0] += m_uc_shift.h;
  m_frac_pos[1] += m_uc_shift.k;
  m_frac_pos[2] += m_uc_shift.l;
}

void Atom::evaluateCartesianCoordinates(const Matrix3q &directCellMatrix) {
  m_pos = directCellMatrix * m_frac_pos;
}

bool Atom::isSameAtom(const Atom &atom) const {
  return m_uc_atom_idx == atom.m_uc_atom_idx && m_uc_shift == atom.m_uc_shift;
}

// This was initially written with qFuzzyCompare
// but this returned false for a difference in the 5th decimal place which was
// too strict.
bool Atom::atSamePosition(const Atom &atom) const {
  bool xc = fabs(fx() - atom.fx()) < POSITION_TOL;
  bool yc = fabs(fy() - atom.fy()) < POSITION_TOL;
  bool zc = fabs(fz() - atom.fz()) < POSITION_TOL;
  return xc && yc && zc;
}

void Atom::applySymop(const SpaceGroup &sg, const Matrix3q &directCellMatrix,
                      int symopId, int unitCellAtomIndex) {
  applySymopAlt(sg, directCellMatrix, symopId, unitCellAtomIndex,
                sg.translationForSymop(symopId));
}

// Differs from Atom::applySymop in that it accepts a shift from the user
// instead of applying the translation from the symop.
void Atom::applySymopAlt(const SpaceGroup &sg, const Matrix3q &directCellMatrix,
                         int symopId, int unitCellAtomIndex,
                         const Vector3q &relativeShift) {
  m_uc_atom_idx = unitCellAtomIndex;

  m_frac_pos = sg.rotationMatrixForSymop(symopId) * m_frac_pos;
  m_frac_pos += relativeShift;
  m_uc_shift.h = floor(m_frac_pos[0]);
  m_uc_shift.k = floor(m_frac_pos[1]);
  m_uc_shift.l = floor(m_frac_pos[2]);
  evaluateCartesianCoordinates(directCellMatrix);
}

void Atom::shiftToUnitCell(const Matrix3q &directCellMatrix) {
  for (int i = 0; i < 3; ++i) {
    while (m_frac_pos[i] < 0.0) {
      m_frac_pos[i] += 1.0;
    }
    while (m_frac_pos[i] >= 1.0) {
      m_frac_pos[i] -= 1.0;
    }
  }
  m_uc_shift.h = floor(m_frac_pos[0]);
  m_uc_shift.k = floor(m_frac_pos[1]);
  m_uc_shift.l = floor(m_frac_pos[2]);
  evaluateCartesianCoordinates(directCellMatrix);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QDataStream &operator<<(QDataStream &ds, const Atom &atom) {
  ds << atom.m_site_label;
  ds << atom.m_atomicNumber;
  ds << atom.m_frac_pos.x() << atom.m_frac_pos.y() << atom.m_frac_pos.z();
  ds << atom.m_pos.x() << atom.m_pos.y() << atom.m_pos.z();
  ds << atom.m_disorder_group << atom.m_occupancy;
  ds << atom.m_adp;
  ds << atom.m_uc_atom_idx << atom.m_uc_shift;
  ds << atom.m_visible << atom.m_contact_atom;
  ds << atom.m_ellipsoid_p_scale_fac;
  ds << atom.m_custom_color;
  ds << atom.m_use_custom_color;
  ds << atom.m_suppressed;
  return ds;
}

QDataStream &operator>>(QDataStream &ds, Atom &atom) {
  ds >> atom.m_site_label;
  ds >> atom.m_atomicNumber;

  qDebug() << atom.m_site_label << atom.m_atomicNumber;

  double x, y, z;
  ds >> x >> y >> z;
  atom.m_frac_pos << x, y, z;

  ds >> x >> y >> z;
  atom.m_pos << x, y, z;

  ds >> atom.m_disorder_group >> atom.m_occupancy;

  QVector<float> adp;
  ds >> adp;
  ds >> atom.m_uc_atom_idx >> atom.m_uc_shift;
  ds >> atom.m_visible >> atom.m_contact_atom;
  ds >> atom.m_ellipsoid_p_scale_fac;

  ds >> atom.m_custom_color;
  ds >> atom.m_use_custom_color;

  ds >> atom.m_suppressed;

  // reinit parts of atom not read in
  if (!adp.empty()) {
    atom.addAdp(adp);
  }
  atom.m_selected = false;

  return ds;
}
