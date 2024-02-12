#pragma once
#include <QDebug>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

#include "colorschemer.h"
#include "globals.h"

class IsosurfacePropertyDetails {
public:
  enum class Type {
    None,
    DistanceInternal,
    DistanceExternal,
    DistanceNorm,
    ShapeIndex,
    Curvedness,
    PromoleculeDensity,
    ElectronDensity,
    DeformationDensity,
    ElectricPotential,
    Orbital,
    SpinDensity,
    /*,
    localIonisationEnergyProp,
    localElectronAffinityProp
    */
    FragmentPatch,
    Domain,
    Unknown
  };

  struct Attributes {
    // Attributes ...
    ColorScheme colorScheme{ColorScheme::noneColor};
    QString name{""};
    QString tontoName{""};
    QString unit{""};
    bool needsWavefunction{false};
    bool needsIsovalue{false};
    bool needsOrbitals{false};
    QString description{""};
  };

  inline static Type defaultType() { return Type::None; }
  static Attributes getAttributes(Type t);
  static Type typeFromTontoName(const QString &);
  static inline const auto &getAvailableTypes() { return availableTypes; }

private:
  static const QMap<Type, Attributes> availableTypes;
  IsosurfacePropertyDetails() = delete;
};

class IsosurfaceDetails {
public:
  enum class Type {
    Hirshfeld,
    CrystalVoid,
    PromoleculeDensity,
    ElectronDensity,
    DeformationDensity,
    ElectricPotential,
    Orbital,
    ADP,
    SpinDensity,
    Unknown
  };

  struct Attributes {
    QString label{""};
    QString tontoLabel{""};
    bool needsWavefunction{false};
    bool needsIsovalue{false};
    bool needsOrbitals{false};
    bool needsClusterOptions{false};
    float defaultIsovalue{0.0f};
    QString description{""};
  };

  inline static Type defaultType() { return Type::Hirshfeld; }
  static Attributes getAttributes(Type t);
  static const QList<IsosurfacePropertyDetails::Type> &
  getRequestableProperties(Type t);
  static inline const auto &getAvailableTypes() { return availableTypes; }

private:
  static const QMap<Type, Attributes> availableTypes;
  static const QMap<Type, QList<IsosurfacePropertyDetails::Type>>
      requestableProperties;
  static const QStringList resolutionLevels;
  IsosurfaceDetails() = delete;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Resolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ResolutionDetails {
public:
    enum class Level {
        VeryLow,
        Low,
        Medium,
        High,
        VeryHigh
    };

    static float value(Level);
    static const char* name(Level);
    static inline Level defaultLevel() { return Level::High; }
    static const auto &getLevels() { return levels; }
private:
    ResolutionDetails() = delete;
    static const QList<Level> levels;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Orbital Types
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum OrbitalType { HOMO, LUMO };
const OrbitalType defaultOrbitalType = HOMO;
const QStringList orbitalLabels = QStringList() << "HOMO"
                                                << "LUMO";

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Property Statistics Type
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum PropertyStatisticsType {
  MeanPlus,
  MeanMinus,
  PiStat,
  SigmaPlus,
  SigmaMinus,
  SigmaT,
  NuStat
};

static QMap<PropertyStatisticsType, QString> getPropertyStatisticsNames() {
  QMap<PropertyStatisticsType, QString> map;

  map[MeanPlus] = "Mean+";
  map[MeanMinus] = "Mean-";
  map[PiStat] = "Pi";
  map[SigmaPlus] = "Sigma+";
  map[SigmaMinus] = "Sigma-";
  map[SigmaT] = "SigmaT";
  map[NuStat] = "Nu";

  return map;
}
const QMap<PropertyStatisticsType, QString> propertyStatisticsNames =
    getPropertyStatisticsNames();
