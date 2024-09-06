#include "frameworkoptions.h"
#include <array>

inline constexpr std::array<FrameworkOptions::Coloring, 4> availableColorings =
    {FrameworkOptions::Coloring::Component, FrameworkOptions::Coloring::Value,
     FrameworkOptions::Coloring::Interaction,
     FrameworkOptions::Coloring::Custom};

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

inline constexpr std::array<FrameworkOptions::ConnectionMode, 3>
    availableConnectionModes = {
        FrameworkOptions::ConnectionMode::Centroids,
        FrameworkOptions::ConnectionMode::CentersOfMass,
        FrameworkOptions::ConnectionMode::NearestAtoms,
};

QStringList availableFrameworkConnectionModeOptions() {
  QStringList result;
  for (const auto &c : availableConnectionModes) {
    result.append(frameworkConnectionModeToString(c));
  }
  return result;
}

QString frameworkConnectionModeToString(FrameworkOptions::ConnectionMode mode) {
  switch (mode) {
  case FrameworkOptions::ConnectionMode::Centroids:
    return "Centroids";
  case FrameworkOptions::ConnectionMode::CentersOfMass:
    return "Centers of Mass";
  case FrameworkOptions::ConnectionMode::NearestAtoms:
    return "Nearest Atoms";
  }
  return "Centroids";
}

FrameworkOptions::ConnectionMode
frameworkConnectionModeFromString(const QString &s) {
  for (const auto &c : availableConnectionModes) {
    if (s == frameworkConnectionModeToString(c)) {
      return c;
    }
  }
  return FrameworkOptions::ConnectionMode::Centroids;
}
