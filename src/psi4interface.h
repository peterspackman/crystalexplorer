#pragma once
#include "quantumchemistryinterface.h"

class Psi4Interface : public QuantumChemistryInterface {
public:
  Psi4Interface(QWidget *parent = nullptr);
  QString outputFilename();
  bool isExecutableInstalled() { return executableInstalled(); }
  void prejobSetup() {}
  static bool executableInstalled();
  static QString defaultFchkFileExtension() { return "fchk"; }
  static QString calculationName(QString, QString);
  static QString fchkFilename(const JobParameters &, QString);
  static QString methodName(const JobParameters &);
  static QString basisName(const JobParameters &);
  void writeWavefunctionCalculationBlock(QTextStream &, const JobParameters &,
                                         const QString &);

protected:
  static QString executable();

private:
  QString inputFilename();
  QString normalTerminationHook() { return " Psi4 exiting successfully"; }
  void writeInputForWavefunctionCalculation(QTextStream &,
                                            const JobParameters &,
                                            DeprecatedCrystal *);
  bool shouldUseUnrestricted(int);
  QString programName();
  QString program();
  QStringList commandline(const JobParameters &);
  QString taskName(const JobParameters &);
  static QString exchangeKeyword(ExchangePotential);
  static QString correlationKeyword(CorrelationPotential);
  QString basissetName(BasisSet);
  bool redirectStdoutToOutputFile() { return true; }
};
