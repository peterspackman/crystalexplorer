#include "shaderloader.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>

namespace cx::shader {

QString readFileContents(const QString &filename) {

  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    // Handle the error
    qWarning() << "Unable to open shader file:" << filename;
    return QString();
  }

  QTextStream stream(&file);
  return stream.readAll();
}

QString processIncludes(const QString &shaderSource) {
  QStringList lines = shaderSource.split('\n');
  QString processedSource;

  for (const QString &line : lines) {
    if (line.trimmed().startsWith("#include")) {
      // Extract the file name from the #include directive
      int start = line.indexOf('"') + 1;
      int end = line.lastIndexOf('"');
      QString includeFileName = line.mid(start, end - start);

      // Read the included file content
      QString includeFileContent =
          readFileContents(":/shaders/" + includeFileName);

      // Recursively process includes within the included file
      includeFileContent = processIncludes(includeFileContent);

      // Append the contents of the included file to the processed source
      processedSource.append(includeFileContent);
    } else {
      processedSource.append(line);
    }
    processedSource.append('\n');
  }
  return processedSource;
}

QString loadShaderFile(const QString &filename) {
  QString shaderSource = readFileContents(filename);
  qDebug() << "Processing shader source for " << filename;
  return processIncludes(shaderSource);
}

} // namespace cx::shader
