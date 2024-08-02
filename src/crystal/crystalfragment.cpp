#include "crystalfragment.h"

QDebug operator<<(QDebug debug, const CrystalFragmentIndex &f) {
  debug.nospace() << "CrystalFragment(" << f.u << ": " << f.h << " " << f.k
                  << " " << f.l << ")";
  return debug;
}

QDebug operator<<(QDebug debug, const CrystalFragmentPair &f) {
  debug.nospace() << f.a << "->" << f.b;
  return debug;
}
