#pragma once
#include <QVector>

#include "jobparameters.h"
#include "scene.h"

namespace crystaldata {

QVector<Scene *> loadCrystalsFromTontoOutput(const QString& filename, const QString &cif);

}

class CrystalData {
public:
  static QVector<Scene *> getData(const JobParameters &);
  static QVector<Scene *> getData(QTextStream &, QString);

private:
  static void readCIFBlock(QTextStream &);
  static Scene *processCrystalBlock(QTextStream &);
  static bool processCrystalCellBlock(QTextStream &, Scene *);
  static bool processSeitzMatricesBlock(QTextStream &, Scene *);
  static bool processInverseSymopsBlock(QTextStream &, Scene *);
  static bool processSymopProductsBlock(QTextStream &, Scene *);
  static bool processSymopsForUnitCellAtomsBlock(QTextStream &, Scene *);
  static bool processAtomCoordsBlock(QTextStream &, Scene *);
  static bool processUnitCellBlock(QTextStream &, Scene *);
  static bool processAsymmetricAtomsBlock(QTextStream &, Scene *);
  static bool processAdpBlock(QTextStream &, Scene *);
  static void findTwoTokenLine(QTextStream &, QString &, QString &);
};
