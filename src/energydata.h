#pragma once
#include "energydescription.h"
#include <QMap>
#include <QSet>

const QVector<EnergyType> ENERGY_TYPES_TO_EXTRACT{
    EnergyType::CoulombEnergy,
    EnergyType::PolarizationEnergy,
    EnergyType::DispersionEnergy,
    EnergyType::RepulsionEnergy,

};

class EnergyData {
public:
  static QMap<EnergyType, double> getData(const QString &);
  static QMap<EnergyType, double> getOccData(const QString &);
  static QMap<EnergyType, double> getOrcaData(const QString &);
  static QMap<EnergyType, double> getXtbData(const QString &);

private:
  static QString tontoHookForEnergyType(EnergyType);
  static QString occHookForEnergyType(EnergyType);
  static QString orcaHookForEnergyType(EnergyType);
  static QString xtbHookForEnergyType(EnergyType);
};
