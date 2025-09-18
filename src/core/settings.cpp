#include "settings.h"
#include "globals.h"

#include <QMap>
#include <QSettings>
#include <QtDebug>

namespace settings {

static const QMap<QString, QVariant> defaults{
    // Gaussian, NWChem, Psi4, occ
    {keys::GAUSSIAN_EXECUTABLE, ""},
    {keys::GAUSSIAN_MDEF, ""},
    {keys::GAUSSIAN_PDEF, ""},
    {keys::GAUSSIAN_EXECUTABLE_NAMES, QStringList{"g16", "g09"}},
    {keys::PRELOAD_MESH_FILES, true},

    {keys::NWCHEM_EXECUTABLE, ""},
    {keys::PSI4_EXECUTABLE, ""},
    {keys::ORCA_EXECUTABLE, ""},
    {keys::ORCA_NTHREADS, 1},
    {keys::XTB_EXECUTABLE, ""},
    {keys::OCC_NTHREADS, 1},
    {keys::OCC_EXECUTABLE, ""},
    {keys::OCC_DATA_DIRECTORY, ""},
    {keys::XH_NORMALIZATION, false},
    {keys::CH_BOND_LENGTH, 1.083f},
    {keys::NH_BOND_LENGTH, 1.009f},
    {keys::OH_BOND_LENGTH, 0.983f},
    {keys::BH_BOND_LENGTH, 1.180f},
    {keys::PREFERRED_WAVEFUNCTION_SOURCE, "occ"},
    // CrystalExplorer settings
    {keys::EXECUTABLE, "CrystalExplorer"},
    {keys::ELEMENTDATA_FILE, GLOBAL_ELEMENTDATA_FILE},
    {keys::USE_JMOL_COLORS, false},
    {keys::DELETE_WORKING_FILES, true},
    {keys::AUTOLOAD_LAST_FILE, false},
    {keys::BACKGROUND_COLOR, "white"},
    {keys::NONE_PROPERTY_COLOR, "#e6cdcd"}, // string rep of rgb, 230,205,0
    {keys::ATOM_LABEL_COLOR, "black"},
    {keys::BOND_THICKNESS, 30}, // percent of hydrogen covalent radius
    {keys::CONTACT_LINE_THICKNESS, 30},
    {keys::RESET_ELEMENTS_ELEMENTDATATXTFILE, true},
    // Close contacts
    {keys::HBOND_COLOR, "#009600"},    // RGB(0,150,0)
    {keys::CONTACT1_COLOR, "#cf423c"}, // RGB(207,66,60)
    {keys::CONTACT2_COLOR, "#fc7d49"}, // RGB(252,125,73)
    {keys::CONTACT3_COLOR, "#ffd462"}, // RGB(255,212,98)
    {keys::CE_RED_COLOR, QColor("#CC0000")},
    {keys::CE_GREEN_COLOR, QColor("#00CC00")},
    {keys::CE_BLUE_COLOR, QColor("#0000CC")},
    // OpenGL defaults
    {keys::SURFACE_DEPTH_BUFFER_SIZE, 24},
    {keys::SURFACE_STENCIL_BUFFER_SIZE, 8},
    {keys::SURFACE_NUMBER_SAMPLES, 4},
    {keys::SURFACE_VSYNC_ENABLED, true},
    {keys::ENABLE_DEPTH_TEST, true},
    {keys::SELECTION_COLOR, "#ffac00"}, // yellow
    {keys::LIGHT_TRACKS_CAMERA, true},
    {keys::LIGHT_POSITION_1, QVector3D(10.0f, 10.0f, 10.0f)},
    {keys::LIGHT_POSITION_2, QVector3D(-10.0f, 10.0f, 10.0f)},
    {keys::LIGHT_POSITION_3, QVector3D(10.0f, -10.0f, 10.0f)},
    {keys::LIGHT_POSITION_4, QVector3D(-10.0f, -10.0f, 10.0f)},
    {keys::LIGHT_AMBIENT, QVariant::fromValue(Qt::white)},
    {keys::LIGHT_SPECULAR_1, QVariant::fromValue(Qt::white)},
    {keys::LIGHT_SPECULAR_2, QVariant::fromValue(Qt::white)},
    {keys::LIGHT_SPECULAR_3, QVariant::fromValue(Qt::white)},
    {keys::LIGHT_SPECULAR_4, QVariant::fromValue(Qt::white)},
    {keys::LIGHT_AMBIENT_INTENSITY, 0.1f},
    {keys::LIGHT_INTENSITY_1, 12.0f},  // Key light (strongest)
    {keys::LIGHT_INTENSITY_2, 6.0f},   // Fill light (medium)
    {keys::LIGHT_INTENSITY_3, 8.0f},   // Rim light (medium-strong)
    {keys::LIGHT_INTENSITY_4, 3.0f},   // Environment light (gentle)
    {keys::LIGHT_ATTENUATION_MINIMUM, 0.2f},
    {keys::LIGHT_ATTENUATION_MAXIMUM, 40.0f},
    {keys::LIGHTING_EXPOSURE, 1.0f},
    {keys::LIGHTING_TONEMAP, 1},
    {keys::MATERIAL, 2},
    {keys::TEXT_FONT_FAMILY, "Sans"},
    {keys::TEXT_FONT_SIZE, 70},
    {keys::DEBUG_VISUALIZATION_ENABLED, false},
    {keys::TEXT_OUTLINE, 0.05f},
    {keys::TEXT_BUFFER, 0.02f},
    {keys::TEXT_SMOOTHING, 0.42f},
    {keys::TEXT_COLOR, QVariant::fromValue(Qt::black)},
    {keys::TEXT_OUTLINE_COLOR, QVariant::fromValue(Qt::white)},
    {keys::DEPTH_FOG_ENABLED, GLOBAL_DEPTH_FOG_ENABLED},
    {keys::DEPTH_FOG_DENSITY, GLOBAL_DEPTH_FOG_DENSITY},
    {keys::DEPTH_FOG_OFFSET, GLOBAL_DEPTH_FOG_OFFSET},
    // molecule lighting ...
    {keys::MATERIAL_ROUGHNESS, 0.10f},
    {keys::MATERIAL_METALLIC, 0.05f},
    {keys::SCREEN_GAMMA, 2.2f},
    // other ...
    {keys::USE_PERSPECTIVE_FLAG, false},
    {keys::MAIN_WINDOW_SIZE, QSize(1920, 1080)},
    {keys::FACE_HIGHLIGHT_COLOR, "red"},
    // Special -- for development only
    {keys::ALLOW_CSV_FINGERPRINT_EXPORT, true},
    {keys::ENERGY_FRAMEWORK_POSITIVE_COLOR, QColor("#ffac00")},
    {keys::ENERGY_TABLE_PRECISION, 1},
    // Energy Structure
    {keys::ENERGY_FRAMEWORK_SCALE, 0.001f}, // pm per kJ/mol;
    {keys::ENERGY_COLOR_SCHEME, "Austria"},
    {keys::ENERGY_FRAMEWORK_CUTOFF_COULOMB, 0.0},    // kJ/mol
    {keys::ENERGY_FRAMEWORK_CUTOFF_DISPERSION, 0.0}, // kJ/mol
    {keys::ENERGY_FRAMEWORK_CUTOFF_TOTAL, 0.0},      // kJ/mol
    {keys::WRITE_GAUSSIAN_CP_FILES, false},
    {keys::ENABLE_EXPERIMENTAL_INTERACTION_ENERGIES, false},
    {keys::ENABLE_EXPERIMENTAL_FEATURE_FLAG, false},
    {keys::SHOW_LIGHT_POSITIONS, false},
    {keys::USE_IMPOSTOR_RENDERING, false},
    {keys::TARGET_FRAMERATE, 120},
    {keys::ENABLE_PERFORMANCE_TIMING, true},
};

namespace impl {
QSettings get() { return QSettings(ORGANISATION_NAME, APPLICATION_NAME); }

QSettings getPrev() {
  return QSettings(PREV_ORGANISATION_NAME, PREV_APPLICATION_NAME);
}
} // namespace impl

QVariant readSetting(const QString key, const SettingsVersion version) {
  QVariant result;
  switch (version) {
  case SettingsVersion::Current: {
    QSettings settings = impl::get();
    result = settings.value(key, defaults.value(key, {}));
    break;
  }
  case SettingsVersion::Previous:
    QSettings settings = impl::getPrev();
    result = settings.value(key, defaults.value(key, {}));
    break;
  }
  return result;
}

void writeSetting(const QString key, const QVariant value) {
  QSettings settings = impl::get();
  settings.setValue(key, value);
}

void writeSettings(const QMap<QString, QVariant> &newSettings) {
  QSettings settings = impl::get();
  for (auto i = newSettings.begin(); i != newSettings.end(); ++i) {
    settings.setValue(i.key(), i.value());
  }
}

void restoreDefaultSetting(const QString &key) {
  QSettings settings = impl::get();
  settings.setValue(key, defaults.value(key, {}));
}

void restoreDefaultSettings(const QStringList &keys) {
  QSettings settings = impl::get();
  for (const auto &key : keys) {
    settings.setValue(key, defaults.value(key, {}));
  }
}

void writeSettingIfEmpty(const QString key, const QVariant value) {
  QSettings settings = impl::get();
  if (!settings.contains(key)) {
    settings.setValue(key, value);
  }
}

QString filePath() {
  QSettings settings = impl::get();
  return settings.fileName();
}

QStringList settingsFromGroup(const QString &group) {
  QSettings settings = impl::get();
  settings.beginGroup(group);
  QStringList keys = settings.childKeys();
  settings.endGroup();
  return keys;
}

void writeAllDefaultSettings(bool override) {
  QSettings settings = impl::get();
  for (auto kv = defaults.constKeyValueBegin();
       kv != defaults.constKeyValueEnd(); kv++) {
    if (override || !settings.contains(kv->first)) {
      settings.setValue(kv->first, kv->second);
    }
  }
}

} // namespace settings
