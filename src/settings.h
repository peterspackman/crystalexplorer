#pragma once
#include "version.h"
#include <QColor>
#include <QSettings>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector3D>

namespace settings {
// The following is used to define a place to store user choices or defaults for
// the program
const QString ORGANISATION_NAME = "crystalexplorer.net";
const QString APPLICATION_NAME =
    QString("CrystalExplorer%1").arg(CX_VERSION_MAJOR);

const QString PREV_ORGANISATION_NAME = "hirshfeldsurface.net";
const QString PREV_APPLICATION_NAME =
    QString("CrystalExplorer%1").arg(HSPrevVersion);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Keys and their *default* values stored for this application.
// The scheme goes: SOMETHING has default value SOMETHING
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// KEYS
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Licenses
const QString LICENCE_GROUP = "licenceVersion2/";
const QString GLOBAL_ELEMENTDATA_FILE = ":/resources/elementdata.json";

// Tonto
#if defined(Q_OS_LINUX)
const QString GLOBAL_TONTO_PATH = "/usr/bin/";
const QString GLOBAL_TONTO_EXECUTABLE = "tonto"; // linux
#elif defined(Q_OS_WIN)
const QString GLOBAL_TONTO_EXECUTABLE = "tonto.exe"; // Windows
#elif defined(Q_OS_MACOS)
const QString GLOBAL_TONTO_EXECUTABLE = "tonto"; // MacOS
#endif
const QString GLOBAL_TONTO_BASIS_DIRECTORY = "basis_sets";

const bool GLOBAL_DEPTH_FOG_ENABLED = false;
const float GLOBAL_DEPTH_FOG_DENSITY = 10.0f;
const float GLOBAL_DEPTH_FOG_OFFSET = 0.1f;

namespace keys {
const QString LICENCE_USER_NAME = LICENCE_GROUP + "userName";
const QString LICENCE_CODE = LICENCE_GROUP + "code";

const QString USE_OCC_NOT_TONTO = "useOccNotTonto";

const QString PREFERRED_WAVEFUNCTION_SOURCE = "preferredWavefunctionSource";

// Gaussian
const QString GAUSSIAN_GROUP = "gaussian";
const QString GAUSSIAN_EXECUTABLE = GAUSSIAN_GROUP + "/executablePath";
const QString GAUSSIAN_MDEF = GAUSSIAN_GROUP + "/memoryEnvironmentVariable";
const QString GAUSSIAN_PDEF = GAUSSIAN_GROUP + "/nprocsEnvironmentVariable";

// NWChem
const QString NWCHEM_GROUP = "nwchem";
const QString NWCHEM_EXECUTABLE = NWCHEM_GROUP + "/executablePath";

// Psi4
const QString PSI4_GROUP = "psi4";
const QString PSI4_EXECUTABLE = PSI4_GROUP + "/executablePath";

// Occ
const QString OCC_GROUP = "occ";
const QString OCC_EXECUTABLE = OCC_GROUP + "/executablePath";
const QString OCC_BASIS_DIRECTORY = OCC_GROUP + "/basisSetDirectory";
const QString OCC_NTHREADS = OCC_GROUP + "/numThreads";

// Orca
const QString ORCA_GROUP = "orca";
const QString ORCA_EXECUTABLE = ORCA_GROUP + "/executablePath";
const QString ORCA_NTHREADS = ORCA_GROUP + "/numProcs";

// XTB
const QString XTB_GROUP = "xtb";
const QString XTB_EXECUTABLE = XTB_GROUP + "/executablePath";

// Tonto
const QString TONTO_GROUP = "tonto/";
const QString TONTO_EXECUTABLE = TONTO_GROUP + "executable";
const QString TONTO_USER_EXECUTABLE = TONTO_GROUP + "userExecutable";
const QString TONTO_BASIS_DIRECTORY = TONTO_GROUP + "basisDirectory";
const QString TONTO_PATH = TONTO_GROUP + "path";

const QString DISABLE_XH_NORMALIZATION = TONTO_GROUP + "disableXHNormalisation";
const QString CH_BOND_LENGTH = TONTO_GROUP + "CHBondLength";
const QString NH_BOND_LENGTH = TONTO_GROUP + "NHBondLength";
const QString OH_BOND_LENGTH = TONTO_GROUP + "OHBondLength";
const QString BH_BOND_LENGTH = TONTO_GROUP + "BHBondLength";
const QString USE_CLEMENTI = TONTO_GROUP + "useClementiBasisset";

// Other Programs
const QString PROGRAMS_GROUP = "programs/";
const QString DFTD3_EXECUTABLE = PROGRAMS_GROUP + "dftd3_executable";

// CrystalExplorer
const QString EXECUTABLE = "executable";
const QString ELEMENTDATA_FILE = "elementdataFile";
const QString DOCUMENTATION = "documentation";
const QString DELETE_WORKING_FILES = "deleteWorkingFiles";
const QString AUTOLOAD_LAST_FILE = "autoLoadLastFile";
const QString FILE_HISTORY_LIST = "fileHistoryList";
const QString BACKGROUND_COLOR = "backgroundColor";
const QString NONE_PROPERTY_COLOR = "nonePropertyColor";
const QString ATOM_LABEL_COLOR = "atomLabelColor";
const QString BOND_THICKNESS = "bondThickness";
const QString CONTACT_LINE_THICKNESS = "contactLineThickness";
const QString USE_SBF_INTERFACE = "useSBFFilesForSurfaceGeneration";
const QString USE_JMOL_COLORS = "useJmolColors";

const QString RESET_ELEMENTS_ELEMENTDATATXTFILE =
    "general/resetElementsFromElementDataTxtFile";

// Close contacts
const QString HBOND_COLOR = "hbondColor";
const QString CONTACT1_COLOR = "contact1Color";
const QString CONTACT2_COLOR = "contact2Color";
const QString CONTACT3_COLOR = "contact3Color";

// OpenGL defaults
const QString SURFACE_NUMBER_SAMPLES = "graphics/numberSamples";
const QString SURFACE_DEPTH_BUFFER_SIZE = "graphics/depthBufferSize";
const QString SURFACE_STENCIL_BUFFER_SIZE = "graphics/stencilBufferSize";
const QString ENABLE_DEPTH_TEST = "graphics/enableDepthTest";

const QString SELECTION_COLOR = "graphics/selectionColor";
const QString CE_BLUE_COLOR = "graphics/blueColor";
const QString CE_GREEN_COLOR = "graphics/greenColor";
const QString CE_RED_COLOR = "graphics/redColor";

const QString LIGHT_TRACKS_CAMERA = "graphics/lightTracksCamera";
const QString MATERIAL_ROUGHNESS = "graphics/materialRoughness";
const QString MATERIAL_METALLIC = "graphics/materialMetallic";
const QString USE_PERSPECTIVE_FLAG = "usePerspectiveFlag";
const QString SCREEN_GAMMA = "screenGamma";
const QString LIGHT_POSITION_1 = "graphics/lightPosition1";
const QString LIGHT_POSITION_2 = "graphics/lightPosition2";
const QString LIGHT_POSITION_3 = "graphics/lightPosition3";
const QString LIGHT_POSITION_4 = "graphics/lightPosition4";
const QString LIGHT_AMBIENT = "graphics/lightAmbientColour";
const QString LIGHT_AMBIENT_INTENSITY = "graphics/lightAmbientIntensity";
const QString LIGHT_SPECULAR_1 = "graphics/light1SpecularColour";
const QString LIGHT_SPECULAR_2 = "graphics/light2SpecularColour";
const QString LIGHT_SPECULAR_3 = "graphics/light3SpecularColour";
const QString LIGHT_SPECULAR_4 = "graphics/light4SpecularColour";
const QString SHOW_LIGHT_POSITIONS = "graphics/showLightPositions";
const QString LIGHT_INTENSITY_1 = "graphics/lightIntensity1";
const QString LIGHT_INTENSITY_2 = "graphics/lightIntensity2";
const QString LIGHT_INTENSITY_3 = "graphics/lightIntensity3";
const QString LIGHT_INTENSITY_4 = "graphics/lightIntensity4";
const QString LIGHT_ATTENUATION_MINIMUM = "graphics/lightAttenuationMinimum";
const QString LIGHT_ATTENUATION_MAXIMUM = "graphics/lightAttenuationMaximum";
const QString LIGHTING_TONEMAP = "graphics/lightToneMapIdentifier";
const QString LIGHTING_EXPOSURE = "graphics/lightExposure";
const QString MATERIAL = "graphics/material";
const QString DEPTH_FOG_ENABLED = "graphics/depthFogEnabled";
const QString DEPTH_FOG_DENSITY = "graphics/depthFogDensity";
const QString DEPTH_FOG_OFFSET = "graphics/depthFogOffset";

const QString TEXT_OUTLINE = "graphics/textOutlineWidth";
const QString TEXT_BUFFER = "graphics/textBufferWidth";
const QString TEXT_SMOOTHING = "graphics/textSmoothingWidth";
const QString TEXT_COLOR = "graphics/textColor";
const QString TEXT_OUTLINE_COLOR = "graphics/textOutlineColor";

const QString BIG_TOOLBAR_BUTTONS = "general/UseBigToolbarButtons";
const QString MAIN_WINDOW_SIZE = "general/mainWindowSize";

const QString FACE_HIGHLIGHT_COLOR = "fingerprint/faceHighlightColor";
const QString ALLOW_CSV_FINGERPRINT_EXPORT = "fingerprint/allowCsvExport";

// Energy Structure
const QString ENERGY_FRAMEWORK_SCALE = "energyFrameworkScale";
const QString ENERGY_COLOR_SCHEME = "energyColourScheme";

const QString ENERGY_FRAMEWORK_CUTOFF_COULOMB = "energyFrameworkCutoffCoulomb";
const QString ENERGY_FRAMEWORK_CUTOFF_DISPERSION =
    "energyFrameworkCutoffDispersion";
const QString ENERGY_FRAMEWORK_CUTOFF_TOTAL = "energyFrameworkCutoffTotal";
const QString ENERGY_FRAMEWORK_POSITIVE_COLOR = "energyFrameworkPositiveColor";
const QString WRITE_GAUSSIAN_CP_FILES = "writeGaussianCPFiles";

const QString ENERGY_TABLE_PRECISION = "energyTablePrecision";

const QString ENABLE_EXPERIMENTAL_INTERACTION_ENERGIES =
    "energyExperimentalToggle";

const QString ENABLE_EXPERIMENTAL_FEATURE_FLAG = "experimentalFeatures";

} // namespace keys

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Settings Class
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum class SettingsVersion { Current, Previous };
QVariant readSetting(const QString,
                     const SettingsVersion = SettingsVersion::Current);
void restoreDefaultSettings(const QStringList &keys);
void restoreDefaultSetting(const QString &key);
void writeSetting(const QString key, const QVariant value);
void writeSettings(const QMap<QString, QVariant> &newSettings);
void writeSettingIfEmpty(const QString key, const QVariant value);
QStringList settingsFromGroup(const QString &group);

void writeAllDefaultSettings(bool overwrite = false);

QString filePath();
} // namespace settings
#define USE_SBF
