#include "surfaceinfodocument.h"
#include "globals.h"
#include <QFont>
#include <QTextCursor>
#include <fmt/format.h>

inline const char *INFO_HORIZONTAL_RULE =
    "--------------------------------------------------------------------------"
    "------------\n";

SurfaceInfoDocument::SurfaceInfoDocument(QWidget *parent) : QWidget(parent) {
  setupUI();
  populateDocument();
}

void SurfaceInfoDocument::setupUI() {
  QVBoxLayout *layout = new QVBoxLayout(this);

  QFont monoFont("Courier");
  monoFont.setStyleHint(QFont::Monospace);
  monoFont.setFixedPitch(true);

  m_contents = new QTextEdit(this);
  m_contents->document()->setDefaultFont(monoFont);
  layout->addWidget(m_contents);
}

void SurfaceInfoDocument::populateDocument() {
  if (!m_scene)
    return;
  m_contents->clear();

  ChemicalStructure *structure = m_scene->chemicalStructure();
  if (!structure)
    return;

  Mesh *mesh = nullptr;
  QTextCursor cursor = m_contents->textCursor();
  cursor.beginEditBlock();
  insertGeneralInformation(cursor, mesh);
  cursor.endEditBlock();
  resetCursorToBeginning();
}

void SurfaceInfoDocument::resetCursorToBeginning() {
  QTextCursor cursor = m_contents->textCursor();
  cursor.movePosition(QTextCursor::Start);
  m_contents->setTextCursor(cursor);
  m_contents->ensureCursorVisible();
}

void SurfaceInfoDocument::updateScene(Scene *scene) {
  m_scene = scene;
  populateDocument();
}

void SurfaceInfoDocument::insertGeneralInformation(QTextCursor &cursor,
                                                   Mesh *mesh) {
  if (!mesh)
    return;
}

void SurfaceInfoDocument::insertMeshInstanceInformation(QTextCursor &cursor,
                                                        MeshInstance *mesh) {
  if (!mesh)
    return;
}
