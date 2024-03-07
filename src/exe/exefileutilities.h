#pragma once
#include <QString>

namespace exe {

bool isTextFile(const QString& filePath);
QString readFileContents(const QString& filePath, const QString& binaryPlaceholder = "Binary file");
bool copyFile(const QString &sourcePath, const QString &targetPath, bool overwrite);
QString findProgramInPath(const QString &program);

}
