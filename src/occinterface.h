#pragma once
#include "deprecatedcrystal.h"
#include "fileeditor.h"
#include "jobparameters.h"
#include <QObject>

class OccInterface : public QObject {
  Q_OBJECT
public:
  OccInterface(QWidget *parent = nullptr);
  QString outputFilename();
  static QString wavefunctionFilename(const JobParameters &jobParams,
                                      QString crystalName);
  bool isExecutableInstalled() { return executableInstalled(); }
  void prejobSetup();
  static bool executableInstalled();
  static QString defaultMoldenFileExtension() { return ".molden"; }
  static QString defaultFchkFileExtension() { return ".fchk"; }
  static QString calculationName(QString, QString);
  void runJob(const JobParameters &, DeprecatedCrystal *,
              const QVector<Wavefunction> & = {});
  QString outputFilePath();
  QString inputFilePath();
  void setWorkingDirectory(const QString &);
  const QString &workingDirectory() const;
  static QString jobDescription(JobType jobType, int maxStep = 0, int step = 0);
public slots:
  void runProcess();
  void stopJob();
  void jobFinished(int, QProcess::ExitStatus);
  void jobState(QProcess::ProcessState);

  QString errorTitle();

signals:
  void processRunning();
  void updateStatusMessage(QString);
  void processFinished(bool, JobType);
  void wavefunctionDone();
  void processCancelled(QString);
  void updateProgressBar(int, int);

protected:
  static QString executable();

private:
  bool writeInputfile(DeprecatedCrystal *);
  void editInputFile();
  bool errorInOutput();
  QString fullInputFilename();

  // These are functions you might want to override
  QString failedWritingInputfileMsg();
  QString execMissingMsg();
  QString execRunningMsg();
  QString execFailedMsg();
  QString execCrashMsg();
  QString processCancellationMsg();

  void setCurrentJobNameFromCrystal(DeprecatedCrystal *);
  // These are the functions you'll probably want to override
  QProcessEnvironment getEnvironment();

  // These are the functions you have to override
  QString inputFilename();
  QString normalTerminationHook() { return "A job well done"; }

  QString programName();
  QString program();
  QStringList commandline(const JobParameters &);

  void writeInputForWavefunctionCalculation(QTextStream &ts,
                                            const JobParameters &jobParams,
                                            DeprecatedCrystal *crystal);
  void writeInputForPairEnergyCalculation(QTextStream &ts,
                                          const JobParameters &jobParams);
  static QString methodName(const JobParameters &);
  static QString basissetName(BasisSet);
  bool redirectStdoutToOutputFile() { return true; }

  QWidget *m_parentWidget{nullptr};
  /// For editing advanced jobs
  FileEditor *m_inputEditor;
  QProcess *m_process{nullptr};
  JobParameters m_currentJobParams;
  QString m_currentJobName;
  QString m_currentInputFilename;
  QString m_workingDirectory;
  int m_cycle{0};
  bool m_processStoppedByUser{false};
  QVector<Wavefunction> m_currentWavefunctions;
  QVector<QString> m_wavefunctionFilenames;
};
