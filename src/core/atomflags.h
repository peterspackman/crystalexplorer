#pragma once
#include <QFlags>

enum class AtomFlag : int {
  NoFlag = 0x0,
  Selected = 0x1,
  Contact = 0x2,
  Suppressed = 0x4,
  CustomColor = 0x8,
};

Q_DECLARE_FLAGS(AtomFlags, AtomFlag);
Q_DECLARE_OPERATORS_FOR_FLAGS(AtomFlags);
