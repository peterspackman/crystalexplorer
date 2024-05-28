#include "infodocuments.h"
#include "energydescription.h" // For description(EnergyModel)
#include "settings.h"
#include "wavefunction.h" // For levelOfTheoryString

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General Crystal Info
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void InfoDocuments::insertGeneralCrystalInfoIntoTextDocument(
    QTextDocument *document, Scene *scene) {
  DeprecatedCrystal *crystal = scene->crystal();
  if (!crystal)
    return;
  Q_ASSERT(crystal);

  QTextCursor cursor = QTextCursor(document);
  QFileInfo fileInfo(crystal->cifFilename());
  UnitCell cell = crystal->unitCell();

  cursor.beginEditBlock();
  int numRows = 10;

  QVector<QString> labels{"Crystal", "CIF", "Formula", "Space Group", "a",
                          "b",       "c",   "alpha",   "beta",        "gamma"};
  QVector<QString> values{
      crystal->crystalName(),
      fileInfo.fileName(),
      crystal->formula(),
      crystal->spaceGroup().symbol(),
      QString("%1 %2").arg(cell.a(), 12, 'f', 6).arg(ANGSTROM_SYMBOL),
      QString("%1 %2").arg(cell.b(), 12, 'f', 6).arg(ANGSTROM_SYMBOL),
      QString("%1 %2").arg(cell.c(), 12, 'f', 6).arg(ANGSTROM_SYMBOL),
      QString("%1 %2").arg(cell.alpha(), 12, 'f', 6).arg(DEGREE_SYMBOL),
      QString("%1 %2").arg(cell.beta(), 12, 'f', 6).arg(DEGREE_SYMBOL),
      QString("%1 %2").arg(cell.gamma(), 12, 'f', 6).arg(DEGREE_SYMBOL)};
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
  DeprecatedCrystal *crystal = scene->crystal();
  if (!crystal)
    return;
  if (crystal->hasSelectedAtoms()) {
    QVector<Atom> atoms = crystal->atoms();
    auto partition_point =
        std::partition(atoms.begin(), atoms.end(),
                       [](const Atom &a) { return a.isSelected(); });
    QVector<Atom> selectedAtoms(partition_point, atoms.end());
    atoms.erase(partition_point, atoms.end());
    insertAtomicCoordinatesSection(cursor, "Selected Atoms", selectedAtoms,
                                   AtomDescription);
    insertAtomicCoordinatesSection(cursor, "Unselected Atoms", atoms,
                                   AtomDescription);
  }

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
}

void InfoDocuments::insertAtomicCoordinatesSection(
    QTextCursor cursor, QString title, const QVector<Atom> &atoms,
    AtomDescription AtomDescription) {
  if (atoms.size() == 0) {
    return;
  }

  insertAtomicCoordinatesHeader(cursor, title, atoms.size(), AtomDescription);
  insertAtomicCoordinates(cursor, atoms, AtomDescription);
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
  cursor.insertText(title + "\n");
  cursor.insertText(QString("%1 atom%2, %3 coordinates\n")
                        .arg(numAtoms)
                        .arg(numAtoms > 1 ? "s" : "")
                        .arg(coords));
  cursor.insertText("Label\tSymbol\tx\ty\tz\tOcc.\n");
  cursor.insertText(INFO_HORIZONTAL_RULE);
}

void InfoDocuments::insertAtomicCoordinates(QTextCursor cursor,
                                            const QVector<Atom> &atoms,
                                            AtomDescription AtomDescription) {
  for (const auto &atom : atoms) {
    cursor.insertText(atom.description(AtomDescription) + "\n");
  }
  cursor.insertText("\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Current Surface Info
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void InfoDocuments::insertCurrentSurfaceInfoIntoTextDocument(
    QTextDocument *document, Scene *scene, FingerprintBreakdown breakdown) {
  Q_ASSERT(scene);

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
}

void InfoDocuments::insertGeneralSurfaceInformation(Surface *surface,
                                                    QTextCursor cursor) {
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
}

void InfoDocuments::insertWavefunctionInformation(Surface *surface,
                                                  QTextCursor cursor) {
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
}

void InfoDocuments::insertSurfacePropertyInformation(Surface *surface,
                                                     QTextCursor cursor) {
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

void InfoDocuments::insertFragmentPatchInformation(Surface *surface,
                                                   QTextCursor cursor) {
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
}

void InfoDocuments::insertSupplementarySurfacePropertyInformation(
    Surface *surface, QTextCursor cursor) {
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
}

void InfoDocuments::insertVoidDomainInformation(Surface *surface,
                                                QTextCursor cursor) {
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
  ChemicalStructure *structure= scene->chemicalStructure();

  if (!structure) return;

  QTextCursor cursor = QTextCursor(document);
  qDebug() << "Made cursor";
  auto * interactions = structure->interactions();

  qDebug() << "have interactions";

  if (interactions->rowCount() > 0) {
    // These must be here for performance!
    cursor.beginEditBlock();
    insertInteractionEnergiesGroupedByPair(interactions, cursor);
    //    insertLatticeEnergy(crystal, cursor);
    // insertInteractionEnergiesGroupedByWavefunction(scene, cursor);
    insertEnergyModelScalingInfo(cursor);
    cursor.endEditBlock();
  } else {
    cursor.insertText("No interaction energies found.");
  }
}

void InfoDocuments::insertEnergyModelScalingInfo(QTextCursor cursor) {
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
    knownComponentsOrder << "coulomb" << "repulsion" << "exchange" << "dispersion";

    QList<QString> sortedComponents;

    // Add known components in the desired order
    for (const QString& component : knownComponentsOrder) {
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

void InfoDocuments::insertInteractionEnergiesGroupedByPair(PairInteractionResults *results,
                                                           QTextCursor cursor) {
    qDebug() << "Cursor" << &cursor;
    const int eprec = settings::readSetting(settings::keys::ENERGY_TABLE_PRECISION).toInt();
    const int ewidth = 6;

    // Insert header
    cursor.insertHtml("<h1>Interaction Energies</h1>");
    insertEnergyScalingPreamble(cursor);

    // Get unique components from the results
    QSet<QString> uniqueComponents;
    for (const auto &result : results->pairInteractionResults()) {
        for (const auto &component : result->components()) {
            uniqueComponents.insert(component.first);
        }
    }

    QList<QString> sortedComponents = getOrderedComponents(uniqueComponents);

    // Define table header
    QStringList tableHeader{"Interaction Model"};
    tableHeader.append(sortedComponents);
    int numHeaderLines = 1;
    int numLines = numHeaderLines + results->pairInteractionResults().size();

    // Create table
    QTextTable *table = createTable(cursor, numLines, tableHeader.size());

    // Insert Table Header
    insertTableHeader(table, cursor, tableHeader);

    int row = 1;

    for (const auto &result : results->pairInteractionResults()) {
        QString interactionModel = result->interactionModel();

        // Insert interaction model into the first cell
        QTextTableCell interactionModelCell = table->cellAt(row, 0);
        QTextCursor interactionModelCursor = interactionModelCell.firstCursorPosition();
        interactionModelCursor.insertText(interactionModel);

        // Insert component values into the corresponding cells
        int column = 1;
        for (const QString &component : sortedComponents) {
            QTextTableCell componentCell = table->cellAt(row, column);
            QTextCursor componentCursor = componentCell.firstCursorPosition();

            bool found = false;
            for (const auto &pair : result->components()) {
                if (pair.first == component) {
		    insertRightAlignedCellValue(table, cursor, row, column,
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
    }

    cursor.movePosition(QTextCursor::End);
    cursor.insertText("\n\n");
}

void InfoDocuments::insertInteractionEnergiesGroupedByWavefunction(
    Scene *scene, QTextCursor cursor) {
  DeprecatedCrystal *crystal = scene->crystal();
  if (!crystal)
    return;
  if (crystal->sameTheoryDifferentEnergies().size() < 2) {
    return; // don't continue if there isn't more than one level of theory
  }

  // Insert header
  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText(
      "Interaction Energies Grouped by Electron Density (kJ/mol)\n");
  cursor.insertText(
      "R is the distance between molecular centers of mass (Å).\n\n");
  insertEnergyScalingPreamble(cursor);
  cursor.insertText(INFO_HORIZONTAL_RULE);
  cursor.insertText("\n");

  QVector<QColor> energyColors = crystal->interactionEnergyColors();
  QVector<SymopId> energySymops = crystal->interactionEnergySymops();
  QVector<double> energyDistances = crystal->interactionEnergyDistances();
  QMap<int, int> fragmentCounts = crystal->interactionEnergyFragmentCount();

  QVector<EnergyType> benchmarkedEnergyComponents{
      EnergyType::CoulombEnergy, EnergyType::PolarizationEnergy,
      EnergyType::DispersionEnergy, EnergyType::RepulsionEnergy,
      EnergyType::TotalEnergy};
  QVector<EnergyType> unbenchmarkedEnergyComponents =
      benchmarkedEnergyComponents;
  unbenchmarkedEnergyComponents.removeAll(EnergyType::TotalEnergy);

  // Insert tables
  const auto &energies = crystal->interactionEnergies();
  for (const auto &energyIndices : crystal->sameTheoryDifferentEnergies()) {
    InteractionEnergy energy = energies[energyIndices[0]];
    cursor.insertText(QString("[%1]\n").arg(Wavefunction::levelOfTheoryString(
        energy.second.theory, energy.second.basisset)));

    QVector<EnergyType> energyComponents = crystal->energyIsBenchmarked(energy)
                                               ? benchmarkedEnergyComponents
                                               : unbenchmarkedEnergyComponents;

    // Define table header
    QStringList tableHeader{"", "N", "Symop", "R"};
    foreach (EnergyType energyComponent, energyComponents) {
      tableHeader << energyNames[energyComponent];
    }
    int numHeaderLines = 1;

    // Create table
    int numRows = numHeaderLines + energyIndices.size();
    QTextTable *table = createTable(cursor, numRows, tableHeader.size());

    // Insert table header
    insertTableHeader(table, cursor, tableHeader);

    // Insert rows of data
    int row = 1;
    for (int i = 0; i < energyIndices.size(); ++i) {
      int energyIndex = energyIndices[i];
      InteractionEnergy energy = energies[energyIndex];
      QColor energyColor = energyColors[energyIndex];
      QString symopString =
          crystal->spaceGroup().symopAsString(energySymops[energyIndex]);
      double distance = energyDistances[energyIndex];
      int n = fragmentCounts[energyIndex];
      insertEnergyAtTableRow(row, table, cursor, energy, energyComponents,
                             energyColor, symopString, n, distance, true);
      row++;
    }
    cursor.movePosition(QTextCursor::End);
    cursor.insertText("\n\n");
  }
}

void InfoDocuments::insertEnergyAtTableRow(
    int row, QTextTable *table, QTextCursor cursor, InteractionEnergy energy,
    QVector<EnergyType> energyComponents, QColor energyColor,
    QString symopString, int n, double distance, bool skipWavefunctionColumn) {
  const int ENERGY_PRECISION =
      settings::readSetting(settings::keys::ENERGY_TABLE_PRECISION).toInt();
  const int ENERGY_WIDTH = 6;

  int column = 0;

  insertColorBlock(table, cursor, row, column++, energyColor);
  insertRightAlignedCellValue(table, cursor, row, column++,
                              QString("%1").arg(n, 3));

  cursor = table->cellAt(row, column++).firstCursorPosition();
  if (symopString.isEmpty()) {
    cursor.insertText("     ");
  } else {
    cursor.insertText(symopString);
  }

  if (distance > 0.0) {
    insertRightAlignedCellValue(table, cursor, row, column,
                                QString("%1").arg(distance, 6, 'f', 2));
  }
  column++;

  if (!skipWavefunctionColumn) {
    cursor = table->cellAt(row, column++).firstCursorPosition();
    cursor.insertText(Wavefunction::levelOfTheoryString(
        energy.second.theory, energy.second.basisset));
  }

  foreach (EnergyType energyComponent, energyComponents) {
    double energyValue = energy.first[energyComponent];
    QString energyString =
        QString("%1").arg(energyValue, ENERGY_WIDTH, 'f', ENERGY_PRECISION);

    insertRightAlignedCellValue(table, cursor, row, column, energyString);
    column++;
  }
}

void InfoDocuments::insertLatticeEnergy(Scene *scene, QTextCursor cursor) {
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
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Support Routines
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QTextTable *InfoDocuments::createTable(QTextCursor cursor, int numRows,
                                       int numColumns) {
  QTextTable *table = cursor.insertTable(numRows, numColumns);
  QTextTableFormat tableFormat = table->format();
  tableFormat.setCellPadding(5.0);
  tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
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
