#include "crystalinfodocument.h"
#include "crystalstructure.h"
#include "globals.h"
#include <QFont>
#include <QTextCursor>
#include <fmt/format.h>

using cx::globals::angstromSymbol;
using cx::globals::degreeSymbol;

inline const char *INFO_HORIZONTAL_RULE =
    "--------------------------------------------------------------------------"
    "------------\n";

CrystalInfoDocument::CrystalInfoDocument(QWidget *parent) : QWidget(parent) {
  setupUI();
  populateDocument();
}

void CrystalInfoDocument::setupUI() {
  QVBoxLayout *layout = new QVBoxLayout(this);

  QFont monoFont("Courier");
  monoFont.setStyleHint(QFont::Monospace);
  monoFont.setFixedPitch(true);

  m_contents = new QTextEdit(this);
  m_contents->document()->setDefaultFont(monoFont);
  layout->addWidget(m_contents);
}

void CrystalInfoDocument::populateDocument() {
  if (!m_scene)
    return;
  m_contents->clear();

  ChemicalStructure *structure = m_scene->chemicalStructure();
  if (!structure)
    return;

  CrystalStructure *crystal = qobject_cast<CrystalStructure *>(structure);
  if (!crystal)
    return;

  QTextCursor cursor = m_contents->textCursor();
  cursor.beginEditBlock();
  insertGeneralInformation(cursor, crystal);
  cursor.endEditBlock();
  resetCursorToBeginning();
}

void CrystalInfoDocument::resetCursorToBeginning() {
  QTextCursor cursor = m_contents->textCursor();
  cursor.movePosition(QTextCursor::Start);
  m_contents->setTextCursor(cursor);
  m_contents->ensureCursorVisible();
}

void CrystalInfoDocument::updateScene(Scene *scene) {
  m_scene = scene;
  populateDocument();
}

void CrystalInfoDocument::insertGeneralInformation(QTextCursor &cursor,
                                                   CrystalStructure *crystal) {
  QFileInfo fileInfo(crystal->filename());

  occ::Vec3 lengths = crystal->cellLengths();
  occ::Vec3 angles = crystal->cellAngles() * 180.0 / M_PI;

  cursor.insertText(QString::fromStdString(fmt::format(
      "{:<12s} {:>12s}\n", "Crystal", crystal->name().toStdString())));
  cursor.insertText(QString::fromStdString(fmt::format(
      "{:<12s} {:>12s}\n", "CIF", fileInfo.fileName().toStdString())));
  cursor.insertText(QString::fromStdString(
      fmt::format("{:<12s} {:>12s}\n", "Formula",
                  crystal->chemicalFormula().toStdString())));
  cursor.insertText(QString::fromStdString(fmt::format(
      "{:<12s} {:>12s}\n", "Space Group", crystal->spaceGroup().symbol())));
  cursor.insertText(QString::fromStdString(fmt::format(
      "{:<12s} {: 12.6f} {}\n", "Length A", lengths(0), angstromSymbol)));
  cursor.insertText(QString::fromStdString(fmt::format(
      "{:<12s} {: 12.6f} {}\n", "Length B", lengths(1), angstromSymbol)));
  cursor.insertText(QString::fromStdString(fmt::format(
      "{:<12s} {: 12.6f} {}\n", "Length C", lengths(2), angstromSymbol)));

  cursor.insertText(QString::fromStdString(fmt::format(
      "{:<12s} {: 12.6f} {}\n", "Angle Alpha", angles(0), degreeSymbol)));
  cursor.insertText(QString::fromStdString(fmt::format(
      "{:<12s} {: 12.6f} {}\n", "Angle Beta", angles(1), degreeSymbol)));
  cursor.insertText(QString::fromStdString(fmt::format(
      "{:<12s} {: 12.6f} {}\n", "Angle Gamma", angles(2), degreeSymbol)));
}
