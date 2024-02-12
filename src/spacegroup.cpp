#include <QDataStream>
#include <QString>
#include <QTextStream>
#include <QtDebug>

#include "spacegroup.h"

Matrix3q SpaceGroup::rotationMatrixForSymop(const SymopId &symop) const {
  return _seitzMatrices[symop].block<3, 3>(0, 0);
}

Vector3q SpaceGroup::translationForSymop(const SymopId &symop) const {
  return _seitzMatrices[symop].block<3, 1>(0, 3);
}

// Based on Tonto's recode_Jones_Faithful_symbol [see spacegroup.foo]
// Appropriated 6/2/14 Tonto Version: 4264
QString SpaceGroup::symopAsString(const SymopId &symopId) const {
  if (symopId == -1) {
    return "-";
  }

  QString result;

  Matrix4q matrix = _seitzMatrices[symopId];

  for (int i = 0; i < 3; ++i) {
    QString str = " ";

    str = rotationString(str, matrix(i, 0), "x");
    str = rotationString(str, matrix(i, 1), "y");
    str = rotationString(str, matrix(i, 2), "z");
    str = translationString(str, matrix(i, 3));

    // Remove leading "+" if present
    if (str.startsWith('+')) {
      str = str.mid(1);
    }

    if (i == 0) {
      result = str.trimmed();
    } else if (str == " ") {
      result = str.trimmed();
    } else {
      result = result.trimmed() + ", " + str.trimmed();
    }
  }
  return result;
}

QString SpaceGroup::rotationString(const QString &s, double value,
                                   const QString &coord) const {
  QString result;

  if (fabs(value - 0.0) < SYMOP_STRING_TOL) {
    result = s;
  } else if (fabs(value - 1.0) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "+" + coord;
  } else if (fabs(value + 1.0) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "-" + coord;
  } else {
    result = s.trimmed() + "?";
  }
  return result;
}

QString SpaceGroup::translationString(const QString &s, double value) const {
  QString result;

  if (fabs(value - 0.0) < SYMOP_STRING_TOL) {
    result = s;
  } else if (fabs(value - 0.5) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "+1/2";
  } else if (fabs(value + 0.5) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "-1/2";
  } else if (fabs(value - 1 / 3.0) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "+1/3";
  } else if (fabs(value + 1 / 3.0) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "-1/3";
  } else if (fabs(value - 2 / 3.0) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "+2/3";
  } else if (fabs(value + 2 / 3.0) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "-2/3";
  } else if (fabs(value - 0.25) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "+1/4";
  } else if (fabs(value + 0.25) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "-1/4";
  } else if (fabs(value - 0.75) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "+3/4";
  } else if (fabs(value + 0.75) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "-3/4";
  } else if (fabs(value - 1 / 6.0) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "+1/6";
  } else if (fabs(value + 1 / 6.0) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "-1/6";
  } else if (fabs(value - 5 / 6.0) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "+5/6";
  } else if (fabs(value + 5 / 6.0) < SYMOP_STRING_TOL) {
    result = s.trimmed() + "-5/6";
  } else {
    result = s.trimmed() + "+?";
  }

  return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &ds, const SpaceGroup &spaceGroup) {
  ds << spaceGroup._HM_Symbol;

  quint32 nSeitz = spaceGroup._seitzMatrices.size();
  ds << nSeitz;
  for (quint32 n = 0; n < nSeitz; ++n) {
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        ds << spaceGroup._seitzMatrices[n](i, j);
      }
    }
  }

  ds << spaceGroup._inverseSymops;
  ds << spaceGroup._symopProducts;

  return ds;
}

QDataStream &operator>>(QDataStream &ds, SpaceGroup &spaceGroup) {
  ds >> spaceGroup._HM_Symbol;

  quint32 nSeitz;
  ds >> nSeitz;
  for (quint32 n = 0; n < nSeitz; ++n) {
    Matrix4q mat;
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        qreal value;
        ds >> value;
        mat(i, j) = value;
      }
    }
    spaceGroup._seitzMatrices.append(mat);
  }

  ds >> spaceGroup._inverseSymops;
  ds >> spaceGroup._symopProducts;

  return ds;
}
