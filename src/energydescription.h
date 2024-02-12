#pragma once
#include "energytype.h"
#include "jobparameters.h"
#include "wavefunction.h" // For levelOfTheoryString

// Scale factors were updated on 23/01/16
// Scaling based on model using D2 dispersion with damping
// from forthcoming paper

enum EnergyModel { None, CE_HF, CE_B3LYP, DLPNO, DFTB };

struct MonomerEnergy {
  QMap<EnergyType, double> energies;
  JobParameters jobParams;
};

typedef QPair<Method, BasisSet> EnergyTheory;

static QMap<EnergyModel, EnergyTheory> getEnergyTheories() {
  QMap<EnergyModel, EnergyTheory> map;
  map[EnergyModel::None] =
      EnergyTheory(Method::hartreeFock,
                   BasisSet::STO_3G); // Junk value (simplest level of theory)
  map[EnergyModel::CE_HF] =
      EnergyTheory(Method::hartreeFock, BasisSet::Pople3_21G);
  map[EnergyModel::CE_B3LYP] =
      EnergyTheory(Method::b3lyp, BasisSet::Pople6_31Gdp);
  return map;
}
const QMap<EnergyModel, EnergyTheory> energyTheories = getEnergyTheories();

static QMap<EnergyType, QString> getEnergyNames() {
  QMap<EnergyType, QString> map;

  map[EnergyType::CoulombEnergy] = "E_ele";
  map[EnergyType::PolarizationEnergy] = "E_pol";
  map[EnergyType::DispersionEnergy] = "E_dis";
  map[EnergyType::RepulsionEnergy] = "E_rep";
  map[EnergyType::TotalEnergy] = "E_tot";
  map[EnergyType::AnisotropicElectrostaticEnergy] = "E_ele_aniso";
  map[EnergyType::IsotropicElectrostaticEnergy] = "E_ele_iso";

  return map;
}
const QMap<EnergyType, QString> energyNames = getEnergyNames();

static QMap<EnergyModel, float> getCoulombScaleFactors() {
  QMap<EnergyModel, float> map;

  map[EnergyModel::None] = 1.0f;
  map[EnergyModel::CE_HF] = 1.0189f;
  map[EnergyModel::CE_B3LYP] = 1.0573f;
  map[EnergyModel::DFTB] = 1.0f;
  map[EnergyModel::DLPNO] = 1.0f;

  return map;
}
const QMap<EnergyModel, float> coulombScaleFactors = getCoulombScaleFactors();

static QMap<EnergyModel, float> getPolarizationScaleFactors() {
  QMap<EnergyModel, float> map;

  map[EnergyModel::None] = 1.0f;
  map[EnergyModel::CE_HF] = 0.6506f;
  map[EnergyModel::CE_B3LYP] = 0.7399f;
  map[EnergyModel::DFTB] = 1.0f;
  map[EnergyModel::DLPNO] = 1.0f;

  return map;
}
const QMap<EnergyModel, float> polarizationScaleFactors =
    getPolarizationScaleFactors();

static QMap<EnergyModel, float> getDispersionScaleFactors() {
  QMap<EnergyModel, float> map;

  map[EnergyModel::None] = 1.0f;
  map[EnergyModel::CE_HF] = 0.9011f;
  map[EnergyModel::CE_B3LYP] = 0.8708f;
  map[EnergyModel::DFTB] = 1.0f;
  map[EnergyModel::DLPNO] = 1.0f;

  return map;
}
const QMap<EnergyModel, float> dispersionScaleFactors =
    getDispersionScaleFactors();

static QMap<EnergyModel, float> getRepulsionScaleFactors() {
  QMap<EnergyModel, float> map;

  map[EnergyModel::None] = 1.0f;
  map[EnergyModel::CE_HF] = 0.8109f;
  map[EnergyModel::CE_B3LYP] = 0.6177f;
  map[EnergyModel::DFTB] = 1.0f;
  map[EnergyModel::DLPNO] = 1.0f;

  return map;
}
const QMap<EnergyModel, float> repulsionScaleFactors =
    getRepulsionScaleFactors();

class EnergyDescription {
public:
  static EnergyModel quantitativeEnergyModel() { return EnergyModel::CE_B3LYP; }
  static EnergyModel qualitativeEnergyModel() { return EnergyModel::CE_HF; }

  static Method quantitativeEnergyModelTheory() {
    return energyTheories[quantitativeEnergyModel()].first;
  }
  static Method qualitativeEnergyModelTheory() {
    return energyTheories[qualitativeEnergyModel()].first;
  }

  static BasisSet quantitativeEnergyModelBasisset() {
    return energyTheories[quantitativeEnergyModel()].second;
  }
  static BasisSet qualitativeEnergyModelBasisset() {
    return energyTheories[qualitativeEnergyModel()].second;
  }

  static QString quantitativeEnergyModelDescription() {
    return description(quantitativeEnergyModel());
  }
  static QString qualitativeEnergyModelDescription() {
    return description(qualitativeEnergyModel());
  }
  static QString description(EnergyModel model) {
    return Wavefunction::levelOfTheoryString(energyTheories[model].first,
                                             energyTheories[model].second);
  }
  static QString fullDescription(EnergyModel model) {
    return QString("CE-") +
           Wavefunction::methodString(energyTheories[model].first) + " ... " +
           Wavefunction::levelOfTheoryString(energyTheories[model].first,
                                             energyTheories[model].second) +
           " electron densities";
  }
};
