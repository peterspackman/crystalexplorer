#pragma once
#include <QString>
#include <QTextStream>

#include "deprecatedcrystal.h"
#include "fileeditor.h"
#include "jobparameters.h"

const QString TONTO_INPUT_FILENAME = "stdin";
const QString TONTO_OUTPUT_FILENAME = "stdout";

const bool TONTO_USE_ANGSTROMS = true;

const QString ERROR_HOOK = "error "; // trailing space important
const QString NO_ISOSURFACE_POINTS_HOOK = "No isosurface points found.";

enum TontoExitStatus {
  NormalExit,
  CrashExit,
  Stopped,
  NoOutput,
  ErrorInOutput,
  NoIsosurfacePoints
};

// Tonto parameters for surface generation (See
// TontoInterface::writeSurfaceGenerationInfo)
const double GLOBAL_VOXEL_PROXIMITY_FACTOR = 5.0;
const double GLOBAL_MINIMUM_SCAN_DIVISION = 1.0;
const double GLOBAL_BOUNDING_BOX_SCALE_FACTOR = 1.0;
const double GLOBAL_CUBE_SCALE_FACTOR = 1.0;
const double GLOBAL_DESIRED_SEPARATION = 0.2;
const QStringList GLOBAL_INTERPOLATION_METHODS = QStringList()
                                                 << "linear"
                                                 << "cubic_spline";
const QString GLOBAL_INTERPOLATION_METHOD = GLOBAL_INTERPOLATION_METHODS[0];
const QStringList GLOBAL_DOMAIN_MAPPINGS = QStringList() << "none"
                                                         << "sqrt"
                                                         << "sqrt(x(1-x))";
const QString GLOBAL_DOMAIN_MAPPING = GLOBAL_DOMAIN_MAPPINGS[1];
const int GLOBAL_TABLE_CUTOFF = -10;
const double GLOBAL_TABLE_SPACING = 0.1;
const int GLOBAL_HIRSHFELD_POWER_FACTOR = 3;

class TontoInterface : public QObject {
  Q_OBJECT

public:
  TontoInterface(QWidget *parent = 0);
  ~TontoInterface();
  void setWorkingDirectory(const QString);
  void runJob(const JobParameters &, DeprecatedCrystal *crystal = nullptr,
              const QVector<Wavefunction> & = {});
  QString getTontoInputFile();
  QString getTontoOutputFile();

  static QString calculationName(const JobParameters &, const QString &);
  static QString tontoWavefunctionFileSuffix();
  static QString tontoSBFName(const JobParameters &, const QString &);
  static QString fchkFilename(const JobParameters &, const QString &);
  static QString moldenFilename(const JobParameters &, const QString &);

signals:
  void tontoRunning();
  void tontoFinished(TontoExitStatus, JobType);
  void tontoCancelled(const QString &);
  void updateStatusMessage(const QString &);
  void updateProgressBar(int, int);

public slots:
  void createProcessAndRunTonto();
  void stopJob();
  void jobState(QProcess::ProcessState);
  void jobFinished(int, QProcess::ExitStatus);

private:
  void init();
  void editTontoInput();
  bool tontoInstalled();
  QString tontoExecutable();
  QString workingDirectory();
  QString basissetDirectory();
  QString basissetName(BasisSet);
  QString exchangePotentialKeyword(ExchangePotential);
  QString correlationPotentialKeyword(CorrelationPotential);
  QString jobDescription(JobType, int, int);

  bool foundStringInTontoOutput(const QString &,
                                bool atBeginningOfLine = false);
  bool errorInTontoOutput();
  bool noIsosurfacePoints();
  bool wantFingerprintProperties(IsosurfaceDetails::Type);
  bool wantShapeProperties(IsosurfaceDetails::Type);

  bool writeTontoInputfile(const JobParameters &,
                           DeprecatedCrystal *crystal = nullptr);
  /// These are tonto input writing routines
  void writeInputForCifProcessing(QTextStream &, const JobParameters &);
  void writeInputForSurfaceGeneration(QTextStream &, const JobParameters &,
                                      DeprecatedCrystal *);
  void writeInputForWavefunctionCalculation(QTextStream &,
                                            const JobParameters &,
                                            DeprecatedCrystal *);
  void writeInputForEnergyCalculation(QTextStream &, const JobParameters &,
                                      DeprecatedCrystal *);
  void writeInputForSurfaceWithProductProperty(QTextStream &,
                                               const JobParameters &,
                                               DeprecatedCrystal *);

  /// These write parts of the tonto input
  void writeHeader(QTextStream &, const QString);
  void writeBasisset(QTextStream &, const QString &slaterBasisName = QString());
  void writeCifData(QTextStream &, const QString &, bool,
                    const QString &dataBlockName = QString());
  void writeVerbosityInfo(QTextStream &);
  void writeProcessCifForCX(QTextStream &, const QString &);
  void writeProcessCifForSurface(QTextStream &ts);
  void writeChargeMultiplicity(QTextStream &, int, int);
  void writeNameReset(QTextStream &, const QString &, const QString &);
  void writeBasissetForWavefunction(QTextStream &, const QString &,
                                    const QString &);
  void writeGaussianWavefunction(QTextStream &, const QString &);
  void writeMoldenWavefunction(QTextStream &, const QString &);
  void writeClusterInfo(QTextStream &, const JobParameters &);
  void writeFragmentInfo(QTextStream &, const QVector<AtomId>,
                         DeprecatedCrystal *, bool isNewData = true);
  void writeFragmentGroups(QTextStream &, const QVector<int>,
                           const QVector<WavefunctionTransform>);
  void writeCreateClusterInfo(QTextStream &);
  void writeSurfaceGenerationInfo(QTextStream &, const JobParameters &);
  void writeSurfaceGenerationInterpolationSettings(QTextStream &);
  void writeSurfaceCreationInfo(QTextStream &, const JobParameters &);
  void writeSurfacePlottingInfo(QTextStream &, const JobParameters &);
  void writeCXInfo(QTextStream &, const QString &);
  QString shellKindForHartreeFock(int);
  QString shellKindForKohnSham(int);
  void writeSCFInfo(QTextStream &, const JobParameters &);
  void writeSCFCommands(QTextStream &);
  void writeWavefunctionMatrix(QTextStream &, const JobParameters &);
  void writeWavefunctionInfo(QTextStream &, const JobParameters &);
  QString getReadFchkCommand();
  QString getReadMoldenCommand();
  void writeTontoWavefunction(QTextStream &, const JobParameters &);
  void writeSerializeMolecule(QTextStream &, const JobParameters &,
                              const QString &);
  void writeSleazySCFCriteria(QTextStream &);
  void writeEnergyCalculationInfo(QTextStream &);
  void writeFooter(QTextStream &);

  QString getFchkFileForGroup(int);
  QString wavefunctionFilename(int index = 0);

  /// Parent widget
  QWidget *_parent;

  /// The background tonto job process
  QProcess *_process;

  /// Parameters for the tonto job
  JobParameters _jobParams;

  /// The directory where tonto runs
  QString _workingDirectory;
  QString _prevWorkingDirectory;
  QTemporaryDir *m_tempDir{nullptr};
  /// For editing advanced jobs
  FileEditor *_tontoInputEditor;

  bool _tontoStoppedByUser;

  QStringList _wavefunctionFilenames;
};
