#pragma once
#include "json.h"
#include "pair_energy_parameters.h"
#include <QColor>
#include <QMap>
#include <QObject>
#include <QVariant>
#include <ankerl/unordered_dense.h>
#include <occ/crystal/dimer_mapping_table.h>

class PairInteraction : public QObject {
  Q_OBJECT
public:
  using EnergyComponents = ankerl::unordered_dense::map<QString, double>;
  using Metadata = ankerl::unordered_dense::map<QString, QVariant>;

  explicit PairInteraction(const QString &interactionModel,
                           QObject *parent = nullptr);
  QString interactionModel() const;
  void addComponent(const QString &component, double value);
  inline const auto &components() const { return m_components; }
  double getComponent(const QString &) const;

  void addMetadata(const QString &label, const QVariant &value);
  inline const auto &metadata() const { return m_metadata; }
  QVariant getMetadata(const QString &) const;

  inline const QString &symmetry() const {
    return m_parameters.fragmentDimer.symmetry;
  }
  inline double nearestAtomDistance() const {
    return m_parameters.fragmentDimer.nearestAtomDistance;
  }
  inline double centroidDistance() const {
    return m_parameters.fragmentDimer.centroidDistance;
  }

  inline const auto &label() const { return m_label; }
  inline void setLabel(const QString &label) { m_label = label; }

  inline const auto &color() const { return m_color; }
  inline void setColor(QColor color) { m_color = color; }
  inline int count() const { return m_count; }
  inline void setCount(int c) { m_count = c; }

  void setParameters(const pair_energy::Parameters &);
  const pair_energy::Parameters &parameters() const;
  inline const auto &pairIndex() const {
    return m_parameters.fragmentDimer.index;
  }
  inline QString dimerDescription() const {
    return m_parameters.fragmentDimer.getName();
  }

  nlohmann::json toJson() const;
  static PairInteraction *fromJson(const nlohmann::json &j,
                                   QObject *parent = nullptr);

private:
  int m_count{0};
  QColor m_color{Qt::white};
  QString m_label{"Not set"};
  QString m_interactionModel;
  EnergyComponents m_components;
  Metadata m_metadata;
  pair_energy::Parameters m_parameters;
};

namespace impl {
struct ValueRange {
  double minValue{std::numeric_limits<double>::max()};
  double maxValue{std::numeric_limits<double>::min()};

  inline ValueRange merge(ValueRange rhs) const {
    return ValueRange{qMin(minValue, rhs.minValue),
                      qMax(maxValue, rhs.maxValue)};
  }
  inline ValueRange update(double v) const {
    return ValueRange{qMin(minValue, v), qMax(maxValue, v)};
  }
};
} // namespace impl

class PairInteractions : public QObject {
  Q_OBJECT
public:
  using PairInteractionList = QList<PairInteraction *>;
  using PairInteractionMap =
      ankerl::unordered_dense::map<FragmentIndexPair, PairInteraction *,
                                   FragmentIndexPairHash>;
  using ModelInteractions =
      ankerl::unordered_dense::map<QString, PairInteractionMap>;

  explicit PairInteractions(QObject *parent = nullptr);

  void add(PairInteraction *result);
  void remove(PairInteraction *result);

  void resetCounts();
  void resetColors();

  QMap<QString, PairInteractionList>
  getInteractionsMatchingFragments(const std::vector<FragmentDimer> &frag);

  PairInteraction *getInteraction(const QString &model,
                                  const FragmentDimer &frag);

  QStringList interactionModels() const;
  QStringList interactionComponents(const QString &model);
  PairInteractionMap filterByModel(const QString &model) const;
  QList<PairInteraction *> filterByComponent(const QString &component) const;
  QList<PairInteraction *>
  filterByModelAndComponent(const QString &model,
                            const QString &component) const;

  int getCount(const QString &model = "") const;
  bool haveInteractions(const QString &model = "") const;
  bool hasPermutationSymmetry(const QString &model = "") const;

  nlohmann::json toJson() const;
  bool fromJson(const nlohmann::json &j);

signals:
  void interactionAdded();
  void interactionRemoved();

private:
  ModelInteractions m_pairInteractions;
  ankerl::unordered_dense::map<QString, impl::ValueRange> m_distanceRange;
  bool m_haveDimerMap{false};
  occ::crystal::DimerMappingTable m_dimerMappingTable;
};
