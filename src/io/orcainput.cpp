#include "orcainput.h"
#include "chemicalstructure.h"
#include <occ/core/element.h>
#include "settings.h"
namespace io {

QString orcaInputString(const wfn::Parameters &params) {
  QByteArray destination;
  QTextStream ts(&destination);
  int numProcs = settings::readSetting(settings::keys::ORCA_NTHREADS).toInt();
  numProcs = numProcs == 0 ? 1 : numProcs;
  ts << "! " << params.method << " " << params.basis << Qt::endl;
  ts << "%PAL NPROCS " << numProcs << " END" << Qt::endl;
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
