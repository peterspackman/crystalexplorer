#include "pair_energy_results.h"
#include <QDebug>

PairInteraction::PairInteraction(const QString &interactionModel,
                                 QObject *parent)
    : QObject(parent), m_interactionModel(interactionModel) {
  setObjectName(interactionModel);
}

void PairInteraction::addComponent(const QString &component, double value) {
  m_components.insert({component, value});
}

double PairInteraction::getComponent(const QString &c) const {
    const auto kv = m_components.find(c);
    if(kv != m_components.end()) return kv->second;
    return 0.0;
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
    for (const auto &l : m_pairInteractions.values()) {
      result += l.size();
    }
  } else {
    result = m_pairInteractions.value(model, {}).size();
  }
  return result;
}

QList<QString> PairInteractions::interactionModels() const {
  return m_pairInteractions.keys();
}

QList<QString> PairInteractions::interactionComponents(const QString &model) {
  QSet<QString> uniqueComponents;
  for (auto *result : m_pairInteractions.value(model, {})) {
    for (const auto &[component, value] : result->components()) {
      uniqueComponents.insert(component);
    }
  }
  return QList<QString>(uniqueComponents.begin(), uniqueComponents.end());
}

void PairInteractions::add(PairInteraction *result) {
  if (!result)
    return;
  qDebug() << "Adding interaction" << result;
  QString model = result->interactionModel();
  m_pairInteractions[model].append(result);
  m_maxNearestDistance[model] =
      qMax(m_maxNearestDistance.value(model), result->nearestAtomDistance());
  emit interactionAdded();
}

QList<PairInteraction *>
PairInteractions::filterByModel(const QString &model) const {
  return m_pairInteractions.value(model);
}

void PairInteractions::remove(PairInteraction *result) {
  // TODO update maxDistance
  QString model = result->interactionModel();
  QList<PairInteraction *> &models = m_pairInteractions[model];
  int index = models.indexOf(result);
  if (index >= 0) {
    models.removeAt(index);
    if (models.isEmpty()) {
      m_pairInteractions.remove(model);
    }
    emit interactionRemoved();
  }
}

QList<PairInteraction *>
PairInteractions::filterByComponent(const QString &component) const {
  QList<PairInteraction *> filtereds;
  for (const auto &models : m_pairInteractions) {
    for (PairInteraction *result : models) {
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
  const auto &interactions = m_pairInteractions.value(model);
  for (PairInteraction *result : interactions) {
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
  QMap<QString, PairInteractionList> result;
  for (const auto &model : models) {
    double maxDistance = m_maxNearestDistance.value(model, 0.0);
    PairInteractionList l;
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
  const auto &interactions = m_pairInteractions.value(model);
  for (auto *result : interactions) {
    if (!result) {
      qWarning() << "Null pointer in stored pair interactions results... "
                    "should not happen";
      continue;
    }
    const auto &params = result->parameters();
    if (params.fragmentDimer == frag)
      return result;
  }
  return nullptr;
}
