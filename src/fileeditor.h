#pragma once
#include <QMainWindow>

#include "ui_fileeditor.h"

/*!
 \brief A barebones QMainWindow for editing input files.
 */
class FileEditor : public QMainWindow, public Ui::FileEditor {
  Q_OBJECT

public:
  FileEditor(QWidget *parent = 0);
  void insertFile(QString);

public slots:
  void saveFile();

signals:
  void writtenFileToDisk();

private:
  void init();

  QString _filename;
};
