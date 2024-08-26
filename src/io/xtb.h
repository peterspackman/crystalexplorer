#pragma once
#include "xtb_parameters.h"

QString xtbCoordString(const xtb::Parameters &params);
xtb::Result loadXtbResult(const xtb::Parameters &params, const QString &jsonFilename, const QString &moldenFilename);
