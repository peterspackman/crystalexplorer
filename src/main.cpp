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
#include "settings.h"
#include "globalconfiguration.h"

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

void writePathsToResourcesInSettings(const QString pathToResources) {
  settings::writeSettings({
    {settings::keys::ELEMENTDATA_FILE, settings::GLOBAL_ELEMENTDATA_FILE},
#if defined(Q_OS_LINUX)
        {settings::keys::OCC_EXECUTABLE,
         settings::GLOBAL_OCC_PATH + settings::GLOBAL_OCC_EXECUTABLE},
#else
        {settings::keys::OCC_EXECUTABLE,
         pathToResources + settings::GLOBAL_OCC_EXECUTABLE},
#endif
        {settings::keys::OCC_DATA_DIRECTORY,
         pathToResources +
             settings::GLOBAL_OCC_DATA_DIRECTORY}, // location of occ share
  });
}

QString getPathToResources() {
  QString pathToCrystalExplorer = QCoreApplication::applicationDirPath();
#if defined(Q_OS_MACOS)
  // On the Mac, we put the ancilliary bits in
  // CrystalExplorer.app/Resources
  QString pathToResources = pathToCrystalExplorer + "/../Resources/";
#elif defined(Q_OS_LINUX)
  QString pathToResources = "/usr/share/crystalexplorer/";
#else
  // On Windows, it's just in the CrystalExplorer
  // executable directory
  QString pathToResources = pathToCrystalExplorer + "/";
#endif
  return pathToResources;
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
  if (parser.isSet(resourcesOption)) {
    QString resourcesPath = parser.value(resourcesOption);
    resourcesPath = QFileInfo(resourcesPath).canonicalFilePath() + "/";
    writePathsToResourcesInSettings(resourcesPath);
  } else {
    writePathsToResourcesInSettings(getPathToResources());
  }
  auto * config = GlobalConfiguration::getInstance();
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
