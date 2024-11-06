#include "io_utilities.h"
#include "texteditdialog.h"
#include <QByteArray>
#include <QFileInfo>
#include <QTextStream>

namespace io {

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

QByteArray readFileBytes(const QString &filePath,
                         QIODevice::OpenMode modeFlags) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | modeFlags))
    return {};

  auto result = file.readAll();
  file.close();
  return result;
}

bool deleteFile(const QString &filePath) {
  // return true if the file does not exist at the end of this function
  if (QFileInfo(filePath).exists()) {
    if (!QFile::remove(filePath)) {
      qDebug() << "Could not delete file..." << filePath;
      return false;
    }
    qDebug() << "File deleted: " << !QFileInfo(filePath).exists();
  }
  return true;
}

bool deleteFiles(const QStringList &filePaths) {
  bool success = true;
  for (const auto &filePath : filePaths) {
    success &= deleteFile(filePath);
  }
  return success;
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

bool writeTextFile(const QString &filename, const QString &text) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
    return false;
  file.write(text.toUtf8());
  file.close();
  return true;
}

QString changeSuffix(const QString &filePath, const QString &suffix) {
  QFileInfo fileInfo(filePath);
  return fileInfo.completeBaseName() + suffix;
}

bool editableTextToFileBlocking(const QString &filename, const QString &text,
                                bool showEditor, QWidget *parent) {
  QString contentToWrite = text;

  if (showEditor) {
    TextEditDialog dialog(text, parent);
    if (dialog.exec() == QDialog::Accepted) {
      contentToWrite = dialog.getText();
    } else {
      // User cancelled the editing
      return false;
    }
  }

  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    if (showEditor) {
      QMessageBox::critical(parent, "Error",
                            QString("Failed to open file %1 for writing: %2")
                                .arg(filename)
                                .arg(file.errorString()));
    }
    return false;
  }

  // Write the content
  QByteArray data = contentToWrite.toUtf8();
  qint64 bytesWritten = file.write(data);
  file.close();

  if (bytesWritten != data.size()) {
    if (showEditor) {
      QMessageBox::critical(
          parent, "Error",
          QString("Failed to write complete data to file %1").arg(filename));
    }
    return false;
  }

  return true;
}

} // namespace io
