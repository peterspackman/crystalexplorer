#include "orcainput.h"
#include "chemicalstructure.h"
#include <occ/core/element.h>

namespace io {

QString orcaInputString(const wfn::Parameters &params) {
  QByteArray destination;
  QTextStream ts(&destination);

  ts << "! " << params.method << " " << params.basis << Qt::endl;
  auto nums = params.structure->atomicNumbersForIndices(params.atoms);
  auto pos = params.structure->atomicPositionsForIndices(params.atoms);

  ts << "* xyz " << params.charge << " " << params.multiplicity << Qt::endl;
  // should be in angstroms
  for (int i = 0; i < nums.rows(); i++) {
    ts << QString::fromStdString(occ::core::Element(nums(i)).symbol()) << " ";
    ts << pos(0, i) << " " << pos(1, i) << " " << pos(2, i) << Qt::endl;
  }
  ts << "end" << Qt::endl;

  return QString(destination);
}

}
