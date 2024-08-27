#pragma once
#include "scene.h"
#include "meshinstance.h"
#include <QVBoxLayout>
#include <QTextEdit>
#include <QTabWidget>
#include <QWidget>

class SurfaceInfoDocument: public QWidget {
  Q_OBJECT

public:
  explicit SurfaceInfoDocument(QWidget *parent = nullptr);

  void updateScene(Scene *scene);

private:
  Scene *m_scene{nullptr};
  QTextEdit *m_contents{nullptr};

  void setupUI();
  void populateDocument();
  void resetCursorToBeginning();
  void insertGeneralInformation(QTextCursor &, Mesh *);
  void insertMeshInstanceInformation(QTextCursor &, MeshInstance *);

};
