#pragma once
#include "io_utilities.h"
#include <QString>
#include <QByteArray>

namespace exe {

QString readFileContents(const QString& filePath, const QString& binaryPlaceholder = "Binary file");
QString findProgramInPath(const QString &program);

}
