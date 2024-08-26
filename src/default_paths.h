#pragma once
#include <QCoreApplication>
#include <QDir>
#include <QString>

namespace cx::paths {


inline QString determineOCCDataDirectoryPath() {
  const QString appDirPath = QCoreApplication::applicationDirPath();

#if defined(Q_OS_MACOS)
  // For macOS, data is in .app/Contents/Resources/
  QDir dir(appDirPath);
  dir.cdUp();
  return dir.filePath("Resources/share/occ");
#elif defined(Q_OS_LINUX)
  // For Linux, data is in usr/share/crystalexplorer/
  QDir dir(appDirPath);
  dir.cdUp();
  return dir.filePath("share/crystalexplorer/share/occ");
#elif defined(Q_OS_WIN)
  // For Windows, data is in the same directory as the executable
  return QDir(appDirPath).filePath("share/occ");
#else
#error "Unsupported platform"
#endif
}

inline QString determineResourcesPath() {
  const QString appDirPath = QCoreApplication::applicationDirPath();

#if defined(Q_OS_MACOS)
  // For macOS, resources are in .app/Contents/Resources/
  QDir dir(appDirPath);
  dir.cdUp();
  return dir.filePath("Resources");
#elif defined(Q_OS_LINUX)
  // For Linux, resources are in usr/share/crystalexplorer/
  QDir dir(appDirPath);
  dir.cdUp();
  return dir.filePath("share/crystalexplorer");
#elif defined(Q_OS_WIN)
  // For Windows, resources are in the same directory as the executable
  return appDirPath;
#else
#error "Unsupported platform"
#endif
}

inline QString determineOCCExecutablePath() {
  const QString appDirPath = QCoreApplication::applicationDirPath();
#if defined(Q_OS_MACOS)
  // For macOS, executable is in .app/Contents/MacOS/
  return QDir(appDirPath).filePath("occ");
#elif defined(Q_OS_LINUX)
  // For Linux, executable is in usr/bin/
  return QDir(appDirPath).filePath("occ");
#elif defined(Q_OS_WIN)
  // For Windows, executable is in the same directory as the main app
  return QDir(appDirPath).filePath("occ.exe");
#else
#error "Unsupported platform"
#endif
}

} // namespace cx::paths
