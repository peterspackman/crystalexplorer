#include "frameworkoptions.h"
#include <array>

inline constexpr std::array<FrameworkOptions::Coloring, 4> availableColorings = {
  FrameworkOptions::Coloring::Component,
  FrameworkOptions::Coloring::Value,
  FrameworkOptions::Coloring::Interaction,
  FrameworkOptions::Coloring::Custom
};

QStringList availableFrameworkColoringOptions() {
  QStringList result;
  for (const auto &c : availableColorings) {
    result.append(frameworkColoringToString(c));
  }
  return result;
}

QString frameworkColoringToString(FrameworkOptions::Coloring coloring) {
  switch (coloring) {
  case FrameworkOptions::Coloring::Component:
    return "Component";
  case FrameworkOptions::Coloring::Value:
    return "Value";
  case FrameworkOptions::Coloring::Interaction:
    return "Interaction";
  case FrameworkOptions::Coloring::Custom:
    return "Custom";
  }
  return "Custom";
}

FrameworkOptions::Coloring frameworkColoringFromString(const QString &s) {
  for (const auto &c : availableColorings) {
    if (s == frameworkColoringToString(c)) {
      return c;
    }
  }
  return FrameworkOptions::Coloring::Component;
}
