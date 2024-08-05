#pragma once
#include <QString>

namespace exe {

bool isTextFile(const QString& filePath);
bool writeTextFile(const QString &filename, const QString &text);
bool copyFile(const QString &sourcePath, const QString &targetPath, bool overwrite);
bool deleteFile(const QString &filePath);

QString readFileContents(const QString& filePath, const QString& binaryPlaceholder = "Binary file");

QString findProgramInPath(const QString &program);

QString changeSuffix(const QString &filePath, const QString &suffix);

}
