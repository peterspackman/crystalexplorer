#include "xtbcoord.h"
#include <QTextStream>
#include <QByteArray>
#include "chemicalstructure.h"
#include "occ/core/element.h"

QString xtbCoordString(const xtb::Parameters &params) {
  QByteArray destination;
  QTextStream ts(&destination);

  ts << "$coord angs" << Qt::endl;
  auto nums = params.structure->atomicNumbersForIndices(params.atoms);
  auto pos = params.structure->atomicPositionsForIndices(params.atoms);

  // should be in angstroms
  for (int i = 0; i < nums.rows(); i++) {
    ts << pos(0, i) << " " << pos(1, i) << " " << pos(2, i) <<  " " << 
      QString::fromStdString(occ::core::Element(nums(i)).symbol()) << Qt::endl;
  }

  ts << "$gfn" << Qt::endl;
  int method = 2;
  switch(params.method) {
    default: break;
    case xtb::Method::GFN0_xTB: 
      method = 0;
      break;
    case xtb::Method::GFN1_xTB: 
      method = 1;
      break;
  };
  ts << "method=" << method << Qt::endl;
  ts << "$chrg"
     << " " << params.charge << Qt::endl;
  ts << "$spin"
     << " " << params.multiplicity - 1 << Qt::endl;
  ts << "$write" << Qt::endl
     << "json=true" << Qt::endl; // ask xtb to write out a json file
  ts << "$end" << Qt::endl;

  return QString(destination);
}
