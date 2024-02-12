#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QtDebug>
#include <QRegularExpression>

#include "energydata.h"
#include "mathconstants.h"

QMap<EnergyType, double> EnergyData::getData(const QString &filename) {
  QMap<EnergyType, double> energies;

  QVector<EnergyType> energyTypesToExtract = ENERGY_TYPES_TO_EXTRACT;

  QFile file(filename);
  if (file.open(QIODevice::ReadOnly)) {

    QTextStream ts(&file);

    while (!ts.atEnd()) {
      QString line = ts.readLine();
      if (energyTypesToExtract.size() > 0) {

        foreach (EnergyType requestedEnergyType, energyTypesToExtract) {
          if (line.startsWith(tontoHookForEnergyType(requestedEnergyType),
                              Qt::CaseSensitive)) {
            QStringList tokens = line.split(QRegularExpression("\\s+"));
            Q_ASSERT(tokens.size() >
                     2); // Should be at least 3 tokens: "xxx ..... value"
            energies[requestedEnergyType] = tokens.last().toDouble();
            energyTypesToExtract.removeOne(requestedEnergyType);
          }
        }
      }
    }
  }

  // Put dummy values in if not found Tonto output
  if (!energies.contains(EnergyType::CoulombEnergy)) {
    energies[EnergyType::CoulombEnergy] = 0.0;
  }
  if (!energies.contains(EnergyType::PolarizationEnergy)) {
    energies[EnergyType::PolarizationEnergy] = 0.0;
  }
  if (!energies.contains(EnergyType::DispersionEnergy)) {
    energies[EnergyType::DispersionEnergy] = 0.0;
  }
  if (!energies.contains(EnergyType::RepulsionEnergy)) {
    energies[EnergyType::RepulsionEnergy] = 0.0;
  }

  energies[EnergyType::TotalEnergy] = 0.0;

  return energies;
}

QString EnergyData::tontoHookForEnergyType(EnergyType energyType) {
  QString hook;
  switch (energyType) {
  case EnergyType::CoulombEnergy:
    hook = "Delta E_coul (kJ/mol)";
    break;
  case EnergyType::PolarizationEnergy:
    hook = "Polarization energy (kJ/mol)";
    break;
  case EnergyType::DispersionEnergy:
    hook = "Grimme06 dispersion energy (kJ/mol)";
    break;
  case EnergyType::RepulsionEnergy:
    hook = "Delta E_exch-rep (kJ/mol)";
    break;
  default: // The total energy never comes from Tonto so
           // there isn't a hook for it
    Q_ASSERT(false);
  }
  return hook;
}

QMap<EnergyType, double> EnergyData::getOccData(const QString &filename) {
  QMap<EnergyType, double> energies;

  QVector<EnergyType> energyTypesToExtract = ENERGY_TYPES_TO_EXTRACT;

  QFile file(filename);
  if (file.open(QIODevice::ReadOnly)) {
    QTextStream ts(&file);

    while (!ts.atEnd()) {
      QString line = ts.readLine();
      if (energyTypesToExtract.size() == 0)
        break;
      for (const auto &requestedEnergyType : energyTypesToExtract) {
        if (line.startsWith(occHookForEnergyType(requestedEnergyType),
                            Qt::CaseSensitive)) {
          QStringList tokens = line.split(QRegularExpression("\\s+"));
          Q_ASSERT(tokens.size() >
                   1); // Should be at least 2 tokens: "xxx value"
          energies[requestedEnergyType] = tokens.last().toDouble();
          energyTypesToExtract.removeOne(requestedEnergyType);
        }
      }
    }
  }

  // Put dummy values in if not found Tonto output
  if (!energies.contains(EnergyType::CoulombEnergy)) {
    energies[EnergyType::CoulombEnergy] = 0.0;
  }
  if (!energies.contains(EnergyType::PolarizationEnergy)) {
    energies[EnergyType::PolarizationEnergy] = 0.0;
  }
  if (!energies.contains(EnergyType::DispersionEnergy)) {
    energies[EnergyType::DispersionEnergy] = 0.0;
  }
  if (!energies.contains(EnergyType::RepulsionEnergy)) {
    energies[EnergyType::RepulsionEnergy] = 0.0;
  }

  energies[EnergyType::TotalEnergy] = 0.0;

  return energies;
}

QString EnergyData::occHookForEnergyType(EnergyType energyType) {
  QString hook;
  switch (energyType) {
  case EnergyType::CoulombEnergy:
    hook = "Coulomb";
    break;
  case EnergyType::PolarizationEnergy:
    hook = "Polarization";
    break;
  case EnergyType::DispersionEnergy:
    hook = "Dispersion";
    break;
  case EnergyType::RepulsionEnergy:
    hook = "Exchange-repulsion";
    break;
  case EnergyType::TotalEnergy:
    hook = "Total";
    break;
  default:
    Q_ASSERT(false);
  }
  return hook;
}

QMap<EnergyType, double> EnergyData::getOrcaData(const QString &filename) {
  QMap<EnergyType, double> energies;

  QVector<EnergyType> energyTypesToExtract = ENERGY_TYPES_TO_EXTRACT;
  energyTypesToExtract.push_back(EnergyType::TotalEnergy);

  QFile file(filename);
  if (file.open(QIODevice::ReadOnly)) {
    QTextStream ts(&file);

    while (!ts.atEnd()) {
      QString line = ts.readLine();
      if (energyTypesToExtract.size() == 0)
        break;
      for (const auto &requestedEnergyType : energyTypesToExtract) {
        if (line.startsWith(orcaHookForEnergyType(requestedEnergyType),
                            Qt::CaseSensitive)) {
          QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
          Q_ASSERT(tokens.size() >
                   1); // Should be at least 2 tokens: "xxx value"
          qDebug() << "Read energy: " << tokens.last().toDouble() << "Eh";
          energies[requestedEnergyType] =
              tokens.last().toDouble() * KJMOL_PER_HARTREE;
          qDebug() << "After conversion: " << energies[requestedEnergyType];
          energyTypesToExtract.removeOne(requestedEnergyType);
        }
      }
    }
  }

  // Put dummy values in if not found Tonto output
  if (!energies.contains(EnergyType::CoulombEnergy)) {
    energies[EnergyType::CoulombEnergy] = 0.0;
  }
  if (!energies.contains(EnergyType::PolarizationEnergy)) {
    energies[EnergyType::PolarizationEnergy] = 0.0;
  }
  if (!energies.contains(EnergyType::DispersionEnergy)) {
    energies[EnergyType::DispersionEnergy] = 0.0;
  }
  if (!energies.contains(EnergyType::RepulsionEnergy)) {
    energies[EnergyType::RepulsionEnergy] = 0.0;
  }

  if (!energies.contains(EnergyType::TotalEnergy)) {
    energies[EnergyType::TotalEnergy] = 0.0;
  }
  return energies;
}

QString EnergyData::orcaHookForEnergyType(EnergyType energyType) {
  QString hook;
  switch (energyType) {
  case EnergyType::CoulombEnergy:
    hook = "Electrostatics (REF.)";
    break;
  case EnergyType::PolarizationEnergy:
    hook = "Dispersion (weak pairs)";
    break;
  case EnergyType::DispersionEnergy:
    hook = "Dispersion (strong pairs)";
    break;
  case EnergyType::RepulsionEnergy:
    hook = "Exchange (REF.)";
    break;
  case EnergyType::TotalEnergy:
    hook = "Sum of INTER-fragment total energies";
    break;
  default:
    Q_ASSERT(false);
  }
  return hook;
}

QMap<EnergyType, double> EnergyData::getXtbData(const QString &filename) {
  QMap<EnergyType, double> energies;

  QVector<EnergyType> energyTypesToExtract = {
      EnergyType::AnisotropicElectrostaticEnergy,
      EnergyType::IsotropicElectrostaticEnergy, EnergyType::DispersionEnergy,
      EnergyType::TotalEnergy};
  energyTypesToExtract.push_back(EnergyType::TotalEnergy);

  QFile file(filename);
  if (file.open(QIODevice::ReadOnly)) {
    QTextStream ts(&file);

    while (!ts.atEnd()) {
      QString line = ts.readLine();
      if (energyTypesToExtract.size() == 0)
        break;
      for (const auto &requestedEnergyType : energyTypesToExtract) {
        if (line.contains(xtbHookForEnergyType(requestedEnergyType),
                          Qt::CaseSensitive)) {
          QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
          Q_ASSERT(tokens.size() >
                   1); // Should be at least 2 tokens: "xxx value"
          energies[requestedEnergyType] =
              tokens.at(tokens.size() - 3).toDouble() * KJMOL_PER_HARTREE;
          energyTypesToExtract.removeOne(requestedEnergyType);
        }
      }
    }
  }

  // Put dummy values in if not found XTB output
  if (!energies.contains(EnergyType::AnisotropicElectrostaticEnergy)) {
    energies[EnergyType::AnisotropicElectrostaticEnergy] = 0.0;
  }
  if (!energies.contains(EnergyType::IsotropicElectrostaticEnergy)) {
    energies[EnergyType::IsotropicElectrostaticEnergy] = 0.0;
  }
  if (!energies.contains(EnergyType::CoulombEnergy)) {
    energies[EnergyType::CoulombEnergy] =
        energies[EnergyType::AnisotropicElectrostaticEnergy] +
        energies[EnergyType::IsotropicElectrostaticEnergy];
  }
  if (!energies.contains(EnergyType::PolarizationEnergy)) {
    energies[EnergyType::PolarizationEnergy] = 0.0;
  }
  if (!energies.contains(EnergyType::DispersionEnergy)) {
    energies[EnergyType::DispersionEnergy] = 0.0;
  }
  if (!energies.contains(EnergyType::RepulsionEnergy)) {
    energies[EnergyType::RepulsionEnergy] = 0.0;
  }

  if (!energies.contains(EnergyType::TotalEnergy)) {
    energies[EnergyType::TotalEnergy] = 0.0;
  }
  return energies;
}

QString EnergyData::xtbHookForEnergyType(EnergyType energyType) {
  QString hook;
  switch (energyType) {
  case EnergyType::IsotropicElectrostaticEnergy:
    hook = "-> isotropic ES";
    break;
  case EnergyType::AnisotropicElectrostaticEnergy:
    hook = "-> anisotropic ES";
    break;
  case EnergyType::DispersionEnergy:
    hook = "-> dispersion";
    break;
  case EnergyType::TotalEnergy:
    hook = "total energy";
    break;
  default:
    Q_ASSERT(false);
  }
  return hook;
}
