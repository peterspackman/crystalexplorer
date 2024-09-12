#pragma once
#include <tuple>

struct AtomLabelOptions {
  bool show{false};
  bool fragmentLabel{false};
  inline bool operator==(const AtomLabelOptions &rhs) const {
    return std::tie(show, fragmentLabel) ==
           std::tie(rhs.show, rhs.fragmentLabel);
  }

  inline AtomLabelOptions cycle() const {
    auto result = *this;
    if (!show) {
      result.show = true;
      result.fragmentLabel = false;
    } else if (!fragmentLabel) {
      result.fragmentLabel = true;
    } else {
      result.show = false;
      result.fragmentLabel = false;
    }
    return result;
  }
};
