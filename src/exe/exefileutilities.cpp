#include "exefileutilities.h"
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QTextStream>

namespace exe {

bool isTextFile(const QString &filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly))
    return false;

  QByteArray data = file.read(1024);
  file.close();

  for (unsigned char byte : data) {
    if (byte == '\0' ||
        (byte < 0x20 && byte != '\n' && byte != '\r' && byte != '\t')) {
      return false;
    }
  }

  return true;
}

QString readFileContents(const QString &filePath,
                         const QString &binaryPlaceholder) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return QString(
        "Can't open file"); // Return empty string if file can't be opened

  if (!isTextFile(filePath))
    return binaryPlaceholder;

  QTextStream stream(&file);
  QString contents = stream.readAll();

  file.close();
  return contents;
}

bool copyFile(const QString &sourcePath, const QString &targetPath,
              bool overwrite) {
  // if they're the same file just do nothing.
  if (sourcePath == targetPath)
    return true;

  // check if it exists
  if (QFileInfo(targetPath).exists()) {
    qDebug() << "File exists, should overwrite: " << overwrite;
    if (!overwrite)
      return false;
    if (!QFile::remove(targetPath)) {
      qDebug() << "Could not delete file...";
      return false;
    }
    qDebug() << "File deleted: " << !QFileInfo(targetPath).exists();
  }

  // just use QFile::copy
  return QFile::copy(sourcePath, targetPath);
}

QString findProgramInPath(const QString &program) {
  QFileInfo file(program);
  if (file.isAbsolute() && file.isExecutable()) {
    return program;
  }
  QStringList paths =
      QProcessEnvironment::systemEnvironment().value("PATH").split(
          QDir::listSeparator());
  for (const auto &path : paths) {
    QString candidate = QDir(path).absoluteFilePath(program);
    QFileInfo candidateFile(candidate);
    if (candidateFile.isExecutable() && !candidateFile.isDir()) {
      return candidateFile.absoluteFilePath();
    }
  }
  return QString();
}

bool writeTextFile(const QString &filename, const QString &text) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
    return false;
  file.write(text.toUtf8());
  file.close();
  return true;
}

} // namespace exe
