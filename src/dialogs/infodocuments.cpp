#include "infodocuments.h"
#include "globals.h"
#include "molecular_wavefunction.h" // For levelOfTheoryString
#include "settings.h"
#include <fmt/core.h>
#include <occ/core/element.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General Crystal Info
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline const char *INFO_HORIZONTAL_RULE =
    "----------------------------------------------------------------------\n";

void InfoDocuments::insertGeneralCrystalInfoIntoTextDocument(
    QTextDocument *document, Scene *scene) {
  ChemicalStructure *structure = scene->chemicalStructure();
  if (!structure)
    return;

  CrystalStructure *crystal = qobject_cast<CrystalStructure *>(structure);
  if (!crystal) {
    qDebug() << "No crystal for info";
    return;
  }

  QTextCursor cursor = QTextCursor(document);
  QFileInfo fileInfo(crystal->filename());

  occ::Vec3 lengths = crystal->cellLengths();
  occ::Vec3 angles = crystal->cellAngles() * 180.0 / M_PI;

  cursor.beginEditBlock();
  int numRows = 10;

  QVector<QString> labels{"Crystal", "CIF", "Formula", "Space Group", "a",
                          "b",       "c",   "alpha",   "beta",        "gamma"};
  QVector<QString> values{
      crystal->name(),
      fileInfo.fileName(),
      crystal->chemicalFormula(),
      QString::fromStdString(crystal->spaceGroup().symbol()),
      QString("%1 %2").arg(lengths(0), 12, 'f', 6).arg(ANGSTROM_SYMBOL),
      QString("%1 %2").arg(lengths(1), 12, 'f', 6).arg(ANGSTROM_SYMBOL),
      QString("%1 %2").arg(lengths(2), 12, 'f', 6).arg(ANGSTROM_SYMBOL),
      QString("%1 %2").arg(angles(0), 12, 'f', 6).arg(DEGREE_SYMBOL),
      QString("%1 %2").arg(angles(1), 12, 'f', 6).arg(DEGREE_SYMBOL),
      QString("%1 %2").arg(angles(2), 12, 'f', 6).arg(DEGREE_SYMBOL)};
  int numCols = 2;
  QTextCharFormat boldFormat = cursor.charFormat();
  boldFormat.setFontWeight(QFont::Bold);
  QTextTable *table = createTable(cursor, numRows, numCols);
  for (int row = 0; row < numRows; row++) {
    cursor = table->cellAt(row, 0).firstCursorPosition();
    cursor.insertText(labels[row], boldFormat);
    insertRightAlignedCellValue(table, cursor, row, 1, values[row]);
  }
  cursor.endEditBlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Atomic Coordinates Info
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void InfoDocuments::insertAtomicCoordinatesIntoTextDocument(
    QTextDocument *document, Scene *scene) {
  Q_ASSERT(scene);

  QTextCursor cursor = QTextCursor(document);

  cursor.beginEditBlock();
  insertAtomicCoordinatesWithAtomDescription(cursor, scene,
                                             AtomDescription::CartesianInfo);
  insertAtomicCoordinatesWithAtomDescription(cursor, scene,
                                             AtomDescription::FractionalInfo);
  cursor.endEditBlock();
}

void InfoDocuments::insertAtomicCoordinatesWithAtomDescription(
    QTextCursor cursor, Scene *scene, AtomDescription AtomDescription) {
  ChemicalStructure *structure = scene->chemicalStructure();
  if (!structure)
    return;
  const auto selectedAtoms = structure->atomsWithFlags(AtomFlag::Selected);
  const auto unselectedAtoms =
      structure->atomsWithFlags(AtomFlag::Selected, false);
  qDebug() << "Selected atoms: " << selectedAtoms.size();
  if (selectedAtoms.size() > 0) {
    insertAtomicCoordinatesSection(cursor, "Selected Atoms", structure,
                                   selectedAtoms, AtomDescription);
  }
  qDebug() << "Unselected atoms: " << unselectedAtoms.size();
  if (unselectedAtoms.size() > 0) {
    insertAtomicCoordinatesSection(cursor, "Unselected Atoms", structure,
                                   unselectedAtoms, AtomDescription);
  }

  /*
  int numFragments = crystal->numberOfFragments();
  QString header = QString("All Atoms [%1 molecule%2]")
                       .arg(numFragments)
                       .arg(numFragments > 1 ? "s" : "");
  insertAtomicCoordinatesHeader(cursor, header, crystal->atoms().size(),
                                AtomDescription);
  foreach (int fragIndex, crystal->fragmentIndices()) {
    SymopId symopId = crystal->symopIdForFragment(fragIndex);
    if (symopId != NOSYMOP) {
      QString symopString = crystal->spaceGroup().symopAsString(symopId);
      cursor.insertText("[" + symopString + "]\n");
    }
    insertAtomicCoordinates(cursor, crystal->atomsForFragment(fragIndex),
                            AtomDescription);
  }
  */
}

void InfoDocuments::insertAtomicCoordinatesSection(
    QTextCursor cursor, QString title, ChemicalStructure *structure,
    const std::vector<GenericAtomIndex> &atoms,
    AtomDescription AtomDescription) {
  if (atoms.size() == 0) {
    return;
  }

  auto format = cursor.charFormat();
  format.setFontStyleHint(QFont::Monospace);
  insertAtomicCoordinatesHeader(cursor, title, atoms.size(), AtomDescription);
  insertAtomicCoordinates(cursor, structure, atoms, AtomDescription);
}

void InfoDocuments::insertAtomicCoordinatesHeader(
    QTextCursor cursor, QString title, int numAtoms,
    AtomDescription AtomDescription) {
  // Determine coordinate system fom AtomDescription
  QString coords;
  switch (AtomDescription) {
  case AtomDescription::CartesianInfo:
    coords = "Cartesian";
    break;
  case AtomDescription::FractionalInfo:
    coords = "fractional";
    break;
  default:
    Q_ASSERT(false);
  }

  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText(title + "\n");
  cursor.insertText(QString("%1 atom%2, %3 coordinates\n")
                        .arg(numAtoms)
                        .arg(numAtoms > 1 ? "s" : "")
                        .arg(coords));
  cursor.insertText(QString::fromStdString(
      fmt::format("{:<6s} {:<6s} {:>20s} {:>20s} {:>20s} {:>4s}\n", "Label",
                  "Symbol", "x", "y", "z", "Occ.")));
  cursor.insertText(INFO_HORIZONTAL_RULE);
}

void InfoDocuments::insertAtomicCoordinates(
    QTextCursor cursor, ChemicalStructure *structure,
    const std::vector<GenericAtomIndex> &atoms,
    AtomDescription atomDescription) {
  if (!structure)
    return;

  switch (atomDescription) {
  case AtomDescription::CartesianInfo: {
    auto nums = structure->atomicNumbersForIndices(atoms);
    auto pos = structure->atomicPositionsForIndices(atoms);
    auto labels = structure->labelsForIndices(atoms);

    for (int i = 0; i < nums.rows(); i++) {
      std::string symbol = occ::core::Element(nums(i)).symbol();
      std::string s = fmt::format(
          "{:<6s} {:<6s} {: 20.12f} {: 20.12f} {: 20.12f} {:4.3f}\n",
          labels[i].toStdString(), symbol, pos(0, i), pos(1, i), pos(2, i),
          1.0);
      cursor.insertText(QString::fromStdString(s));
    }
    break;
  }
  case AtomDescription::FractionalInfo: {
    break;
  }
  default:
    break;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Current Surface Info
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void InfoDocuments::insertCurrentSurfaceInfoIntoTextDocument(
    QTextDocument *document, Scene *scene, FingerprintBreakdown breakdown) {
  Q_ASSERT(scene);

  // TODO surface info text
  /*
  QTextCursor cursor = QTextCursor(document);
  cursor.beginEditBlock();
  Surface *surface = scene->currentSurface();

  if (!surface) {
    cursor.insertText("No current surface found.");
    return;
  }

  insertGeneralSurfaceInformation(surface, cursor);
  insertSurfacePropertyInformation(surface, cursor);
  if (surface->isFingerprintable()) {
    insertFingerprintInformation(
        breakdown, scene->crystal()->listOfElementSymbols(), cursor);
  }
  if (surface->isHirshfeldBased()) {
    insertFragmentPatchInformation(surface, cursor);
  }

  insertSupplementarySurfacePropertyInformation(surface, cursor);
  if (surface->isVoidSurface() && surface->hasCalculatedDomains()) {
    insertVoidDomainInformation(surface, cursor);
  }
  cursor.endEditBlock();
  */
}

void InfoDocuments::insertGeneralSurfaceInformation(Mesh *surface,
                                                    QTextCursor cursor) {
  /*
  const QString TITLE = "General Surface Information";

  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText(TITLE + "\n");
  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText("\n");
  cursor.insertText(QString("Type\t%1\n").arg(surface->surfaceName()));
  if (surface->type() == IsosurfaceDetails::Type::Orbital) {
    cursor.insertText(
        QString("MO\t%1\n").arg(surface->molecularOrbitalDescription()));
  }
  cursor.insertText(
      QString("Resolution\t%1\n").arg(surface->resolutionDescription()));
  cursor.insertText(QString("Isovalue\t%1\n").arg(surface->isovalue()));
  cursor.insertText("\n");
  cursor.insertText(QString("Volume\t%1 %2%3\n")
                        .arg(surface->volume(), 3, 'f', 2)
                        .arg(ANGSTROM_SYMBOL)
                        .arg(CUBED_SYMBOL));
  cursor.insertText(QString("Area\t%1 %2%3\n")
                        .arg(surface->area(), 3, 'f', 2)
                        .arg(ANGSTROM_SYMBOL)
                        .arg(SQUARED_SYMBOL));
  cursor.insertText(
      QString("Globularity\t%1\n").arg(surface->globularity(), 4, 'f', 3));
  cursor.insertText(
      QString("Asphericity\t%1\n").arg(surface->asphericity(), 4, 'f', 3));
  cursor.insertText("\n");

  insertWavefunctionInformation(surface, cursor);

  cursor.insertText("\n");
  */
}

void InfoDocuments::insertWavefunctionInformation(Mesh *surface,
                                                  QTextCursor cursor) {
  /*
if (surface->jobParameters().program == ExternalProgram::None) {
  return;
}

JobParameters jobParams = surface->jobParameters();
QString source = jobParams.programName();
QString basisset = jobParams.basisSetName();
QString method;
switch (jobParams.theory) {
case Method::hartreeFock:
case Method::mp2:
case Method::b3lyp:
  method = methodLabels[static_cast<int>(jobParams.theory)];
  break;
case Method::kohnSham:
  method =
      exchangePotentialLabels[static_cast<int>(jobParams.exchangePotential)] +
      correlationPotentialLabels[static_cast<int>(
          jobParams.correlationPotential)];
  break;
default:
  throw std::runtime_error("Invalid in insertWavefunctionInformation");
}
cursor.insertText(QString("Wavefunc.\t") + method + "/" + basisset + "\n");
cursor.insertText(QString("Source\t" + source + "\n"));
cursor.insertText(QString("Charge\t%1\n").arg(jobParams.charge));
cursor.insertText(QString("Multiplicity\t%1\n").arg(jobParams.multiplicity));
*/
}

void InfoDocuments::insertSurfacePropertyInformation(Mesh *surface,
                                                     QTextCursor cursor) {
  /*
  QVector<IsosurfaceDetails::Type> surfacesToSkip =
      QVector<IsosurfaceDetails::Type>()
      << IsosurfaceDetails::Type::CrystalVoid;
  if (surfacesToSkip.contains(surface->type())) {
    return;
  }

  QVector<IsosurfacePropertyDetails::Type> propertiesToSkip =
      QVector<IsosurfacePropertyDetails::Type>()
      << IsosurfacePropertyDetails::Type::None
      << IsosurfacePropertyDetails::Type::FragmentPatch;

  const int WIDTH = 4;
  const int PRECISION = 3;

  const QString TITLE = "Surface Property Information";

  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText(TITLE + "\n");
  cursor.insertText(
      QString("%1 Properties\n").arg(surface->numberOfProperties()));
  cursor.insertText("Name\tMin\tMean\tMax\tUnits\n");
  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText("\n");
  for (int i = 0; i < surface->numberOfProperties(); ++i) {
    const SurfaceProperty *property = surface->propertyAtIndex(i);
    if (propertiesToSkip.contains(property->type())) {
      continue;
    }

    cursor.insertText(QString("%1\t%2\t%3\t%4\t%5\n")
                          .arg(property->propertyName())
                          .arg(property->min(), WIDTH, 'f', PRECISION)
                          .arg(property->mean(), WIDTH, 'f', PRECISION)
                          .arg(property->max(), WIDTH, 'f', PRECISION)
                          .arg(property->units()));
  }
  cursor.insertText("\n");
  */
}

void InfoDocuments::insertFingerprintInformation(
    FingerprintBreakdown fingerprintBreakdown, QStringList elementSymbols,
    QTextCursor cursor) {
  const QString TITLE = "Fingerprint Breakdown";

  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText(TITLE + "\n\n");
  cursor.insertText("Filtering fingerprint by element type.\n");
  cursor.insertText(
      "Surface area included (as percentage of the total surface area)\n");
  cursor.insertText(
      "for close contacts between atoms inside and outside the surface.\n");
  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText("\n");

  cursor.insertText("Inside\tOutside Atom\n");
  cursor.insertText(QString("Atom\t") + elementSymbols.join("\t") + "\n");

  QVector<double> columnTotals(elementSymbols.size());
  columnTotals.fill(0.0);

  QString rowString;

  foreach (QString outsideElementSymbol, fingerprintBreakdown.keys()) {
    rowString = outsideElementSymbol;
    double totalAreaForRow = 0.0;
    QVector<double> rowAreas = fingerprintBreakdown[outsideElementSymbol];
    for (int i = 0; i < rowAreas.size(); ++i) {
      double area = rowAreas[i];
      totalAreaForRow += area;
      columnTotals[i] = columnTotals[i] + area;
      QString value = area > 0.0 ? QString("%1").arg(area, 0, 'f', 1) : ".";
      rowString += QString("\t") + value;
    }

    // Add row totals onto end
    rowString += QString("\t%1").arg(totalAreaForRow, 0, 'f', 1);
    cursor.insertText(rowString + "\n");
  }

  // Output column totals
  rowString = "";
  for (int i = 0; i < columnTotals.size(); ++i) {
    double area = columnTotals[i];
    rowString += QString("\t%1").arg(area, 0, 'f', 1);
  }
  cursor.insertText(rowString + "\n");

  cursor.insertText("\n");
}

void InfoDocuments::insertFragmentPatchInformation(Mesh *surface,
                                                   QTextCursor cursor) {
  /*
QVector<double> areas = surface->areasOfFragmentPatches();
QVector<QColor> colors = surface->colorsOfFragmentPatches();
Q_ASSERT(areas.size() == colors.size());
int numFragments = areas.size();

// Properties that can be summed over patch
QVector<QVector<float>> patchProperties;
QStringList propertyHeaders;

const QString TITLE = "Fragment Patch Information";
cursor.insertText(INFO_HORIZONTAL_RULE);
cursor.insertText(TITLE + "\n");
cursor.insertText(QString("%1 Fragment Patches\n").arg(numFragments));
cursor.insertText(INFO_HORIZONTAL_RULE);

// Define Table Header
QString areaString =
    QString("Area /%2%3").arg(ANGSTROM_SYMBOL).arg(SQUARED_SYMBOL);
QStringList tableHeader = QStringList() << "" << areaString;
tableHeader.append(propertyHeaders);
int numHeaderLines = 1;

// Create table
int numRows = numHeaderLines + numFragments;
QTextTable *table = createTable(cursor, numRows, tableHeader.size());

// Insert Table Header
insertTableHeader(table, cursor, tableHeader);

const float SCALE_FACTOR = 100.0;
int row = 1;
for (int i = 0; i < numFragments; ++i) {
  int col = 0;
  insertColorBlock(table, cursor, row, col++, colors[i]);
  insertRightAlignedCellValue(table, cursor, row, col++,
                              QString("%1").arg(areas[i], 0, 'f', 1));
  foreach (QVector<float> patchProperty, patchProperties) {
    insertRightAlignedCellValue(
        table, cursor, row, col++,
        QString("%1").arg(patchProperty[i] * SCALE_FACTOR, 0, 'f', 3));
  }
  row++;
}
*/
}

void InfoDocuments::insertSupplementarySurfacePropertyInformation(
    Mesh *surface, QTextCursor cursor) {
  /*
QVector<IsosurfaceDetails::Type> surfacesToSkip =
    QVector<IsosurfaceDetails::Type>()
    << IsosurfaceDetails::Type::CrystalVoid;
if (surfacesToSkip.contains(surface->type())) {
  return;
}

QVector<IsosurfacePropertyDetails::Type> propertiesToSkip =
    QVector<IsosurfacePropertyDetails::Type>()
    << IsosurfacePropertyDetails::Type::None
    << IsosurfacePropertyDetails::Type::FragmentPatch;

const int WIDTH = 4;
const int PRECISION = 3;

const QString TITLE = "Supplementary Surface Property Statistics";

QStringList statisticsLabels = surface->statisticsLabels();
statisticsLabels.prepend("Name");

cursor.insertText(INFO_HORIZONTAL_RULE);
cursor.insertText(TITLE + "\n");
cursor.insertText(statisticsLabels.join("\t") + "\n");
cursor.insertText(INFO_HORIZONTAL_RULE);
cursor.insertText("\n");
for (int i = 0; i < surface->numberOfProperties(); ++i) {
  const SurfaceProperty *property = surface->propertyAtIndex(i);
  if (propertiesToSkip.contains(property->type())) {
    continue;
  }

  QStringList valueStrings;
  foreach (double value, property->getStatistics().values()) {
    valueStrings.append(QString("%1").arg(value, WIDTH, 'g', PRECISION));
  }
  valueStrings.prepend(property->propertyName());
  valueStrings.replaceInStrings("nan", "~");
  cursor.insertText(valueStrings.join("\t") + "\n");
}
cursor.insertText("\n");
*/
}

void InfoDocuments::insertVoidDomainInformation(Mesh *surface,
                                                QTextCursor cursor) {
  /*
// Get values for table
QVector<QColor> domainColors = surface->domainColors();
QVector<double> domainVolumes = surface->domainVolumes();
QVector<double> domainSurfaceAreas = surface->domainSurfaceAreas();
int numDomains = domainColors.size();
Q_ASSERT(numDomains != 0);
Q_ASSERT(numDomains == domainVolumes.size());
Q_ASSERT(numDomains == domainSurfaceAreas.size());

// Insert header
cursor.insertText(INFO_HORIZONTAL_RULE);
cursor.insertText("Void Domains\n");
cursor.insertText(QString("%1 domains\n").arg(numDomains));
cursor.insertText(INFO_HORIZONTAL_RULE);
cursor.insertText("\n");

// Define Table Header
QString areaString =
    QString("Surface Area /%1%2").arg(ANGSTROM_SYMBOL).arg(SQUARED_SYMBOL);
QString volumeString =
    QString("Volume /%1%2").arg(ANGSTROM_SYMBOL).arg(CUBED_SYMBOL);
QStringList tableHeader = QStringList() << "" << areaString << volumeString;
int numHeaderLines = 1;

// Create table
int numRows = numHeaderLines + numDomains;
QTextTable *table = createTable(cursor, numRows, tableHeader.size());

// Insert Table Header
insertTableHeader(table, cursor, tableHeader);

int row = 1;
for (int d = 0; d < domainColors.size(); ++d) {
  insertDomainAtTableRow(row, table, cursor, domainColors[d],
                         domainSurfaceAreas[d], domainVolumes[d]);
  row++;
}
cursor.movePosition(QTextCursor::End);
cursor.insertText("\n\n");
*/
}

void InfoDocuments::insertDomainAtTableRow(int row, QTextTable *table,
                                           QTextCursor cursor,
                                           QColor domainColor,
                                           double surfaceArea, double volume) {
  const int PRECISION = 2;
  const int WIDTH = 0;

  int column = 0;

  QTextTableCell cell = table->cellAt(row, column++);
  if (domainColor.isValid()) {
    QTextCharFormat format = cell.format();
    format.setBackground(domainColor);
    cell.setFormat(format);
  }
  cursor = cell.firstCursorPosition();
  cursor.insertText("     ");

  cursor = table->cellAt(row, column++).firstCursorPosition();
  cursor.insertText(QString("%1").arg(surfaceArea, WIDTH, 'f', PRECISION));

  cursor = table->cellAt(row, column++).firstCursorPosition();
  cursor.insertText(QString("%1").arg(volume, WIDTH, 'f', PRECISION));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Interaction Energy Info
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void InfoDocuments::insertInteractionEnergiesIntoTextDocument(
    QTextDocument *document, Scene *scene) {
  ChemicalStructure *structure = scene->chemicalStructure();

  if (!structure)
    return;

  QTextCursor cursor = QTextCursor(document);
  qDebug() << "Made cursor";
  auto *interactions = structure->pairInteractions();
  qDebug() << "have interactions";

  if (interactions->getCount() > 0) {
    FragmentPairSettings pairSettings;
    pairSettings.allowInversion = false;
    scene->colorFragmentsByEnergyPair();
    // These must be here for performance!
    cursor.beginEditBlock();
    insertInteractionEnergiesGroupedByPair(interactions, cursor);
    insertEnergyModelScalingInfo(cursor);
    cursor.endEditBlock();
  } else {
    cursor.insertText("No interaction energies found.");
  }
}

void InfoDocuments::insertEnergyModelScalingInfo(QTextCursor cursor) {
  /*
const int SF_PRECISION = 3; // Precision of scale factors in table
const int SF_WIDTH = 6;     // Field width of scale factors in table

// Insert header
cursor.insertBlock();
cursor.insertHtml("<h2>Scale factors for benchmarked energy models</h2>");
cursor.insertBlock();
cursor.insertHtml("See <em>Mackenzie et al. IUCrJ (2017)</em>");

// Define table header
QStringList tableHeader{"Energy Model", "k_ele", "k_pol", "k_disp", "k_rep"};
int numHeaderLines = 1;

QVector<EnergyModel> energyModels =
    QVector<EnergyModel>() << EnergyModel::CE_HF << EnergyModel::CE_B3LYP;

// Create table
int numRows = numHeaderLines + energyModels.size();
QTextTable *table = createTable(cursor, numRows, tableHeader.size());

// Insert Table Header
insertTableHeader(table, cursor, tableHeader);

// Insert rows of data...
int row = 1;
foreach (EnergyModel model, energyModels) {
  int column = 0;

  cursor = table->cellAt(row, column++).firstCursorPosition();
  cursor.insertText(EnergyDescription::fullDescription(model));

  insertRightAlignedCellValue(table, cursor, row, column++,
                              QString("%1").arg(coulombScaleFactors[model],
                                                SF_WIDTH, 'f', SF_PRECISION));
  insertRightAlignedCellValue(
      table, cursor, row, column++,
      QString("%1").arg(polarizationScaleFactors[model], SF_WIDTH, 'f',
                        SF_PRECISION));
  insertRightAlignedCellValue(table, cursor, row, column++,
                              QString("%1").arg(dispersionScaleFactors[model],
                                                SF_WIDTH, 'f', SF_PRECISION));
  insertRightAlignedCellValue(table, cursor, row, column++,
                              QString("%1").arg(repulsionScaleFactors[model],
                                                SF_WIDTH, 'f', SF_PRECISION));

  row++;
}

cursor.movePosition(QTextCursor::End);
*/
}

void InfoDocuments::insertEnergyScalingPreamble(QTextCursor cursor) {
  QTextBlockFormat regularFormat = cursor.blockFormat();
  QTextListFormat listFormat;
  listFormat.setStyle(QTextListFormat::ListDisc);
  listFormat.setIndent(1);
  cursor.insertList(listFormat);
  cursor.insertHtml("All energies are reported in kJ/mol");
  cursor.insertBlock();
  cursor.insertHtml("<b>R</b> is the distance between molecular centroids "
                    "(mean atomic position) in Å, and <b>N</b> is the number "
                    "of symmetry-equivalent molecular dimers.");

  cursor.insertBlock();
  cursor.insertHtml("For <em>CrystalExplorer</em> (CE) model energies, the "
                    "total energy is only reported for two benchmarked "
                    "energy models, and is are the sum of the four energy "
                    "components, scaled appropriately (see the "
                    "scale factor table below)");

  cursor.insertBlock();
  cursor.insertHtml("For other energies, the total energy is not a scaled sum, "
                    "and not all columns will have values, "
                    "in these cases a filler value of 0.0 is used throughout");

  cursor.insertBlock();
  cursor.insertHtml(
      "It's extremely important to note that energy components between "
      "different methods are likely not "
      "directly comparable. <em>Always</em> check the definitions of each "
      "component from the scientific works, and "
      "remember that the total interaction energy is likely the only number "
      "with an agreed upon definition.");

  cursor.insertBlock();
  cursor.setBlockFormat(regularFormat);
}

QList<QString> getOrderedComponents(QSet<QString> uniqueComponents) {
  QList<QString> knownComponentsOrder;
  knownComponentsOrder << "coulomb"
                       << "repulsion"
                       << "exchange"
                       << "dispersion";

  QList<QString> sortedComponents;

  // Add known components in the desired order
  for (const QString &component : knownComponentsOrder) {
    if (uniqueComponents.contains(component)) {
      sortedComponents << component;
      uniqueComponents.remove(component);
    }
  }

  // Add remaining components (excluding "total") in ascending order
  QList<QString> remainingComponents = uniqueComponents.values();
  remainingComponents.removeOne("total");
  std::sort(remainingComponents.begin(), remainingComponents.end());
  sortedComponents << remainingComponents;

  // Add "total" component at the end if it exists
  if (uniqueComponents.contains("total")) {
    sortedComponents << "total";
  }
  return sortedComponents;
}

void InfoDocuments::insertInteractionEnergiesGroupedByPair(PairInteractions *results, QTextCursor cursor) {
  if(!results) return;
  const int eprec =
      settings::readSetting(settings::keys::ENERGY_TABLE_PRECISION).toInt();

  // Insert header
  cursor.insertHtml("<h1>Interaction Energies</h1>");
  insertEnergyScalingPreamble(cursor);

  QList<QString> sortedModels = results->interactionModels();
  std::sort(sortedModels.begin(), sortedModels.end());

  QSet<QString> uniqueComponents;
  for (const QString &model : sortedModels) {
    for (const auto &[index, result] : results->filterByModel(model)) {
      for (const auto &component : result->components()) {
        uniqueComponents.insert(component.first);
      }
    }
  }

  QList<QString> sortedComponents = getOrderedComponents(uniqueComponents);
  QStringList tableHeader{"Color", "Model", "Distance", "Symmetry"};
  tableHeader.append(sortedComponents);

  int numHeaderLines = 1;
  int totalResults = results->getCount();
  int numLines = numHeaderLines + totalResults;
  QTextTable *table = createTable(cursor, numLines, tableHeader.size());

  insertTableHeader(table, cursor, tableHeader);


  int row = 1;
  for (const QString &model : sortedModels) {
    int interactionIndex = 0;
    for (const auto &[index, result] : results->filterByModel(model)) {
      insertColorBlock(table, cursor, row, 0, result->color());

      insertRightAlignedCellValue(table, cursor, row, 1, model);

      insertRightAlignedCellValue(
          table, cursor, row, 2,
          QString::number(result->centroidDistance(), 'f', 2));
      insertRightAlignedCellValue(table, cursor, row, 3, result->symmetry());

      int column = 4;
      for (const QString &component : sortedComponents) {
        bool found = false;
        for (const auto &pair : result->components()) {
          if (pair.first == component) {
            insertRightAlignedCellValue(
                table, cursor, row, column,
                QString("%1").arg(pair.second, 6, 'f', eprec));
            found = true;
            break;
          }
        }
        if (!found) {
          insertRightAlignedCellValue(table, cursor, row, column, "-");
        }
        column++;
      }
      row++;
      interactionIndex++;
    }
  }
  cursor.movePosition(QTextCursor::End);
  cursor.insertText("\n\n");
}

void InfoDocuments::insertLatticeEnergy(Scene *scene, QTextCursor cursor) {
  qDebug() << "insertLatticeEnergy";
  /*
DeprecatedCrystal *crystal = scene->crystal();
if (!crystal)
  return;
const auto theories = crystal->levelsOfTheoriesForLatticeEnergies();
const auto energies = crystal->latticeEnergies();

Q_ASSERT(theories.size() == energies.size());

for (int i = 0; i < theories.size(); i++) {
  QString latticeEnergyLine =
      QString("Estimated Lattice Energy [%1]:\t%2 kJ/mol\n")
          .arg(theories[i])
          .arg(energies[i]);
  cursor.insertText(latticeEnergyLine);
}
cursor.insertText("\n\n");
  */
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Support Routines
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QTextTable *InfoDocuments::createTable(QTextCursor cursor, int numRows,
                                       int numColumns) {
  QTextTable *table = cursor.insertTable(numRows, numColumns);
  QTextTableFormat tableFormat = table->format();
  tableFormat.setCellPadding(5.0);
  tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_None);
  tableFormat.setCellSpacing(-1.0);
  tableFormat.setBorder(1.0);
  table->setFormat(tableFormat);
  return table;
}

void InfoDocuments::insertTableHeader(QTextTable *table, QTextCursor cursor,
                                      QStringList tableHeader) {
  const int row = 0;
  QTextCharFormat format = table->cellAt(0, 0).format();
  format.setFontWeight(QFont::Bold);
  for (int column = 0; column < tableHeader.size(); column++) {
    cursor = table->cellAt(row, column).firstCursorPosition();
    cursor.setCharFormat(format);
    cursor.insertText(tableHeader[column]);
  }
}

void InfoDocuments::insertColorBlock(QTextTable *table, QTextCursor cursor,
                                     int row, int column, QColor color) {
  QTextTableCell cell = table->cellAt(row, column);
  if (color.isValid()) {
    QTextCharFormat format = cell.format();
    format.setBackground(color);
    cell.setFormat(format);
  }
  cursor = cell.firstCursorPosition();
  cursor.insertText("     ");
}

void InfoDocuments::insertRightAlignedCellValue(QTextTable *table,
                                                QTextCursor cursor, int row,
                                                int column,
                                                QString valueString) {
  cursor = table->cellAt(row, column).firstCursorPosition();

  QTextBlockFormat blockFormat = cursor.blockFormat();
  Qt::Alignment vertAlign = blockFormat.alignment() & Qt::AlignVertical_Mask;
  Qt::Alignment combAlign = Qt::AlignRight | vertAlign;
  blockFormat.setAlignment(combAlign);
  cursor.setBlockFormat(blockFormat);

  cursor.insertText(valueString);
}
