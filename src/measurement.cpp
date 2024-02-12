#include <QtDebug>
#include <cmath>

#include "globals.h"
#include "graphics.h"
#include "mathconstants.h"
#include "measurement.h"
#include "settings.h"
#include <fmt/core.h>

#include <QDebug>

Measurement::Measurement() { m_color = QColor("green"); }

Measurement::Measurement(MeasurementType measurementType)
    : _type(measurementType) {
  m_color = QColor("green");
}

void Measurement::addPosition(QVector3D pos) {
  _positions.append(pos);
  if (_positions.size() == Measurement::totalPositions(_type)) {
    calculateMeasurement();
  }
}

void Measurement::setColor(QColor color) { m_color = color; }

void Measurement::calculateMeasurement() {
  Q_ASSERT(_positions.size() == Measurement::totalPositions(_type));

  switch (_type) {
  case MeasurementType::Distance:
    calculateDistance();
    break;
  case MeasurementType::Angle:
    calculateAngle();
    break;
  case MeasurementType::Dihedral:
    calculateDihedral();
    break;
  case MeasurementType::OutOfPlaneBend:
    calculateOutOfPlaneBend();
    break;
  case MeasurementType::InPlaneBend:
    calculateInPlaneBend();
    break;
  }
}

void Measurement::calculateDistance() {
  _value = (_positions[0] - _positions[1]).length();

  _label = QString::fromStdString(fmt::format("{:.3f}Å ", _value));

  _labelPosition = (_positions[0] + _positions[1]) / 2.0;
}

// void Measurement::calculateMinimumDistance()
//{
//	//_value = ;
//	//_label = ;
//	//_labelPostion = ;
//}

void Measurement::calculateAngle() {
  const double RADIAL_FRACTION = 8 / 10.0;
  const double ANGULAR_FRACTION = 5 / 10.0;

  QVector3D d0 = _positions[0] - _positions[1];
  QVector3D d1 = _positions[2] - _positions[1];
  double dp = QVector3D::dotProduct(d0, d1);
  _value = acos(dp / (d0.length() * d1.length())) * DEG_PER_RAD;

  _label =
      QString("%1° ").arg(QString::number(_value, 'f', ANGLE_TEXT_PRECISION));
  _labelPosition = _positions[1] + RADIAL_FRACTION * d0 +
                   ANGULAR_FRACTION * RADIAL_FRACTION * (d1 - d0);
}

/// \brief Calculate the dihedral angle between the four points in _positions
/// Code taken from:
/// http://xiang-jun.blogspot.com/2009/10/how-to-calculate-torsion-angle.html
void Measurement::calculateDihedral() {
  QVector3D b_c_norm = (_positions[2] - _positions[1]).normalized();
  QVector3D b_a = _positions[0] - _positions[1];
  QVector3D c_d = _positions[3] - _positions[2];
  QVector3D b_a_orth =
      (b_a - b_c_norm * QVector3D::dotProduct(b_a, b_c_norm)).normalized();
  QVector3D c_d_orth =
      (c_d - b_c_norm * QVector3D::dotProduct(c_d, b_c_norm)).normalized();
  double _value = acos(QVector3D::dotProduct(b_a_orth, c_d_orth)) * DEG_PER_RAD;
  double sign = QVector3D::dotProduct(
      QVector3D::crossProduct(b_a_orth, c_d_orth), b_c_norm);
  if (sign < 0.0) {
    _value = -_value;
  }

  _label =
      QString("%1° ").arg(QString::number(_value, 'f', ANGLE_TEXT_PRECISION));

  _labelPosition = (_positions[3] + _positions[0]) / 2.0;
}

void Measurement::calculateOutOfPlaneBend() {
  const double thresh = 0.00001;

  QVector3D v = (_positions[0] - _positions[1]);
  QVector3D x = (_positions[2] - _positions[1]);
  QVector3D y = (_positions[3] - _positions[1]);

  QVector3D n = QVector3D::crossProduct(x, y);
  double dv = v.length();
  double dn = n.length();
  QVector3D m =
      QVector3D::crossProduct(n, QVector3D::crossProduct(v, n) / n.length()) /
      n.length();

  // out of plane bend angle
  double angle = 0.0;
  if (dv > thresh && dn > thresh) {
    angle = (double)acos(QVector3D::dotProduct(v, n) / (dv * dn));
  }
  angle = angle * DEG_PER_RAD;
  if (angle > 90.0) {
    angle = angle - 90.0;
  } else {
    angle = 90.0 - angle;
  }

  _label =
      QString("%1° ").arg(QString::number(angle, 'f', ANGLE_TEXT_PRECISION));
  _labelPosition = (v + m) / 2.0 + _positions[1];
}

void Measurement::calculateInPlaneBend() {
  const double thresh = 0.00001;

  QVector3D center = _positions[1];

  QVector3D v = (_positions[0] - center);
  QVector3D x = (_positions[2] - center);
  QVector3D y = (_positions[3] - center);

  QVector3D n = QVector3D::crossProduct(x, y);
  double dn = n.length();
  QVector3D m =
      QVector3D::crossProduct(n, QVector3D::crossProduct(v, n) / dn) / dn;
  double dm = m.length();

  // QVector3D u = (x.normalized() + y.normalized())/2.0;
  QVector3D u = x;
  double du = u.length();

  // out of plane bend angle
  float angle = 0.0;
  if (dm > thresh && du > thresh) {
    angle = (float)acos(QVector3D::dotProduct(m, u) / (dm * du));
  }
  angle = angle * DEG_PER_RAD;
  if (angle > 180.0) {
    angle = angle - 180.0;
  }

  _label =
      QString("%1° ").arg(QString::number(angle, 'f', ANGLE_TEXT_PRECISION));
  _labelPosition = (_positions[3] + _positions[0]) / 2.0;
}

void Measurement::draw(LineRenderer *lines, CircleRenderer *circles) const {
  switch (_type) {
  case MeasurementType::Distance:
    drawDistance(lines, circles);
    break;
  case MeasurementType::Angle:
    drawAngle(lines, circles);
    break;
  case MeasurementType::Dihedral:
    drawDihedral(lines, circles);
    break;
  case MeasurementType::OutOfPlaneBend:
    drawOutOfPlaneBend(lines, circles);
    break;
  case MeasurementType::InPlaneBend:
    drawInPlaneBend(lines, circles);
    break;
  }
}

float Measurement::lineRadius() const {
  double bondThicknessRatio =
      settings::readSetting(settings::keys::CONTACT_LINE_THICKNESS).toInt() /
      100.0;
  return bondThicknessRatio;
}

void Measurement::drawDistance(LineRenderer *lines,
                               CircleRenderer *circles) const {
  const double LINE_RADIUS = lineRadius();

  cx::graphics::addDashedLineToLineRenderer(*lines, _positions[0], _positions[1],
                                        LINE_RADIUS, m_color);
}

void Measurement::drawAngle(LineRenderer *lines,
                            CircleRenderer *circles) const {
  const float LINE_RADIUS = lineRadius();

  QVector3D v0 = _positions[0] - _positions[1];
  QVector3D v1 = _positions[2] - _positions[1];
  //    double radius = qMin(v0.length(), v1.length()) * VECTOR_FRACTION;
  //    Graphics::drawPieBetweenVectors(v0, v1, _positions[1], radius,
  //    SPHERE_RADIUS,
  //                                  ANGLE_COLOR);
  cx::graphics::addCurvedLineToLineRenderer(*lines, v0, v1, _positions[1],
                                        LINE_RADIUS * 2, m_color);
}

void Measurement::drawDihedral(LineRenderer *lines,
                               CircleRenderer *circles) const {
  const double LINE_RADIUS = lineRadius();
  QColor planeColor = m_color;
  planeColor.setAlphaF(0.2);
  const double LINE_LENGTH = 1.0;

  QVector3D center = (_positions[1] + _positions[2]) / 2.0;

  QVector3D a = _positions[0] - _positions[1];
  QVector3D b_norm = (_positions[2] - _positions[1]).normalized();
  QVector3D d0 = a - QVector3D::dotProduct(a, b_norm) * b_norm;
  QVector3D p0 = center + LINE_LENGTH * d0.normalized();
  cx::graphics::addDashedLineToLineRenderer(*lines, center, p0, LINE_RADIUS,
                                        m_color);

  QVector3D c = _positions[3] - _positions[2];
  QVector3D d1 = c - QVector3D::dotProduct(c, b_norm) * b_norm;
  QVector3D p1 = center + LINE_LENGTH * d1.normalized();
  cx::graphics::addDashedLineToLineRenderer(*lines, center, p1, LINE_RADIUS,
                                        m_color);

  QVector3D v0 = p0 - center;
  QVector3D v1 = p1 - center;
  cx::graphics::addCurvedLineToLineRenderer(*lines, 0.5 * v0, 0.5 * v1, center,
                                        LINE_RADIUS, m_color);

  //  Graphics::drawPieBetweenVectors(v0, v1, center, LINE_LENGTH,
  //  SPHERE_RADIUS,
  //                                  DIHEDRAL_COLOR);

  cx::graphics::addPartialDiskToCircleRenderer(
      *circles, a, (_positions[2] - _positions[1]), _positions[1], planeColor);
  cx::graphics::addPartialDiskToCircleRenderer(
      *circles, c, (_positions[1] - _positions[2]), _positions[2], planeColor);
}

void Measurement::drawOutOfPlaneBend(LineRenderer *lines,
                                     CircleRenderer *circles) const {
  const double LINE_RADIUS = lineRadius();
  QColor planeColor = m_color;
  planeColor.setAlphaF(0.2);
  QVector3D center = _positions[1];

  QVector3D v = (_positions[0] - center);
  QVector3D x = (_positions[2] - center);
  QVector3D y = (_positions[3] - center);

  QVector3D n = QVector3D::crossProduct(x, y);
  double dn = n.length();
  QVector3D m =
      QVector3D::crossProduct(n, QVector3D::crossProduct(v, n) / dn) / dn;

  float lx = x.length();
  float ly = y.length();
  QVector3D right = x.normalized();
  QVector3D up = y.normalized();
  up = up - QVector3D::dotProduct(right, up) * right;
  up.normalize();

  up *= std::max(lx, ly);
  right *= std::max(lx, ly);

  cx::graphics::addDashedLineToLineRenderer(*lines, center, center + m, LINE_RADIUS,
                                        m_color);
  cx::graphics::addDashedLineToLineRenderer(*lines, center, center + v, LINE_RADIUS,
                                        m_color);
  cx::graphics::addCurvedLineBetweenVectors(*circles, v, m, center, m_color);
  cx::graphics::addCircleToCircleRenderer(*circles, center, right, up, planeColor);
}

void Measurement::drawInPlaneBend(LineRenderer *lines,
                                  CircleRenderer *circles) const {
  const double LINE_RADIUS = lineRadius();
  QColor planeColor = m_color;
  planeColor.setAlphaF(0.2);

  QVector3D center = _positions[1];

  QVector3D v = (_positions[0] - center);
  QVector3D x = (_positions[2] - center);
  QVector3D y = (_positions[3] - center);

  QVector3D n = QVector3D::crossProduct(x, y);
  double dn = n.length();
  QVector3D m =
      QVector3D::crossProduct(n, QVector3D::crossProduct(v, n) / dn) / dn;

  // QVector3D u = (x.normalized() + y.normalized())/2.0;
  QVector3D u = x;

  float lx = x.length();
  float ly = y.length();
  QVector3D right = x.normalized();
  QVector3D up = y.normalized();
  up = up - QVector3D::dotProduct(right, up) * right;
  up.normalize();

  up *= std::max(lx, ly);
  right *= std::max(lx, ly);

  cx::graphics::addDashedLineToLineRenderer(*lines, center, center + u, LINE_RADIUS,
                                        m_color);
  cx::graphics::addDashedLineToLineRenderer(*lines, center, center + m, LINE_RADIUS,
                                        m_color);

  cx::graphics::addCurvedLineBetweenVectors(*circles, u, m, center, m_color);

  cx::graphics::addCircleToCircleRenderer(*circles, center, right, up, planeColor);
}
