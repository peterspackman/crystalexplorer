#pragma once
#include <QTextCursor>

#include "fingerprintwindow.h" // For FingerprintBreakdown typedef
#include "scene.h"

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
                                             const QVector<Atom> &,
                                             AtomDescription);
  static void insertAtomicCoordinatesWithAtomDescription(QTextCursor, Scene *,
                                                         AtomDescription);
  static void insertAtomicCoordinatesHeader(QTextCursor, QString, int,
                                            AtomDescription);
  static void insertAtomicCoordinates(QTextCursor, const QVector<Atom> &,
                                      AtomDescription);

  // Current Surface Info
  static void insertGeneralSurfaceInformation(Surface *, QTextCursor);
  static void insertWavefunctionInformation(Surface *, QTextCursor);
  static void insertSurfacePropertyInformation(Surface *, QTextCursor);
  static void insertFingerprintInformation(FingerprintBreakdown, QStringList,
                                           QTextCursor);
  static void insertFragmentPatchInformation(Surface *, QTextCursor);
  static void insertSupplementarySurfacePropertyInformation(Surface *,
                                                            QTextCursor);
  static void insertVoidDomainInformation(Surface *, QTextCursor);
  static void insertDomainAtTableRow(int, QTextTable *, QTextCursor, QColor,
                                     double, double);

  // Interaction Energy Info
  static void insertEnergyScalingPreamble(QTextCursor);
  static void insertEnergyModelScalingInfo(QTextCursor);
  static void insertInteractionEnergiesGroupedByPair(Scene *, QTextCursor);
  static void insertInteractionEnergiesGroupedByWavefunction(Scene *,
                                                             QTextCursor);
  static void insertEnergyAtTableRow(int, QTextTable *, QTextCursor,
                                     InteractionEnergy, QVector<EnergyType>,
                                     QColor, QString, int, double,
                                     bool skipWavefunctionColumn = false);
  static void insertLatticeEnergy(Scene *, QTextCursor);

  // Support Routines
  static QTextTable *createTable(QTextCursor, int, int);
  static void insertTableHeader(QTextTable *, QTextCursor, QStringList);
  static void insertColorBlock(QTextTable *, QTextCursor, int, int, QColor);
  static void insertRightAlignedCellValue(QTextTable *, QTextCursor, int, int,
                                          QString);
};
