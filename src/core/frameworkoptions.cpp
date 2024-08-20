#include "frameworkoptions.h"

using enum FrameworkOptions::Coloring;
inline constexpr std::array<FrameworkOptions::Coloring, 4> availableColorings =
    {Component, Value, Interaction, Custom};

QStringList availableFrameworkColoringOptions() {
  QStringList result;
  for (const auto &c : availableColorings) {
    result.append(frameworkColoringToString(c));
  }
  return result;
}

QString frameworkColoringToString(FrameworkOptions::Coloring coloring) {
  switch (coloring) {
  case Component:
    return "Component";
  case Value:
    return "Value";
  case Interaction:
    return "Interaction";
  case Custom:
    return "Custom";
  }
}

FrameworkOptions::Coloring frameworkColoringFromString(const QString &s) {
  for (const auto &c : availableColorings) {
    if (s == frameworkColoringToString(c)) {
      return c;
    }
  }
  return Component;
}
