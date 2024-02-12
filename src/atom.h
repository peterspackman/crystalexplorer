#pragma once

#include <QColor>
#include <QString>
#include <QVector3D>
#include <QtOpenGL>

#include "atomid.h"
#include "element.h"
#include "qeigen.h"
#include "spacegroup.h"
#include "unitcell.h"

enum class AtomDescription {
  SiteLabel,
  UnitCellShift,
  Hybrid,
  Coordinates,
  CartesianInfo,
  FractionalInfo
};

// Used to decide whether two atomic positions are the same
const double POSITION_TOL = 0.0001;

class Atom {
  friend QDataStream &operator<<(QDataStream &, const Atom &);
  friend QDataStream &operator>>(QDataStream &, Atom &);

public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  Atom();
  Atom(QString, QString, float, float, float, int, float);

  QVector3D pos() const { return QVector3D(m_pos[0], m_pos[1], m_pos[2]); }
  const Vector3q &posvector() const { return m_pos; }
  float x() const { return m_pos[0]; }
  float y() const { return m_pos[1]; }
  float z() const { return m_pos[2]; }
  float fx() const { return m_frac_pos[0]; }
  float fy() const { return m_frac_pos[1]; }
  float fz() const { return m_frac_pos[2]; }
  const QString &symbol() const;
  float covRadius() const;
  float vdwRadius() const;
  void evaluateCartesianCoordinates(const Matrix3q &);
  const QColor &color() const;
  bool isHydrogen() const;
  float distanceToAtom(const Atom &) const;

  Element *element() const;
  QVector<float> adp() {
    return m_adp;
  } // --> Asymmetric Displacement Parameters = Thermal Ellipsoids

  void addAdp(QVector<float>);
  bool hasAdp() const { return m_adp.size() == 6; }

  QString description(AtomDescription defaultAtomDescription =
                          AtomDescription::SiteLabel) const;

  const QString &label() const { return m_site_label; }
  void setLabel(const QString &s) { m_site_label = s; }

  void setVisible(bool visibility) { m_visible = visibility; }
  bool isVisible() const { return m_visible; }

  void toggleSelected() { m_selected = !m_selected; }
  void setSelected(bool selected) { m_selected = selected; }
  bool isSelected() const { return m_selected; }

  void setSuppressed(bool suppress) { m_suppressed = suppress; }
  bool isSuppressed() const { return m_suppressed; }

  void setContactAtom(bool contactAtom) { m_contact_atom = contactAtom; }
  bool isContactAtom() const { return m_contact_atom; }

  void setUnitCellAtomIndexTo(int index) {
    m_uc_atom_idx = index;
  } // Should only be used in Crystal::setUnitCellAtoms

  int unitCellAtomIndex() const { return m_uc_atom_idx; }
  const Shift &unitCellShift() const { return m_uc_shift; }
  AtomId atomId() const { return AtomId{m_uc_atom_idx, m_uc_shift}; }
  void displace(const Shift &, Matrix3q);
  bool isSameAtom(const Atom &) const;
  bool atSamePosition(const Atom &) const;
  void applySymop(const SpaceGroup &, const Matrix3q &, int, int);
  void applySymopAlt(const SpaceGroup &, const Matrix3q &, int, int,
                     const Vector3q &);
  void shiftToUnitCell(const Matrix3q &);

  int disorderGroup() const { return m_disorder_group; }
  bool isDisordered() const { return m_disorder_group != 0; }
  float occupancy() const { return m_occupancy; }

  void setEllipsoidProbability(QString);
  bool thermalEllipsoidIsIsotropic() const {
    return m_isotropic_thermal_ellipsoid;
  }
  static const char *thermalEllipsoidProbabilityStrings[3];
  static const float thermalEllipsoidProbabilityScaleFactors[3];
  static const int numThermalEllipsoidSettings = 3;
  const QPair<Vector3q, Matrix3q> &thermalTensorAmplitudesRotations() const {
    return _thermalTensorAmplitudesRotations;
  }

  bool hasCustomColor() const { return m_use_custom_color; }
  void setCustomColor(QColor color) {
    m_custom_color = color;
    m_use_custom_color = true;
  }
  void clearCustomColor() {
    m_use_custom_color = false;
    m_custom_color = Qt::black;
  }

private:
  static QString defaultSymbol;
  void calcThermalTensorAmplitudesRotations();
  void updateFractionalCoordinates();
  QString generalInfoDescription(double, double, double) const;

  QString m_site_label;
  int m_atomicNumber{0};
  Vector3q m_frac_pos{0.0, 0.0, 0.0};
  Vector3q m_pos{0.0, 0.0, 0.0};
  int m_disorder_group{0};
  float m_occupancy{0.0f};
  QVector<float> m_adp;
  int m_uc_atom_idx{0};
  Shift m_uc_shift{0, 0, 0};
  bool m_visible{true};
  bool m_selected{false};
  bool m_contact_atom{false};
  bool m_suppressed{false};

  bool m_isotropic_thermal_ellipsoid{false};
  float m_ellipsoid_p_scale_fac{0.0f};
  QPair<Vector3q, Matrix3q> _thermalTensorAmplitudesRotations;

  QColor m_custom_color{Qt::black};
  bool m_use_custom_color{false};
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &, const Atom &);
QDataStream &operator>>(QDataStream &, Atom &);
