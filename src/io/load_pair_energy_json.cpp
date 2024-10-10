#include "load_pair_energy_json.h"
#include "json.h"
#include <QFile>

PairInteraction *load_pair_energy_json(const QString &filename) {
  constexpr double hartree_to_kj_per_mol{2625.5};
  PairInteraction *result{nullptr};

  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Couldn't open pair energy json file:" << filename;
    return result;
  }
  QByteArray data = file.readAll();

  nlohmann::json doc;
  try {
    doc = nlohmann::json::parse(data.constData());
    // Extract interaction model
    QString interactionModel =
        doc.at("interaction_model").at("name").get<QString>();

    result = new PairInteraction(interactionModel);
    for (const auto &item : doc["interaction_energy"].items()) {
      double value = item.value().get<double>() * hartree_to_kj_per_mol;
      result->addComponent(QString::fromStdString(item.key()), value);
    }
  } catch (nlohmann::json::parse_error &e) {
    qWarning() << "JSON parse error:" << e.what();
    return result;
  }

  return result;
}
