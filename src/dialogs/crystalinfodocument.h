#pragma once
#include "scene.h"
#include "crystalstructure.h"
#include <QVBoxLayout>
#include <QTextEdit>
#include <QWidget>

class CrystalInfoDocument: public QWidget {
  Q_OBJECT

public:
  explicit CrystalInfoDocument(QWidget *parent = nullptr);

  void updateScene(Scene *scene);

private:
  Scene *m_scene{nullptr};
  QTextEdit *m_contents{nullptr};

  void setupUI();
  void populateDocument();
  void resetCursorToBeginning();
  void insertGeneralInformation(QTextCursor &, CrystalStructure *);

};
