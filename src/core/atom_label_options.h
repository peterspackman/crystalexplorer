#pragma once
#include <tuple>

struct AtomLabelOptions {
  bool showAtoms{false};
  bool showFragment{false};
  inline bool operator==(const AtomLabelOptions &rhs) const {
    return std::tie(showAtoms, showFragment) ==
           std::tie(rhs.showAtoms, rhs.showFragment);
  }
};
