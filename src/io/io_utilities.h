#pragma once
#include <QFile>

namespace io {

bool isTextFile(const QString& filePath);
bool writeTextFile(const QString &filename, const QString &text);
bool copyFile(const QString &sourcePath, const QString &targetPath, bool overwrite);
bool deleteFile(const QString &filePath);
bool deleteFiles(const QStringList &filePaths);

QString changeSuffix(const QString &filePath, const QString &suffix);
QByteArray readFileBytes(const QString& filePath, QIODeviceBase::OpenMode mode = QIODevice::Text);

}
