#pragma once
#include <QTextCursor>

#include "fingerprintwindow.h" // For FingerprintBreakdown typedef
#include "scene.h"
#include "pair_energy_results.h"

enum class AtomDescription {
  SiteLabel,
  UnitCellShift,
  Hybrid,
  Coordinates,
  CartesianInfo,
  FractionalInfo
};

/*

QString Atom::description(AtomDescription defaultAtomDescription) const {
  QString result;

  switch (defaultAtomDescription) {
  case AtomDescription::SiteLabel:
    result = label();
    break;
  case AtomDescription::UnitCellShift:
    result = QString("(%1,%2,%3)")
                 .arg(m_uc_shift.h)
                 .arg(m_uc_shift.k)
                 .arg(m_uc_shift.l);
    break;
  case AtomDescription::Hybrid:
    result = label() + QString("(%1,%2,%3) %4 *")
                           .arg(m_uc_shift.h)
                           .arg(m_uc_shift.k)
                           .arg(m_uc_shift.l)
                           .arg(m_uc_atom_idx);
    break;
    break;
  case AtomDescription::Coordinates:
    result = QString("(%1,%2,%3)").arg(fx()).arg(fy()).arg(fz());
    break;
  case AtomDescription::CartesianInfo:
    result = generalInfoDescription(x(), y(), z());
    break;
  case AtomDescription::FractionalInfo:
    result = generalInfoDescription(fx(), fy(), fz());
    break;
  }
  return result;
}

QString Atom::generalInfoDescription(double x, double y, double z) const {
  const int WIDTH = 5;
  const int PRECISION = 4;

  QString result;

  QString xString = QString("%1").arg(x, WIDTH, 'f', PRECISION);
  QString yString = QString("%1").arg(y, WIDTH, 'f', PRECISION);
  QString zString = QString("%1").arg(z, WIDTH, 'f', PRECISION);
  QString occString = QString("%1").arg(m_occupancy, 4, 'f', 3);
  result = QString("%1\t%2\t%3\t%4\t%5\t%6")
               .arg(label())
               .arg(symbol())
               .arg(xString)
               .arg(yString)
               .arg(zString)
               .arg(occString);

  return result;
}
*/

class InfoDocuments {
public:
  static void insertGeneralCrystalInfoIntoTextDocument(QTextDocument *,
                                                       Scene *);
  static void insertAtomicCoordinatesIntoTextDocument(QTextDocument *, Scene *);
  static void insertCurrentSurfaceInfoIntoTextDocument(QTextDocument *, Scene *,
                                                       FingerprintBreakdown);
  static void insertInteractionEnergiesIntoTextDocument(QTextDocument *,
                                                        Scene *);

private:
  // Atomic Coordinates Info
  static void insertAtomicCoordinatesSection(QTextCursor, QString,
                                             ChemicalStructure *,
                                             const std::vector<GenericAtomIndex> &,
                                             AtomDescription);
  static void insertAtomicCoordinatesWithAtomDescription(QTextCursor, Scene *,
                                                         AtomDescription);
  static void insertAtomicCoordinatesHeader(QTextCursor, QString, int,
                                            AtomDescription);
  static void insertAtomicCoordinates(QTextCursor,
                                      ChemicalStructure *,
                                      const std::vector<GenericAtomIndex> &,
                                      AtomDescription);

  // Current Surface Info
  static void insertGeneralSurfaceInformation(Mesh *, QTextCursor);
  static void insertWavefunctionInformation(Mesh *, QTextCursor);
  static void insertSurfacePropertyInformation(Mesh *, QTextCursor);
  static void insertFingerprintInformation(FingerprintBreakdown, QStringList,
                                           QTextCursor);
  static void insertFragmentPatchInformation(Mesh *, QTextCursor);
  static void insertSupplementarySurfacePropertyInformation(Mesh *,
                                                            QTextCursor);
  static void insertVoidDomainInformation(Mesh *, QTextCursor);
  static void insertDomainAtTableRow(int, QTextTable *, QTextCursor, QColor,
                                     double, double);

  // Interaction Energy Info
  static void insertEnergyScalingPreamble(QTextCursor);
  static void insertEnergyModelScalingInfo(QTextCursor);
  static void insertInteractionEnergiesGroupedByPair(PairInteractions *, QTextCursor);
  static void insertInteractionEnergiesGroupedByWavefunction(Scene *,
                                                             QTextCursor);
  static void insertLatticeEnergy(Scene *, QTextCursor);

  // Support Routines
  static QTextTable *createTable(QTextCursor, int, int);
  static void insertTableHeader(QTextTable *, QTextCursor, QStringList);
  static void insertColorBlock(QTextTable *, QTextCursor, int, int, QColor);
  static void insertRightAlignedCellValue(QTextTable *, QTextCursor, int, int,
                                          QString);
};
