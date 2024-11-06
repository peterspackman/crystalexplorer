#include "texteditdialog.h"

TextEditDialog::TextEditDialog(const QString &text, QWidget *parent)
    : QDialog(parent) {
  setWindowTitle("Edit File Contents");
  setModal(true);

  auto layout = new QVBoxLayout(this);

  // Create text editor
  editor = new QPlainTextEdit(this);
  editor->setPlainText(text);
  editor->setMinimumSize(600, 400);
  layout->addWidget(editor);

  // Create button box
  auto buttonBox = new QHBoxLayout();
  auto saveButton = new QPushButton("Save", this);
  auto cancelButton = new QPushButton("Cancel", this);

  buttonBox->addStretch();
  buttonBox->addWidget(saveButton);
  buttonBox->addWidget(cancelButton);
  layout->addLayout(buttonBox);

  connect(saveButton, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QString TextEditDialog::getText() const { return editor->toPlainText(); }
