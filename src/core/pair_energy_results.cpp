#include "pair_energy_results.h"
#include "chemicalstructure.h"
#include <QDebug>

PairInteraction::PairInteraction(const QString &interactionModel,
                                 QObject *parent)
    : QObject(parent), m_interactionModel(interactionModel) {
  setObjectName(interactionModel);
}

void PairInteraction::addComponent(const QString &component, double value) {
  m_components.insert({component, value});
}

void PairInteraction::addMetadata(const QString &component,
                                  const QVariant &value) {
  m_metadata.insert({component, value});
}

double PairInteraction::getComponent(const QString &c) const {
  const auto kv = m_components.find(c);
  if (kv != m_components.end())
    return kv->second;
  return 0.0;
}

QVariant PairInteraction::getMetadata(const QString &c) const {
  const auto kv = m_metadata.find(c);
  if (kv != m_metadata.end())
    return kv->second;
  return QVariant();
}

QString PairInteraction::interactionModel() const { return m_interactionModel; }

void PairInteraction::setParameters(const pair_energy::Parameters &params) {
  m_parameters = params;
}
const pair_energy::Parameters &PairInteraction::parameters() const {
  return m_parameters;
}

PairInteractions::PairInteractions(QObject *parent) : QObject(parent) {
  setObjectName("Pair Interactions");
}

int PairInteractions::getCount(const QString &model) const {
  int result = 0;
  if (model.isEmpty()) {
    for (const auto &[k, v] : m_pairInteractions) {
      result += v.size();
    }
  } else {
    const auto kv = m_pairInteractions.find(model);
    if (kv != m_pairInteractions.end())
      return kv->second.size();
    return 0;
  }
  return result;
}

QStringList PairInteractions::interactionModels() const {
  QStringList result;
  for (const auto &[model, interactions] : m_pairInteractions) {
    result.append(model);
  }
  return result;
}

QStringList PairInteractions::interactionComponents(const QString &model) {

  const auto kv = m_pairInteractions.find(model);
  if (kv == m_pairInteractions.end())
    return {};

  QSet<QString> uniqueComponents;
  for (const auto &[index, interaction] : kv->second) {
    for (const auto &[component, value] : interaction->components()) {
      uniqueComponents.insert(component);
    }
  }
  return QStringList(uniqueComponents.begin(), uniqueComponents.end());
}

void PairInteractions::add(PairInteraction *result) {
  if (!result)
    return;
  QString model = result->interactionModel();
  if (result->label() == "Not set") {
    result->setLabel(QString("%1").arg(m_pairInteractions[model].size() + 1));
  }
  m_pairInteractions[model].insert({result->pairIndex(), result});
  impl::ValueRange currentRange;
  {
    const auto kv = m_distanceRange.find(model);
    if (kv != m_distanceRange.end()) {
      currentRange = kv->second;
    }
  }
  double d = result->nearestAtomDistance();
  qDebug() << "Nearest atomdistance" << d;
  m_distanceRange[model] = currentRange.update(d);
  emit interactionAdded();
}

PairInteractions::PairInteractionMap
PairInteractions::filterByModel(const QString &model) const {
  const auto kv = m_pairInteractions.find(model);
  if (kv == m_pairInteractions.end())
    return {};
  return kv->second;
}

void PairInteractions::remove(PairInteraction *result) {
  QString model = result->interactionModel();
  auto kv = m_pairInteractions.find(model);
  if (kv == m_pairInteractions.end())
    return;

  bool removeModel = false;
  {
    auto &interactions = kv->second;
    interactions.erase(result->pairIndex());
    removeModel = (interactions.size() == 0);
  }

  if (removeModel) {
    m_pairInteractions.erase(model);
    m_distanceRange.erase(model);
  } else {
    // Recalculate distance range for this model
    auto &interactions = kv->second;
    if (!interactions.empty()) {
      impl::ValueRange newRange;
      for (const auto &[pairIdx, interaction] : interactions) {
        double d = interaction->nearestAtomDistance();
        newRange = newRange.update(d);
      }
      m_distanceRange[model] = newRange;
      qDebug() << "Updated distance range for model" << model
               << "to [" << newRange.minValue << "," << newRange.maxValue << "]";
    }
  }

  emit interactionRemoved();
}

QList<PairInteraction *>
PairInteractions::filterByComponent(const QString &component) const {
  QList<PairInteraction *> filtereds;
  for (const auto &[model, interactions] : m_pairInteractions) {
    for (const auto &kv : interactions) {
      PairInteraction *result = kv.second;
      for (const auto &pair : result->components()) {
        if (pair.first == component) {
          filtereds.append(result);
          break;
        }
      }
    }
  }
  return filtereds;
}

QList<PairInteraction *>
PairInteractions::filterByModelAndComponent(const QString &model,
                                            const QString &component) const {
  QList<PairInteraction *> filtereds;
  const auto interactions = m_pairInteractions.find(model);
  if (interactions == m_pairInteractions.end())
    return filtereds;

  for (const auto kv : interactions->second) {
    PairInteraction *result = kv.second;
    for (const auto &pair : result->components()) {
      if (pair.first == component) {
        filtereds.append(result);
        break;
      }
    }
  }
  return filtereds;
}

QMap<QString, PairInteractions::PairInteractionList>
PairInteractions::getInteractionsMatchingFragments(
    const std::vector<FragmentDimer> &dimers) {
  const auto models = interactionModels();
  QMap<QString, PairInteractions::PairInteractionList> result;
  for (const auto &model : models) {
    double maxDistance = 0.0;
    const auto kv = m_distanceRange.find(model);
    if (kv != m_distanceRange.end())
      maxDistance = kv->second.maxValue;

    QList<PairInteraction *> l;
    for (const auto &dimer : dimers) {
      if (dimer.nearestAtomDistance > maxDistance) {
        l.append(nullptr);
      } else {
        l.append(getInteraction(model, dimer));
      }
    }
    result[model] = l;
  }
  return result;
}

PairInteraction *PairInteractions::getInteraction(const QString &model,
                                                  const FragmentDimer &frag) {
  const auto kv = m_pairInteractions.find(model);
  if (kv == m_pairInteractions.end())
    return nullptr;

  const auto &interactions = kv->second;

  const auto result = interactions.find(frag.index);
  if (result == interactions.end())
    return nullptr;

  return result->second;
}

void PairInteractions::resetCounts() {
  const auto models = interactionModels();
  for (auto &[model, interactions] : m_pairInteractions) {
    for (auto &[idx, interaction] : interactions) {
      interaction->setCount(0);
    }
  }
}

void PairInteractions::resetColors() {
  const auto models = interactionModels();
  for (auto &[model, interactions] : m_pairInteractions) {
    for (auto &[idx, interaction] : interactions) {
      interaction->setColor(Qt::white);
    }
  }
}

bool PairInteractions::haveInteractions(const QString &model) const {
  return getCount(model) > 0;
}

bool PairInteractions::hasPermutationSymmetry(const QString &model) const {
  if (model.isEmpty()) {
    for (const auto &[k, v] : m_pairInteractions) {
      for (const auto &[interaction_key, interaction] : v) {
        if (!interaction->parameters().hasPermutationSymmetry)
          return false;
      }
    }
  } else {
    const auto kv = m_pairInteractions.find(model);
    if (kv != m_pairInteractions.end()) {
      for (const auto &[interaction_key, interaction] : kv->second) {
        if (!interaction->parameters().hasPermutationSymmetry)
          return false;
      }
    }
  }
  return true;
}

nlohmann::json PairInteraction::toJson() const {
  nlohmann::json j;

  // Basic properties
  j["interactionModel"] = m_interactionModel;
  j["label"] = m_label;
  j["color"] = m_color;
  j["count"] = m_count;

  // Energy components
  nlohmann::json componentsJson = nlohmann::json::object();
  for (const auto &[component, value] : m_components) {
    componentsJson[component.toStdString()] = value;
  }
  j["components"] = componentsJson;

  // Metadata
  nlohmann::json metadataJson = nlohmann::json::object();
  for (const auto &[key, value] : m_metadata) {
    metadataJson[key.toStdString()] = value;
  }
  j["metadata"] = metadataJson;

  nlohmann::json paramsJson;
  to_json(paramsJson, m_parameters);
  // Parameters
  j["parameters"] = paramsJson;

  return j;
}

PairInteraction *PairInteraction::fromJson(const nlohmann::json &j,
                                           QObject *parent) {
  try {
    QString model = j.at("interactionModel").get<QString>();

    // Create the interaction
    PairInteraction *interaction = new PairInteraction(model, parent);

    if (j.contains("label")) {
      interaction->setLabel(j.at("label").get<QString>());
    }

    if (j.contains("color")) {
      QColor color = j.at("color").get<QColor>();
      interaction->setColor(color);
    }

    if (j.contains("count")) {
      interaction->setCount(j.at("count").get<int>());
    }

    // Set components
    if (j.contains("components")) {
      const auto &componentsJson = j.at("components");
      for (auto it = componentsJson.begin(); it != componentsJson.end(); ++it) {
        QString key = QString::fromStdString(it.key());
        double value = it.value().get<double>();
        interaction->addComponent(key, value);
      }
    }

    // Set metadata
    if (j.contains("metadata")) {
      const auto &metadataJson = j.at("metadata");
      for (auto it = metadataJson.begin(); it != metadataJson.end(); ++it) {
        QString key = QString::fromStdString(it.key());
        QVariant value;

        if (it.value().is_number_float()) {
          value = QVariant(it.value().get<double>());
        } else if (it.value().is_number_integer()) {
          value = QVariant(it.value().get<int>());
        } else if (it.value().is_boolean()) {
          value = QVariant(it.value().get<bool>());
        } else if (it.value().is_string()) {
          value = QVariant(it.value().get<QString>());
        }
        interaction->addMetadata(key, value);
      }
    }

    // Set parameters
    if (j.contains("parameters")) {
      const auto &paramsJson = j.at("parameters");
      pair_energy::Parameters params;
      from_json(j.at("parameters"), params);
      interaction->setParameters(params);
    }

    return interaction;
  } catch (const std::exception &e) {
    qDebug() << "Failed to deserialize PairInteraction: " << e.what();
    return nullptr;
  }
}

nlohmann::json PairInteractions::toJson() const {
  nlohmann::json j;

  // Serialize interactions by model
  nlohmann::json interactionsJson = nlohmann::json::object();
  for (const auto &[model, interactions] : m_pairInteractions) {
    nlohmann::json modelInteractionsJson = nlohmann::json::array();
    for (const auto &[index, interaction] : interactions) {
      modelInteractionsJson.push_back(interaction->toJson());
    }
    interactionsJson[model.toStdString()] = modelInteractionsJson;
  }
  j["interactions"] = interactionsJson;

  return j;
}

bool PairInteractions::fromJson(const nlohmann::json &j) {
  ChemicalStructure *p = qobject_cast<ChemicalStructure *>(parent());
  try {
    // Clear existing data first
    for (auto &[model, interactions] : m_pairInteractions) {
      for (auto &[index, interaction] : interactions) {
        delete interaction;
      }
    }
    m_pairInteractions.clear();
    m_distanceRange.clear();

    // Deserialize interactions
    if (j.contains("interactions")) {
      const auto &interactionsJson = j.at("interactions");
      for (auto it = interactionsJson.begin(); it != interactionsJson.end();
           ++it) {
        QString model = QString::fromStdString(it.key());
        const auto &modelInteractionsJson = it.value();

        for (const auto &interactionJson : modelInteractionsJson) {
          PairInteraction *interaction =
              PairInteraction::fromJson(interactionJson, this);
          if (interaction) {
            // Add to map - use standard add method to maintain internal state
            add(interaction);
          }
        }
      }
    }

    return true;
  } catch (const std::exception &e) {
    qDebug() << "Failed to deserialize PairInteractions: " << e.what();
    return false;
  }
}
