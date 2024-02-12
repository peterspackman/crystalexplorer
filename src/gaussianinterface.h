#pragma once
#include "quantumchemistryinterface.h"

// ========= !!!! Keep same ordering {g98, g03, g09} for all of the following
// lists:
enum GaussianVersion { g98, g03, g09 };
const QStringList GAUSSIAN_BASENAME = {"g98", "g03", "g09"};
const QStringList GAUSSIAN_INPUT_EXTENSION = {"gjf", "gjf", "gjf"};
#if defined(Q_OS_WIN)
const QStringList GAUSSIAN_OUTPUT_EXTENSION = {"log", "log", "out"};
#else
const QStringList GAUSSIAN_OUTPUT_EXTENSION = {"log", "log", "log"};
#endif
// =========

// Running Gaussian on Windows
// ---------------------------
// In the previous version of CrystalExplorer (version 2.2) this didn't work and
// since
// we didn't have a means to test this it was difficult to fix.
// The Gaussian interface code has been rewritten and may now work on Windows
//
// There are potentially a number of issues with running Gaussian on Windows
// including:
// (i) Paths on windows can have spaces so perhaps the QStrings that hold them
// need to
// be quoted. The following options turns this on/off for the executable path,
// and input
// and output files names. Note: this has never been tested.
const bool WIN_USE_QUOTED_PATHS = true;
//
// (ii) On Mac/Linux gaussian is run using redirection i.e. g03 < inputfile.gjf
// It is unclear whether this works on Windows or whether the input should be
// specified as an argument to the QProcess i.e. g03 inputfile.gjf

class GaussianInterface : public QuantumChemistryInterface {
public:
  GaussianInterface(QWidget *parent = 0);
  QString outputFilename();

  // static methods
  static GaussianVersion getGaussianVersion();
  static bool executableInstalled();

  static QString defaultFChkFilename() { return "Test.FChk"; }
  static QString defaultFChkFileExtension() { return ".FChk"; }

  bool writeCounterpoiseInputFile(QString, DeprecatedCrystal *,
                                  const JobParameters &);

protected:
  static QString executable();

private:
  QString program();
  QProcessEnvironment getEnvironment();
  QString inputFilename();
  QString normalTerminationHook();
  QString keywords(const JobParameters &jobParams);
  void writeInputForWavefunctionCalculation(QTextStream &,
                                            const JobParameters &,
                                            DeprecatedCrystal *);
  void writeInputForCounterpoiseCalculation(QTextStream &, DeprecatedCrystal *,
                                            const JobParameters &,
                                            QString comments = "");
  bool shouldUseUnrestricted(int);
  QString programName();
  void prejobSetup();
  QStringList commandline(const JobParameters &);
  bool isExecutableInstalled();
  QString methodName(const JobParameters &, const bool);
  QString exchangeKeyword(ExchangePotential);
  QString correlationKeyword(CorrelationPotential);
  QString basissetName(BasisSet);
  bool redirectStdoutToOutputFile() { return false; }

  GaussianVersion _gaussianVersion;
};
