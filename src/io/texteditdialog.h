#pragma once
#include <QDialog>
#include <QFile>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

class TextEditDialog : public QDialog {
  Q_OBJECT
public:
  TextEditDialog(const QString &text, QWidget *parent = nullptr);
  QString getText() const;

private:
  QPlainTextEdit *editor{nullptr};
};
