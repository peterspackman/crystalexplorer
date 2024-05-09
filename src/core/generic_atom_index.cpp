#include "generic_atom_index.h"

QDebug operator<<(QDebug debug, const GenericAtomIndex& g) {
    debug.nospace() << "Idx{" << g.unique << ", " << g.x << ", " << g.y << ", " << g.z << "}\n";
    return debug;
}
