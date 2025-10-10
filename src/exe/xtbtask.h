#pragma once
#include "externalprogram.h"
#include "xtb_parameters.h"

class XtbTask : public ExternalProgramTask {
  Q_OBJECT

public:
  inline static char inputSuffixDefault[7] = {".coord"};

  explicit XtbTask(QObject *parent = nullptr);
  void start() override;
  void preProcess() override;
  void postProcess() override;

  void setParameters(const xtb::Parameters &);
  const xtb::Parameters &getParameters() const;

  xtb::Result getResult() const;

  QString coordFilename() const;
  QString inputSuffix() const;
  QString jsonFilename() const;
  QString moldenFilename() const;
  QString propertiesFilename() const;

  QString stdoutContents() const;
  QString jsonContents() const;
  QString moldenContents() const;
  QString propertiesContents() const;

private:
  xtb::Parameters m_parameters;
};
