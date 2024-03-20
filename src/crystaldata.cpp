#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QVector>
#include <QtDebug>

#include "crystaldata.h"
#include "settings.h"

namespace crystaldata {

QVector<Scene *> loadCrystalsFromTontoOutput(const QString& filename, const QString &cif) {
    QVector<Scene *> crystalList;
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
	QTextStream ts(&file);
	crystalList = CrystalData::getData(ts, cif);
	if (settings::readSetting(settings::keys::DELETE_WORKING_FILES).toBool()) {
	  file.remove();
	}
      }

    return crystalList;
}

}

QVector<Scene *> CrystalData::getData(const JobParameters &jobParams) {

  QVector<Scene *> crystalList;
  QFile file(jobParams.outputFilename);
  if (file.open(QIODevice::ReadOnly)) {

    QTextStream ts(&file);

    crystalList = getData(ts, jobParams.inputFilename);
    if (settings::readSetting(settings::keys::DELETE_WORKING_FILES).toBool()) {
      file.remove();
    }
  }

  return crystalList;
}

QString nameForCrystal(DeprecatedCrystal *crystal) {
  QFileInfo fi(crystal->cifFilename());
  QString cifBasename = fi.baseName();
  QString crystalName = crystal->crystalName();

  QString name;
  if (cifBasename.toLower() == crystalName.toLower()) {
    name = crystalName;
  } else {
    name = cifBasename + " " + crystalName;
  }
  return name;
}

QVector<Scene *> CrystalData::getData(QTextStream &ts, QString cif) {
  QVector<Scene *> sceneList;
  while (!ts.atEnd()) {
    QStringList tokens = ts.readLine().split(QRegularExpression("\\s+"));
    if (tokens.size() < 2) {
      continue;
    }

    QString blockDelimiter = tokens[0];
    if (blockDelimiter != "begin") {
      continue;
    }

    QString blockType = tokens[1];

    if (blockType.contains("CIF", Qt::CaseInsensitive)) {
      readCIFBlock(ts);
    }
    if (blockType.contains("crystal", Qt::CaseInsensitive)) {
      QString crystalName = tokens[2];
      Scene *scene = processCrystalBlock(ts);

      if (scene != nullptr) {
        scene->crystal()->setCifFilename(cif);
        scene->crystal()->setCrystalName(crystalName);
        scene->crystal()->postReadingInit();
        scene->setTitle(nameForCrystal(scene->crystal()));
        sceneList.append(scene);
      }
    }
  }
  return sceneList;
}

void CrystalData::readCIFBlock(QTextStream &ts) {
  while (!ts.atEnd()) {
    QStringList tokens = ts.readLine().split(QRegularExpression("\\s+"));
    if (tokens.size() < 2) {
      continue;
    }
    QString blockDelimiter = tokens[0];
    QString blockType = tokens[1];
    if (blockDelimiter == "end" && blockType == "CIF") {
      break;
    }
  }
}

/*!
 A non-null return value indicates that reading the crystal block was successful
 */
Scene *CrystalData::processCrystalBlock(QTextStream &ts) {
  bool successfullyRead = true;

  Scene *crystal = new Scene();

  QString crystalCellLabel, atomsLabel;

  // Read text stream lines and process ...
  while (!ts.atEnd()) {

    // Find all two-token lines ...
    QStringList tokens =
        ts.readLine().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (tokens.size() < 2) {
      continue;
    }
    QString blockDelimiter = tokens[0];
    QString blockType = tokens[1];

    // Exit crystal block if end crystal found ...
    if (blockDelimiter == "end" && blockType == "crystal") {
      break;
    }

    // Process crystal data blocks
    if (blockDelimiter == "begin") {

      if (blockType == "crystalcell") {
        successfullyRead =
            processCrystalCellBlock(ts, crystal) && successfullyRead;
      }

      if (blockType == "seitz_matrices") {
        successfullyRead =
            processSeitzMatricesBlock(ts, crystal) && successfullyRead;
      }

      if (blockType == "inverse_symops") {
        successfullyRead =
            processInverseSymopsBlock(ts, crystal) && successfullyRead;
      }

      if (blockType == "symop_products") {
        successfullyRead =
            processSymopProductsBlock(ts, crystal) && successfullyRead;
      }

      if (blockType == "symops_for_unit_cell_atoms") {
        successfullyRead =
            processSymopsForUnitCellAtomsBlock(ts, crystal) && successfullyRead;
      }

      if (blockType == "unit_cell") {
        successfullyRead =
            processUnitCellBlock(ts, crystal) && successfullyRead;
      }

      if (blockType == "asymmetric_unit_atom_indices") {
        successfullyRead =
            processAsymmetricAtomsBlock(ts, crystal) && successfullyRead;
      }

      if (blockType == "adp") {
        successfullyRead = processAdpBlock(ts, crystal) && successfullyRead;
      }

      // Error detected, exit ...
      if (!successfullyRead) {
        // << "Problem reading " << blockType << " block" << endl;
        break;
      }
    }
  } // end processing crystal data blocks

  if (successfullyRead) {
    return crystal;
  } else {
    delete crystal;
    return nullptr;
  }
}

bool CrystalData::processCrystalCellBlock(QTextStream &ts, Scene *crystal) {
  float a, b, c, alpha, beta, gamma;
  QString spaceGroup, formula;

  // initialize cell parameters
  a = b = c = 0.0;
  alpha = beta = gamma = 0.0;

  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens =
        line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() < 2) {
      continue;
    }

    QString blockDelimiter = tokens[0];
    QString blockType = tokens[1];

    if (blockDelimiter == "end" && blockType == "crystalcell") {
      break;
    }

    QString var = tokens[0];

    // The values for formula and spacegroup are strings delimited by quotes
    // e.g. formula = "C5 H19 O1"
    QStringList quotedTokens =
        line.split(QRegularExpression("\""), Qt::SkipEmptyParts);
    if (var == "formula") {
      formula = quotedTokens[1];
    }
    if (var == "spacegroup") {
      spaceGroup = line.contains("?") ? "Unknown Spacegroup" : quotedTokens[1];
    }

    // Data read in is assumed to be in Angstroms and degrees.
    if (var == "a") {
      a = tokens[2].toFloat();
    }
    if (var == "b") {
      b = tokens[2].toFloat();
    }
    if (var == "c") {
      c = tokens[2].toFloat();
    }
    if (var == "alpha") {
      alpha = tokens[2].toFloat();
    }
    if (var == "beta") {
      beta = tokens[2].toFloat();
    }
    if (var == "gamma") {
      gamma = tokens[2].toFloat();
    }
  }
  crystal->crystal()->setCrystalCell(formula, spaceGroup, a, b, c, alpha, beta,
                                     gamma);

  return formula != QString() && spaceGroup != QString() &&
         (a * b * c * alpha * beta * gamma > 0);
}

bool CrystalData::processSeitzMatricesBlock(QTextStream &ts, Scene *crystal) {
  int nSeitz = 0;

  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens =
        line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() < 2) {
      continue;
    }

    QString blockDelimiter = tokens[0];
    QString blockType = tokens[1];

    if (blockDelimiter == "end" && blockType == "seitz_matrices") {
      break;
    }

    Matrix4q m;
    int MAX_I = (int)m.rows();
    int MAX_J = (int)m.cols();
    Q_ASSERT(tokens.size() == MAX_I * MAX_J);

    for (int i = 0; i < MAX_I; ++i) {
      for (int j = 0; j < MAX_J; ++j) {
        m(i, j) = tokens[j + MAX_J * i].toFloat();
      }
    }

    crystal->crystal()->spaceGroup().addSeitzMatrix(m);
    nSeitz++;
  }
  return nSeitz > 0;
}

bool CrystalData::processInverseSymopsBlock(QTextStream &ts, Scene *crystal) {
  QVector<int> inverseSymops;

  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens =
        line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() == 0) {
      continue; // skip empty lines
    }

    if (tokens.size() == 2) {
      QString blockDelimiter = tokens[0];
      QString blockType = tokens[1];

      if (blockDelimiter == "end" && blockType == "inverse_symops") {
        break;
      }
    }

    Q_ASSERT(tokens.size() == 1);

    // Tonto (fortran) counts from 1 so if we subtract 1 we get symop indices
    // starting from 0
    int inverseSymop = tokens[0].toInt() - 1;
    inverseSymops.append(inverseSymop);
  }

  crystal->crystal()->spaceGroup().addInverseSymops(inverseSymops);

  return crystal->crystal()->spaceGroup().numberOfSymops() ==
         inverseSymops.size();
}

bool CrystalData::processSymopProductsBlock(QTextStream &ts, Scene *crystal) {
  QVector<QVector<int>> symopProducts;

  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens =
        line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() == 0) {
      continue; // skip empty lines
    }

    if (tokens.size() == 2) {
      QString blockDelimiter = tokens[0];
      QString blockType = tokens[1];

      if (blockDelimiter == "end" && blockType == "symop_products") {
        break;
      }
    }

    Q_ASSERT(tokens.size() ==
             crystal->crystal()->spaceGroup().numberOfSymops());

    QVector<int> rowOfTable;
    QStringListIterator iter(tokens);
    while (iter.hasNext()) {
      // Tonto (fortran) counts from 1 so if we subtract 1 we get symop indices
      // starting from 0
      rowOfTable << iter.next().toInt() - 1;
    }
    symopProducts.append(rowOfTable);
  }

  crystal->crystal()->spaceGroup().addSymopProducts(symopProducts);

  return symopProducts.size() ==
         crystal->crystal()->spaceGroup().numberOfSymops();
}

bool CrystalData::processSymopsForUnitCellAtomsBlock(QTextStream &ts,
                                                     Scene *crystal) {
  std::vector<int> symopsForUnitCellAtoms;

  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens =
        line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() == 0) {
      continue; // skip empty lines
    }

    if (tokens.size() == 2) {

      QString blockDelimiter = tokens[0];
      QString blockType = tokens[1];

      if (blockDelimiter == "end" &&
          blockType == "symops_for_unit_cell_atoms") {
        break;
      }
    }

    Q_ASSERT(tokens.size() == 1);

    // Tonto (fortran) counts from 1 so if we subtract 1 we get symop indices
    // starting from 0
    int symop = tokens[0].toInt() - 1;
    symopsForUnitCellAtoms.push_back(symop);
  }

  crystal->crystal()->setSymopsForUnitCellAtoms(symopsForUnitCellAtoms);

  return symopsForUnitCellAtoms.size() ==
         crystal->crystal()->unitCellAtoms().size();
}

bool CrystalData::processUnitCellBlock(QTextStream &ts, Scene *crystal) {
  QVector<Atom> atomList;

  // begin adding this set of atoms to the cartesian matrix
  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens =
        line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() < 2) {
      continue;
    }

    QString blockDelimiter = tokens[0];
    QString blockType = tokens[1];

    if (blockDelimiter == "end" && blockType == "unit_cell") {
      break;
    }

    Q_ASSERT(tokens.size() == 7);

    QString siteLabel = tokens[0];
    QString elementSymbol = tokens[1];

    // Convert deuteriums to hydrogens (But retain their original sitelabel)
    if (elementSymbol == "D") {
      elementSymbol = "H";
    }

    float x = tokens[2].toFloat();
    float y = tokens[3].toFloat();
    float z = tokens[4].toFloat();
    // hack to workaround any -ve disorder groups
    int disorderGroup = abs(tokens[5].toInt());

    float occupancy = tokens[6].toFloat();

    Atom atom =
        Atom(siteLabel, elementSymbol, x, y, z, disorderGroup, occupancy);
    atomList.append(atom);
  }

  if (atomList.size() > 0) {
    crystal->crystal()->setUnitCellAtoms(atomList);
    return true;
  } else {
    return false; // we didn't get any atoms
  }
}

bool CrystalData::processAsymmetricAtomsBlock(QTextStream &ts, Scene *crystal) {
  QMap<int, Shift> asymmUnit;

  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens =
        line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() < 2) {
      continue;
    }

    QString blockDelimiter = tokens[0];
    QString blockType = tokens[1];

    if (blockDelimiter == "end" &&
        blockType == "asymmetric_unit_atom_indices") {
      break;
    }

    Q_ASSERT(tokens.size() == 4);

    int atomIndex = tokens[0].toInt() - 1;
    Shift offset{tokens[1].toInt(), tokens[2].toInt(), tokens[3].toInt()};
    asymmUnit[atomIndex] = offset;
  }

  if (asymmUnit.keys().size() > 0) {
    crystal->crystal()->setAsymmetricUnitIndicesAndShifts(asymmUnit);
    return true;
  } else {
    return false; // fail!
  }
}

bool CrystalData::processAdpBlock(QTextStream &ts, Scene *crystal) {
  /* Assymetric Displacement Parameters for Thermal Ellipsoids */
  QVector<Atom> &atoms = crystal->crystal()->unitCellAtoms();
  int nAdps = 0;

  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens =
        line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() < 2) {
      continue;
    }

    QString blockDelimiter = tokens[0];
    QString blockType = tokens[1];

    if (blockDelimiter == "end" && blockType == "adp") {
      break;
    }

    Q_ASSERT(tokens.size() == 7);

    QString elementSymbol = tokens[0];
    QVector<float> adp;
    for (int i = 0; i < 6; ++i) {
      adp << tokens[i + 1].toFloat();
    }
    for (Atom &atom : atoms) {
      if (atom.symbol().toUpper() == elementSymbol.toUpper()) {
        atom.addAdp(adp);
        nAdps++;
        break;
      }
    }
  }
  return (nAdps > 0);
}
