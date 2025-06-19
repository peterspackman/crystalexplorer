#include "surfaceinfodocument.h"
#include "globals.h"
#include "fingerprintcalculator.h"
#include "isosurface.h"
#include "chemicalstructure.h"
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
  insertPropertyInformation(cursor, mesh);
  insertFingerprintBreakdown(cursor, mesh);
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

  const QString TITLE = "General Surface Information";

  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText(TITLE + "\n");
  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText("\n");
  
  const auto &attr = mesh->attributes();
  cursor.insertText(
      QString("Type\t\t%1\n").arg(isosurface::kindToString(attr.kind)));
  cursor.insertText(
      QString("Resolution\t%1 Å\n").arg(attr.separation, 3, 'f', 2));
  cursor.insertText(QString("Isovalue\t\t%1\n").arg(attr.isovalue));
  cursor.insertText("\n");
  
  cursor.insertText(QString("Vertices\t\t%1\n").arg(mesh->numberOfVertices()));
  cursor.insertText(QString("Faces\t\t%1\n").arg(mesh->numberOfFaces()));
  cursor.insertText("\n");
  
  cursor.insertText(QString("Volume\t\t%1 Å³\n").arg(mesh->volume(), 3, 'f', 2));
  cursor.insertText(
      QString("Surface Area\t%1 Å²\n").arg(mesh->surfaceArea(), 3, 'f', 2));
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

void SurfaceInfoDocument::insertPropertyInformation(QTextCursor &cursor, Mesh *mesh) {
  if (!mesh)
    return;
    
  const QString TITLE = "Surface Properties";
  
  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText(TITLE + "\n");
  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText("\n");
  
  // Get available properties
  const auto &properties = mesh->vertexProperties();
  
  for (const auto &[propName, propData] : properties) {
    if (propName.contains("_idx") || propName.contains("fragment")) {
      continue; // Skip index properties
    }
    
    if (propData.rows() == 0) continue;
    
    // Try to get as double array for statistics
    if (propData.cols() == 1) {
      auto doubleProp = propData.cast<double>();
      if (doubleProp.rows() > 0) {
        double minVal = doubleProp.minCoeff();
        double maxVal = doubleProp.maxCoeff();
        double meanVal = doubleProp.mean();
        
        cursor.insertText(QString("%1:\n").arg(propName));
        cursor.insertText(QString("  Range\t\t[%1, %2]\n").arg(minVal, 6, 'f', 3).arg(maxVal, 6, 'f', 3));
        cursor.insertText(QString("  Mean\t\t%1\n").arg(meanVal, 6, 'f', 3));
        cursor.insertText("\n");
      }
    }
  }
}

void SurfaceInfoDocument::insertFingerprintBreakdown(QTextCursor &cursor, Mesh *mesh) {
  if (!mesh)
    return;
    
  // Check if this is a fingerprintable surface (Hirshfeld)
  const auto &attr = mesh->attributes();
  if (attr.kind != isosurface::Kind::Hirshfeld)
    return;
    
  // Check if we have the required properties for fingerprint analysis
  QString diName = isosurface::getSurfacePropertyDisplayName("di");
  QString deName = isosurface::getSurfacePropertyDisplayName("de");
  
  if (!mesh->haveVertexProperty(diName) || !mesh->haveVertexProperty(deName))
    return;
    
  auto *structure = qobject_cast<ChemicalStructure *>(mesh->parent());
  if (!structure)
    return;
    
  QStringList elementSymbols = structure->uniqueElementSymbols();
  if (elementSymbols.isEmpty())
    return;
    
  const QString TITLE = "Fingerprint Breakdown";
  
  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText(TITLE + "\n");
  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText("\n");
  
  cursor.insertText("Contact analysis based on Hirshfeld surface partitioning:\n\n");
  
  // Create a fingerprint calculator to calculate the breakdown
  FingerprintCalculator calculator(mesh);
  
  // Create breakdown table
  cursor.insertText(QString("Inside  "));
  for (const QString &outsideElement : elementSymbols) {
    cursor.insertText(QString("%1").arg(outsideElement, 8));
  }
  cursor.insertText(QString("%1\n").arg("Total", 8));
  
  cursor.insertText(QString("Element "));
  for (int i = 0; i < elementSymbols.size(); ++i) {
    cursor.insertText(QString("%1").arg("(%)", 8));
  }
  cursor.insertText(QString("%1\n").arg("(%)", 8));
  
  cursor.insertText(QString("%1\n").arg(QString("-").repeated(8 + elementSymbols.size() * 8 + 8)));
  
  double grandTotal = 0.0;
  
  for (const QString &insideElement : elementSymbols) {
    QVector<double> percentages = calculator.calculateElementBreakdown(insideElement, elementSymbols);
    
    cursor.insertText(QString("%1").arg(insideElement, -8));
    
    double rowTotal = 0.0;
    for (double percentage : percentages) {
      cursor.insertText(QString("%1 ").arg(percentage, 7, 'f', 1));
      rowTotal += percentage;
    }
    
    cursor.insertText(QString("%1\n").arg(rowTotal, 7, 'f', 1));
    grandTotal += rowTotal;
  }
  
  cursor.insertText(QString("%1\n").arg(QString("-").repeated(8 + elementSymbols.size() * 8 + 8)));
  cursor.insertText(QString("Total   "));
  
  // Calculate column totals
  for (int col = 0; col < elementSymbols.size(); ++col) {
    double columnTotal = 0.0;
    for (const QString &insideElement : elementSymbols) {
      QVector<double> percentages = calculator.calculateElementBreakdown(insideElement, elementSymbols);
      if (col < percentages.size()) {
        columnTotal += percentages[col];
      }
    }
    cursor.insertText(QString("%1 ").arg(columnTotal, 7, 'f', 1));
  }
  
  cursor.insertText(QString("%1\n\n").arg(grandTotal, 7, 'f', 1));
  
  cursor.insertText("Note: Percentages represent the fraction of total surface area\n");
  cursor.insertText("for each type of intermolecular contact.\n\n");
}
