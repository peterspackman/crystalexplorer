#pragma once
#include "deprecatedcrystal.h"
#include "fileeditor.h"
#include "jobparameters.h"

class QuantumChemistryInterface : public QObject {
  Q_OBJECT

public:
  QuantumChemistryInterface(QWidget *parent = 0);
  ~QuantumChemistryInterface();
  void setWorkingDirectory(const QString);
  void runJob(const JobParameters &, DeprecatedCrystal *);
  QString workingDirectory();

  QString outputFilePath();
  QString inputFilePath();
  // These are the functions you'll have to override
  virtual QString outputFilename() = 0;

public slots:
  void createProcessAndRun();
  void stopJob();
  void jobFinished(int, QProcess::ExitStatus);
  void jobState(QProcess::ProcessState);

signals:
  void processRunning();
  void updateStatusMessage(QString);
  void processFinished(bool);
  void wavefunctionDone();
  void processCancelled(QString);

protected:
  QString _jobName;
  QString _inputFilename;

private:
  void init();
  bool writeInputfile(DeprecatedCrystal *);
  void editInputFile();
  bool errorInOutput();
  void setJobName(DeprecatedCrystal *);
  QString fullInputFilename();

  // These are functions you might want to override
  virtual QString errorTitle();
  virtual QString failedWritingInputfileMsg();
  virtual QString execMissingMsg();
  virtual QString execRunningMsg();
  virtual QString execFailedMsg();
  virtual QString execCrashMsg();
  virtual QString processCancellationMsg();
  virtual bool redirectStdoutToOutputFile();

  // These are the functions you'll probably want to override
  virtual QProcessEnvironment getEnvironment();

  // These are the functions you have to override
  virtual void prejobSetup() = 0;
  virtual QString inputFilename() = 0;
  virtual QString normalTerminationHook() = 0;
  virtual void writeInputForWavefunctionCalculation(QTextStream &,
                                                    const JobParameters &,
                                                    DeprecatedCrystal *) = 0;
  virtual QString programName() = 0;
  virtual QString program() = 0;
  virtual QStringList commandline(const JobParameters &) = 0;
  virtual bool isExecutableInstalled() = 0;

  QWidget *_parent;
  QProcess *_process;

  JobParameters _jobParams;

  QString _workingDirectory;

  int _cycle;
  bool _processStoppedByUser;

  /// For editing advanced jobs
  FileEditor *_inputEditor;
};
