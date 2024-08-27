#include "atominfodocument.h"
#include <QFont>
#include <QTextCursor>
#include <fmt/format.h>

inline const char *INFO_HORIZONTAL_RULE =
    "--------------------------------------------------------------------------"
    "------------\n";

AtomInfoDocument::AtomInfoDocument(QWidget *parent) : QWidget(parent) {
  setupUI();
  populateDocument();
}

void AtomInfoDocument::setupUI() {
  QVBoxLayout *layout = new QVBoxLayout(this);
  m_tabWidget = new QTabWidget(this);
  layout->addWidget(m_tabWidget);

  QFont monoFont("Courier");
  monoFont.setStyleHint(QFont::Monospace);
  monoFont.setFixedPitch(true);

  m_cartesianCoordinates = new QTextEdit(this);
  m_cartesianCoordinates->document()->setDefaultFont(monoFont);
  m_fractionalCoordinates = new QTextEdit(this);
  m_fractionalCoordinates->document()->setDefaultFont(monoFont);

  m_tabWidget->addTab(m_cartesianCoordinates, "Cartesian");
  m_tabWidget->addTab(m_fractionalCoordinates, "Fractional");
}


void AtomInfoDocument::populateDocument() {
  if (!m_scene)
    return;
  m_cartesianCoordinates->clear();
  m_fractionalCoordinates->clear();
  insertAtomicCoordinates(m_cartesianCoordinates, AtomDescription::CartesianInfo);
  insertAtomicCoordinates(m_fractionalCoordinates, AtomDescription::FractionalInfo);
  resetCursorsToBeginning();
}

void AtomInfoDocument::resetCursorsToBeginning()
{
    QTextCursor cartesianCursor = m_cartesianCoordinates->textCursor();
    cartesianCursor.movePosition(QTextCursor::Start);
    m_cartesianCoordinates->setTextCursor(cartesianCursor);
    m_cartesianCoordinates->ensureCursorVisible();

    QTextCursor fractionalCursor = m_fractionalCoordinates->textCursor();
    fractionalCursor.movePosition(QTextCursor::Start);
    m_fractionalCoordinates->setTextCursor(fractionalCursor);
    m_fractionalCoordinates->ensureCursorVisible();
}

void AtomInfoDocument::updateScene(Scene *scene) {
  m_scene = scene;
  populateDocument();
}

void AtomInfoDocument::insertAtomicCoordinates(
    QTextEdit *textEdit, AtomDescription atomDescription) {

  ChemicalStructure *structure = m_scene->chemicalStructure();
  if (!structure)
    return;

  const auto selectedAtoms = structure->atomsWithFlags(AtomFlag::Selected);
  const auto unselectedAtoms =
      structure->atomsWithFlags(AtomFlag::Selected, false);

  if (!selectedAtoms.empty()) {
    insertAtomicCoordinatesSection(textEdit, "Selected Atoms", structure,
                                   selectedAtoms, atomDescription);
  }

  if (!unselectedAtoms.empty()) {
    insertAtomicCoordinatesSection(textEdit, "Unselected Atoms", structure,
                                   unselectedAtoms, atomDescription);
  }
}

void AtomInfoDocument::insertAtomicCoordinatesSection(
    QTextEdit *textEdit,
    const QString &title, ChemicalStructure *structure,
    const std::vector<GenericAtomIndex> &atoms,
    AtomDescription atomDescription) {
  if (atoms.empty()) {
    return;
  }

  bool frac = (atomDescription == AtomDescription::FractionalInfo);
  if (frac &&
      (structure->structureType() == ChemicalStructure::StructureType::Cluster))
    return;

  QTextCursor cursor(textEdit->document());
  cursor.movePosition(QTextCursor::End);

  cursor.beginEditBlock();
  insertAtomicCoordinatesHeader(cursor, title, atoms.size(), atomDescription);
  insertAtomicCoordinates(cursor, structure, atoms, atomDescription);
  cursor.endEditBlock();
}

void AtomInfoDocument::insertAtomicCoordinatesHeader(
    QTextCursor &cursor, const QString &title, int numAtoms,
    AtomDescription atomDescription) {
  bool frac = atomDescription == AtomDescription::FractionalInfo;
  QString coords = frac ? "fractional" : "Cartesian";

  cursor.insertText(title + "\n");
  cursor.insertText(QString("%1 atom%2, %3 coordinates\n")
                        .arg(numAtoms)
                        .arg(numAtoms > 1 ? "s" : "")
                        .arg(coords));

  cursor.insertText(QString::fromStdString(
      fmt::format("{:<6s} {:<6s} {:>20s} {:>20s} {:>20s} {:>8s}\n", "Label",
                  "Symbol", "x", "y", "z", "Occ")));
  cursor.insertText(INFO_HORIZONTAL_RULE);
}

void AtomInfoDocument::insertAtomicCoordinates(
    QTextCursor &cursor, ChemicalStructure *structure,
    const std::vector<GenericAtomIndex> &atoms,
    AtomDescription atomDescription) {
  if (!structure)
    return;

  auto nums = structure->atomicNumbersForIndices(atoms);
  auto pos = structure->atomicPositionsForIndices(atoms);
  if (atomDescription == AtomDescription::FractionalInfo) {
    pos = structure->convertCoordinates(
        pos, ChemicalStructure::CoordinateConversion::CartToFrac);
  }
  auto labels = structure->labelsForIndices(atoms);

  for (int i = 0; i < nums.rows(); i++) {
    std::string symbol = occ::core::Element(nums(i)).symbol();

    cursor.insertText(QString::fromStdString(
        fmt::format("{:<6s} {:<6s} {: 20.12f} {: 20.12f} {: 20.12f} {: 8.3f}\n",
                    labels[i].toStdString(), symbol, pos(0, i), pos(1, i),
                    pos(2, i), 1.0)));
  }
}
