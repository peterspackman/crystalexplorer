#pragma once
#include "externalprogram.h"
#include "wavefunction_parameters.h"

class OrcaSCFTask : public ExternalProgramTask {
  Q_OBJECT

public:
  inline static char inputSuffixDefault[5] = {".inp"};

  explicit OrcaSCFTask(QObject *parent = nullptr);
  void start() override;

  void setParameters(const wfn::Parameters &);
  const wfn::Parameters &getParameters() const;

  QString inputSuffix() const;
  QString jsonFilename() const;
  QString moldenFilename() const;
  QString gbwFilename() const;
  QString propertiesFilename() const;

private:
  wfn::Parameters m_parameters;
};

class OrcaConvertTask : public ExternalProgramTask {

public:
  explicit OrcaConvertTask(QObject *parent = nullptr);
  void start() override;

  void setGbwFilename(const QString &);
  const QString &gbwFilename() const;

  void setFormat(bool json);

  QString moldenFilename() const;
  QString jsonFilename() const;

private:
  QString m_gbw{"input.gbw"};
  bool m_json{false};
};


class OrcaWavefunctionTask : public ExternalProgramTask {
  Q_OBJECT

public:
  explicit OrcaWavefunctionTask(QObject *parent = nullptr);
  void start() override;
  void setBackend(TaskBackend *backend) override;
  inline void setParameters(const wfn::Parameters &p) { m_scfTask->setParameters(p); }
  inline const wfn::Parameters &getParameters() const { return m_scfTask->getParameters(); }

  inline QString inputSuffix() const { return m_scfTask->inputSuffix(); }
  inline QString jsonFilename() const { return m_scfTask->jsonFilename(); }
  inline QString gbwFilename() const { return m_scfTask->gbwFilename(); }
  inline QString propertiesFilename() const { return m_scfTask->propertiesFilename(); }

  inline QString moldenFilename() const { return m_scfTask->moldenFilename(); }

private slots:
  void scfFinished();
  void conversionFinished();
  void updateStdout();

private:
  OrcaSCFTask *m_scfTask{nullptr};
  OrcaConvertTask *m_convertTask{nullptr};
};
