#include <QApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QString>
#include <QSurfaceFormat>

#include "crystalx.h"
#include "default_paths.h"
#include "globalconfiguration.h"
#include "settings.h"

void copySettingFromPreviousToCurrent(QString key) {
  QVariant currentValue = settings::readSetting(key);
  QVariant prevValue =
      settings::readSetting(key, settings::SettingsVersion::Previous);

  // Only write the setting if there wasn't a current value
  // and there is a value from the previous settings
  if (currentValue.isNull() && !prevValue.isNull()) {
    settings::writeSetting(key, prevValue);
  }
}

void copySelectSettingsFromPreviousToCurrent() {
  copySettingFromPreviousToCurrent(settings::keys::GAUSSIAN_EXECUTABLE);
}

void addDefaultPathsIfNotSet() {
  // Handle OCC executable path
  QString occExecutablePath =
      settings::readSetting(settings::keys::OCC_EXECUTABLE).toString();
  QFileInfo occExecutableInfo(occExecutablePath);

  if (occExecutablePath.isEmpty() || !occExecutableInfo.exists() ||
      !occExecutableInfo.isExecutable()) {
    // Path is either not set, doesn't exist, or isn't executable
    QString defaultOccExecutablePath = cx::paths::determineOCCExecutablePath();
    settings::writeSetting(settings::keys::OCC_EXECUTABLE,
                           defaultOccExecutablePath);
  }

  // Handle OCC data directory path
  QString occDataDirectoryPath =
      settings::readSetting(settings::keys::OCC_DATA_DIRECTORY).toString();
  QFileInfo occDataDirectoryInfo(occDataDirectoryPath);

  if (occDataDirectoryPath.isEmpty() || !occDataDirectoryInfo.exists() ||
      !occDataDirectoryInfo.isDir()) {
    // Path is either not set, doesn't exist, or isn't a directory
    QString defaultOccDataDirectoryPath =
        cx::paths::determineOCCDataDirectoryPath();
    settings::writeSetting(settings::keys::OCC_DATA_DIRECTORY,
                           defaultOccDataDirectoryPath);
  }
}

void maybeReOpenFiles(Crystalx *cx) {
  // Depending on settings, autoload the most recently opened CIF
  bool autoLoadLastFile =
      settings::readSetting(settings::keys::AUTOLOAD_LAST_FILE).toBool();
  QStringList history =
      settings::readSetting(settings::keys::FILE_HISTORY_LIST).toStringList();
  if (autoLoadLastFile && history.size() > 0) {
    QString passedFile = history[0];
    if (QFile::exists(passedFile)) {
      cx->loadExternalFileData(passedFile);
    }
  }
}

int main(int argc, char *argv[]) {
  QApplication::addLibraryPath("./");
  QSurfaceFormat format;
  format.setDepthBufferSize(
      settings::readSetting(settings::keys::SURFACE_DEPTH_BUFFER_SIZE).toInt());
  format.setStencilBufferSize(
      settings::readSetting(settings::keys::SURFACE_STENCIL_BUFFER_SIZE)
          .toInt());
#ifdef Q_OS_WASM
  // WebGL only supports OpenGL ES 3.0, not desktop OpenGL 4.3
  format.setVersion(3, 0);
  format.setRenderableType(QSurfaceFormat::OpenGLES);
#else
  format.setVersion(4, 3);
  format.setProfile(QSurfaceFormat::CoreProfile);
#endif
#ifdef QT_DEBUG
  format.setOption(QSurfaceFormat::DebugContext, true);
#endif
  format.setSamples(0); // Disable widget MSAA, use FBO MSAA instead for better control

  // Configure VSync
  bool vsyncEnabled = settings::readSetting(settings::keys::SURFACE_VSYNC_ENABLED).toBool();
  format.setSwapInterval(vsyncEnabled ? 1 : 0); // 1 = VSync on, 0 = VSync off

  QSurfaceFormat::setDefaultFormat(format);

  QApplication app(argc, argv);

  QCoreApplication::setOrganizationName(settings::ORGANISATION_NAME);
  QCoreApplication::setApplicationName(settings::APPLICATION_NAME);
  QCommandLineParser parser;
  parser.setApplicationDescription(settings::APPLICATION_NAME);
  parser.addHelpOption();
  parser.addVersionOption();
  QCommandLineOption resourcesOption(
      QStringList() << "r"
                    << "resources",
      QCoreApplication::translate("main",
                                  "Specify path to resources directory"),
      QCoreApplication::translate("main", "directory"));
  parser.addOption(resourcesOption);
  QCommandLineOption filesOption(
      QStringList() << "o"
                    << "open",
      QCoreApplication::translate("main", "Specify file to open"),
      QCoreApplication::translate("main", "filename"));
  parser.addOption(filesOption);
  parser.process(app);

  // ensure default settings are written

  settings::writeAllDefaultSettings(false);
  addDefaultPathsIfNotSet();

  auto *config = GlobalConfiguration::getInstance();
  config->load();

  Crystalx *cx = new Crystalx();
  cx->show();

  // Open file passed in from command line
  if (parser.isSet(filesOption)) {
    QString passedFile = parser.value(filesOption);
    passedFile = QFileInfo(passedFile).canonicalFilePath();
    if (QFile::exists(passedFile))
      cx->loadExternalFileData(passedFile);
  } else {
    maybeReOpenFiles(cx);
  }

  return app.exec();
}
