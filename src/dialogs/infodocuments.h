#pragma once
#include <QTextCursor>

#include "fingerprintwindow.h" // For FingerprintBreakdown typedef
#include "scene.h"
#include "pair_energy_results.h"

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
