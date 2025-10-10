#include "exefileutilities.h"
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QTextStream>

namespace exe {

QString readFileContents(const QString &filePath,
                         const QString &binaryPlaceholder) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return QString("Error opening file: %1 - %2")
        .arg(filePath)
        .arg(file.errorString());
  }

  if (!io::isTextFile(filePath)) {
    file.close();
    return binaryPlaceholder;
  }

  QTextStream stream(&file);
  QString contents = stream.readAll();

  if (stream.status() != QTextStream::Ok) {
    return QString("Error reading file: %1 - %2")
        .arg(filePath)
        .arg(stream.status() == QTextStream::ReadPastEnd
                 ? "Unexpected end of file"
                 : "Unknown error");
  }

  file.close();
  return contents;
}

QString findProgramInPath(const QString &program) {
#ifdef Q_OS_WASM
  // External programs not supported in WASM
  Q_UNUSED(program);
  return QString();
#else
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
#endif
}

} // namespace exe
