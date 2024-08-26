#include "fragment_index.h"

QDebug operator<<(QDebug debug, const FragmentIndex &f) {
  debug.nospace() << "CrystalFragment(" << f.u << ": " << f.h << " " << f.k
                  << " " << f.l << ")";
  return debug;
}

QDebug operator<<(QDebug debug, const FragmentIndexPair &f) {
  debug.nospace() << f.a << "->" << f.b;
  return debug;
}
