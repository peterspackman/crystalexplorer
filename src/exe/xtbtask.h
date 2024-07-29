#include "externalprogram.h"
#include "xtb_parameters.h"

class XtbTask : public ExternalProgramTask {

public:
  inline static char inputSuffixDefault[7] = {".coord"};

  explicit XtbTask(QObject *parent = nullptr);
  virtual void start() override;

  void setParameters(const xtb::Parameters &);
  const xtb::Parameters &getParameters() const;

  QString inputSuffix() const;
  QString jsonFilename() const;

private:
  xtb::Parameters m_parameters;
};
