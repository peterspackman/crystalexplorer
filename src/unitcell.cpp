#include "unitcell.h"
#include "mathconstants.h"

UnitCell::UnitCell() { _a = _b = _c = _alpha = _beta = _gamma = _volume = 0.0; }

UnitCell::UnitCell(float a, float b, float c, float alpha, float beta,
                   float gamma) {
  _a = a;
  _b = b;
  _c = c;
  _alpha = alpha;
  _beta = beta;
  _gamma = gamma;
  init();
}

void UnitCell::init() {
  float ca = cos(_alpha * RAD_PER_DEG);
  float cb = cos(_beta * RAD_PER_DEG);
  float cg = cos(_gamma * RAD_PER_DEG);
  float sg = sin(_gamma * RAD_PER_DEG);

  _volume = _a * _b * _c *
            sqrt(1.0 - ca * ca - cb * cb - cg * cg + 2.0 * ca * cb * cg);

  _aAxis << _a, 0.0, 0.0;
  _bAxis << _b * cg, _b * sg, 0.0;
  _cAxis << _c * cb, _c * (ca - cb * cg) / sg, _volume / (_a * _b * sg);

  _directCellMatrix.col(0) = _aAxis;
  _directCellMatrix.col(1) = _bAxis;
  _directCellMatrix.col(2) = _cAxis;

  _inverseCellMatrix = _directCellMatrix.inverse();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &ds, const UnitCell &unitCell) {
  ds << unitCell._a << unitCell._b << unitCell._c << unitCell._alpha
     << unitCell._beta << unitCell._gamma;
  return ds;
}

QDataStream &operator>>(QDataStream &ds, UnitCell &unitCell) {
  ds >> unitCell._a >> unitCell._b >> unitCell._c >> unitCell._alpha >>
      unitCell._beta >> unitCell._gamma;
  unitCell.init();
  return ds;
}
