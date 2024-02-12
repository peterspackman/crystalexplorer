#pragma once
#include "energydescription.h"
#include "fragmentpairinfo.h" // FragmentStyle
#include "graphics.h"         // SegmentedLine, RoundedLine
#include "settings.h"

enum FrameworkType {
  coulombFramework,
  dispersionFramework,
  totalFramework,
  annotatedTotalFramework
};

/////////////////////////////////////////////////////
// Cutoff Settings Keys
/////////////////////////////////////////////////////
inline QMap<FrameworkType, QString> getCutoffSettingsKeys() {
  QMap<FrameworkType, QString> map;
  map[coulombFramework] = settings::keys::ENERGY_FRAMEWORK_CUTOFF_COULOMB;
  map[dispersionFramework] = settings::keys::ENERGY_FRAMEWORK_CUTOFF_DISPERSION;
  map[totalFramework] = settings::keys::ENERGY_FRAMEWORK_CUTOFF_TOTAL;
  map[annotatedTotalFramework] = settings::keys::ENERGY_FRAMEWORK_CUTOFF_TOTAL;
  return map;
}

/////////////////////////////////////////////////////
// Cutoff Defaults
/////////////////////////////////////////////////////
inline QMap<FrameworkType, float> getCutoffDefaults() {
  QMap<FrameworkType, float> map;
  map[coulombFramework] =
      settings::readSetting(settings::keys::ENERGY_FRAMEWORK_CUTOFF_COULOMB)
          .toFloat();
  map[dispersionFramework] =
      settings::readSetting(settings::keys::ENERGY_FRAMEWORK_CUTOFF_DISPERSION)
          .toFloat();
  map[totalFramework] =
      settings::readSetting(settings::keys::ENERGY_FRAMEWORK_CUTOFF_TOTAL)
          .toFloat();
  map[annotatedTotalFramework] =
      settings::readSetting(settings::keys::ENERGY_FRAMEWORK_CUTOFF_TOTAL)
          .toFloat();
  return map;
}

/////////////////////////////////////////////////////
// Energy Types
/////////////////////////////////////////////////////
inline QMap<FrameworkType, EnergyType> getEnergyTypes() {
  QMap<FrameworkType, EnergyType> map;
  map[coulombFramework] = EnergyType::CoulombEnergy;
  map[dispersionFramework] = EnergyType::DispersionEnergy;
  map[totalFramework] = EnergyType::TotalEnergy;
  map[annotatedTotalFramework] = EnergyType::TotalEnergy;
  return map;
}

/////////////////////////////////////////////////////
// Show Framework Options Flags
/////////////////////////////////////////////////////
inline QMap<FrameworkType, bool> getShowScaleOptionsFlags() {
  QMap<FrameworkType, bool> map;
  map[coulombFramework] = true;
  map[dispersionFramework] = true;
  map[totalFramework] = true;
  map[annotatedTotalFramework] = false;
  return map;
}

/////////////////////////////////////////////////////
// Framework color
/////////////////////////////////////////////////////
inline QMap<FrameworkType, QColor> getFrameworkColors() {
  QMap<FrameworkType, QColor> map;
  map[coulombFramework] =
      settings::readSetting(settings::keys::CE_RED_COLOR).toString();
  map[dispersionFramework] =
      settings::readSetting(settings::keys::CE_GREEN_COLOR).toString();
  map[totalFramework] =
      settings::readSetting(settings::keys::CE_BLUE_COLOR).toString();
  map[annotatedTotalFramework] =
      settings::readSetting(settings::keys::CE_BLUE_COLOR).toString();
  return map;
}

/////////////////////////////////////////////////////
// Detailed Descriptions
/////////////////////////////////////////////////////
inline QMap<FrameworkType, QString> getDetailedDescriptions() {
  QMap<FrameworkType, QString> map;
  map[coulombFramework] = "Coulomb Energy";
  map[dispersionFramework] = "Dispersion Energy";
  map[totalFramework] = "Total Energy";
  map[annotatedTotalFramework] = "Total Energy (annotated)";
  return map;
}

/////////////////////////////////////////////////////
// Framework Style
/////////////////////////////////////////////////////
inline QMap<FrameworkType, FragmentPairStyle> getFrameworkStyles() {
  QMap<FrameworkType, FragmentPairStyle> map;
  map[coulombFramework] = FragmentPairStyle::RoundedLine;
  map[dispersionFramework] = FragmentPairStyle::RoundedLine;
  map[totalFramework] = FragmentPairStyle::RoundedLine;
  map[annotatedTotalFramework] = FragmentPairStyle::SegmentedLine;
  return map;
}

/////////////////////////////////////////////////////
// Annotate Frameworks Flag
/////////////////////////////////////////////////////
inline QMap<FrameworkType, bool> getAnnotateFrameworkFlags() {
  QMap<FrameworkType, bool> map;
  map[coulombFramework] = false;
  map[dispersionFramework] = false;
  map[totalFramework] = false;
  map[annotatedTotalFramework] = true;
  return map;
}
