#include <QFile>
#include <QMessageBox>
#include <QTextStream>

#include "fileeditor.h"

FileEditor::FileEditor(QWidget *parent) : QMainWindow(parent) {
  setupUi(this);
  init();
}

void FileEditor::init() {
  setWindowFlags(Qt::Tool);
  textEdit->setFocus();
  textEdit->setFont(QFont("courier"));
  connect(buttonBox, &QDialogButtonBox::accepted, this, &FileEditor::saveFile);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &FileEditor::hide);
}

void FileEditor::insertFile(QString filename) {
  _filename = filename;

  QFile inFile(_filename);
  if (inFile.open(QIODevice::ReadOnly)) {
    QTextStream ts(&inFile);
    textEdit->setPlainText(ts.readAll());
    textEdit->moveCursor(QTextCursor::Start);
  } else {
    QMessageBox::warning(this, "Error", "Unable to read file:\n" + _filename);
  }
}

void FileEditor::saveFile() {
  if (QFile::exists(_filename)) {
    QFile::remove(_filename);
  }
  QFile outFile(_filename);
  if (outFile.open(QIODevice::WriteOnly)) {
    QTextStream ts(&outFile);
    ts << textEdit->toPlainText();
    outFile.close();
    emit writtenFileToDisk();
  } else {
    QMessageBox::warning(this, "Error", "Unable to write file " + _filename);
  }
  hide();
}
