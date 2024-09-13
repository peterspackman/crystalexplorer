#pragma once
#include "xtb_parameters.h"

struct XtbOutputs {
  QString stdoutContents;
  QString jsonPath;
  QString propertiesPath;
  QString moldenPath;
};

QString xtbCoordString(const xtb::Parameters &params);
