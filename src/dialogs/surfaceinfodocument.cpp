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

  m_contents->clear();

  if (!m_scene)
    return;

  ChemicalStructure *structure = m_scene->chemicalStructure();
  if (!structure)
    return;

  const auto &selection = m_scene->selectedSurface();
  if (!selection.surface)
    return;
  auto *mesh = selection.surface->mesh();

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
  qDebug() << "General info surface document called with mesh" << mesh;
  return;
  if (!mesh)
    return;

  const QString TITLE = "General Mesh Information";

  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText(TITLE + "\n");
  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText("\n");
  const auto &params = mesh->parameters();
  cursor.insertText(
      QString("Type\t%1\n").arg(isosurface::kindToString(params.kind)));
  cursor.insertText(
      QString("Resolution\t%1\n").arg(params.separation, 3, 'f', 2));
  cursor.insertText(QString("Isovalue\t%1\n").arg(params.isovalue));
  cursor.insertText("\n");
  cursor.insertText(QString("Volume\t%1 Å³\n").arg(mesh->volume(), 3, 'f', 2));
  cursor.insertText(
      QString("Area\t%1 Å²\n").arg(mesh->surfaceArea(), 3, 'f', 2));
  cursor.insertText(
      QString("Globularity\t%1\n").arg(mesh->globularity(), 4, 'f', 3));
  cursor.insertText(
      QString("Asphericity\t%1\n").arg(mesh->asphericity(), 4, 'f', 3));
  cursor.insertText("\n");
}

void SurfaceInfoDocument::insertMeshInstanceInformation(QTextCursor &cursor,
                                                        MeshInstance *mesh) {
  if (!mesh)
    return;
}
