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
  // Check and set OCC executable path
  if (settings::readSetting(settings::keys::OCC_EXECUTABLE)
          .toString()
          .isEmpty()) {
    QString occExecutablePath = cx::paths::determineOCCExecutablePath();
    settings::writeSetting(settings::keys::OCC_EXECUTABLE, occExecutablePath);
  }

  // Check and set OCC data directory path
  if (settings::readSetting(settings::keys::OCC_DATA_DIRECTORY)
          .toString()
          .isEmpty()) {
    QString occDataDirectoryPath =
        cx::paths::determineOCCDataDirectoryPath();
    settings::writeSetting(settings::keys::OCC_DATA_DIRECTORY,
                           occDataDirectoryPath);
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
  format.setVersion(4, 3);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setOption(QSurfaceFormat::DebugContext, true);
  format.setSamples(
      settings::readSetting(settings::keys::SURFACE_NUMBER_SAMPLES).toInt());
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
