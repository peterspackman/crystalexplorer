#include "shaderloader.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

namespace cx::shader {

QString readFileContents(const QString &filename) {

  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    // Handle the error
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

QString adaptShaderForWasm(const QString &shaderSource) {
#ifdef Q_OS_WASM
  QString adapted = shaderSource;

  // Convert #version 330 core to #version 300 es
  adapted.replace(QRegularExpression("#version\\s+330(\\s+core)?"), "#version 300 es");

  // Add precision qualifier after version line for fragment shaders
  // (detect fragment shader by presence of FragColor or similar output)
  if (adapted.contains("FragColor") || adapted.contains("gl_FragColor") ||
      adapted.contains("out vec4")) {
    // Find the version line
    int versionEnd = adapted.indexOf('\n');
    if (versionEnd != -1) {
      // Insert precision after version line
      adapted.insert(versionEnd + 1, "precision highp float;\n");
    }
  }

  return adapted;
#else
  return shaderSource;
#endif
}

QString loadShaderFile(const QString &filename) {
  QString shaderSource = readFileContents(filename);
  qDebug() << "Processing shader source for " << filename;
  shaderSource = processIncludes(shaderSource);
  return adaptShaderForWasm(shaderSource);
}

} // namespace cx::shader
