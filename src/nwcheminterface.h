#pragma once
#include "quantumchemistryinterface.h"

class NWChemInterface : public QuantumChemistryInterface {
public:
  NWChemInterface(QWidget *parent = nullptr);
  QString outputFilename();
  bool isExecutableInstalled() { return executableInstalled(); }
  void prejobSetup() {}
  static bool executableInstalled();
  static QString defaultMoldenFileExtension() { return ".molden"; }
  void writeMoldenBlock(QTextStream &);
  static QString calculationName(QString, QString);
  static QString moldenFileName(const JobParameters &, QString);
  void writeChargeAndScfBlock(QTextStream &, const JobParameters &);
  void writeDftBlock(QTextStream &, const JobParameters &);
  void writeBasisBlock(QTextStream &, const JobParameters &);

protected:
  static QString executable();

private:
  QString inputFilename();
  QString normalTerminationHook() { return " Total times  cpu"; }
  void writeInputForWavefunctionCalculation(QTextStream &,
                                            const JobParameters &,
                                            DeprecatedCrystal *);
  bool shouldUseUnrestricted(int);
  QString programName();
  QString program();
  QStringList commandline(const JobParameters &);
  QString taskName(const JobParameters &);
  QString exchangeKeyword(ExchangePotential);
  QString correlationKeyword(CorrelationPotential);
  QString basissetName(BasisSet);
  bool redirectStdoutToOutputFile() { return true; }
};
