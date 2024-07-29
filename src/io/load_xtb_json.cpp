#include "load_xtb_json.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

xtb::Result load_xtb_json(const QString &filename) {
  constexpr double hartree_to_kj_per_mol{2625.5};
  xtb::Result result;

  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return result;

  result.jsonContents = file.readAll();
  file.close();

  return result;
}
