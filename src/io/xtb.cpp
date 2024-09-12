#include "xtb.h"
#include "chemicalstructure.h"
#include "occ/core/element.h"
#include <QByteArray>
#include <QFile>
#include <QTextStream>

QString xtbCoordString(const xtb::Parameters &params) {
  QByteArray destination;
  QTextStream ts(&destination);

  ts << "$coord angs" << Qt::endl;
  auto nums = params.structure->atomicNumbersForIndices(params.atoms);
  auto pos = params.structure->atomicPositionsForIndices(params.atoms);

  // should be in angstroms
  for (int i = 0; i < nums.rows(); i++) {
    ts << pos(0, i) << " " << pos(1, i) << " " << pos(2, i) << " "
       << QString::fromStdString(occ::core::Element(nums(i)).symbol())
       << Qt::endl;
  }

  ts << "$gfn" << Qt::endl;
  int method = 2;
  switch (params.method) {
  default:
    break;
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
  ts << "$write" << Qt::endl;
  ts <<  QString("output file=%1_properties.txt").arg(params.name) << Qt::endl;
  ts << "json=true" << Qt::endl;
  ts << "$end" << Qt::endl;

  return QString(destination);
}

xtb::Result loadXtbResult(const xtb::Parameters &params,
                          const QString &jsonFilename,
                          const QString &moldenFilename) {
  xtb::Result result;

  {
    QFile file(jsonFilename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
      return result;

    result.jsonContents = file.readAll();
    file.close();
  }

  if (params.write_molden) {
    QFile file(moldenFilename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
      return result;

    result.moldenContents = file.readAll();
    file.close();
  }

  return result;
}
