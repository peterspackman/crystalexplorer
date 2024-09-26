#pragma once
#include <QStringList>
#include <QColor>

struct FrameworkOptions{
  enum class Display {
    None,
    Tubes,
    Lines
  };

  enum class Coloring {
    Component,
    Value,
    Interaction,
    Custom,
  };

  enum class ConnectionMode {
    Centroids,
    CentersOfMass,
    NearestAtoms
  };

  enum class LabelDisplay {
    None,
    Value,
    Interaction,
    Fragments,
  };

  Display display{Display::None};
  Coloring coloring{Coloring::Component};
  ConnectionMode connectionMode{ConnectionMode::Centroids};
  LabelDisplay labels{LabelDisplay::None};
  QString model{"CE-1P"};
  QString component{"total"};
  double scale{0.001}; // A per kJ/mol
  double cutoff{0.0};
  bool allowInversion{true};
  QColor customColor{Qt::black};
  bool showOnlySelectedFragmentInteractions{false};
};

QString frameworkColoringToString(FrameworkOptions::Coloring);
FrameworkOptions::Coloring frameworkColoringFromString(const QString &s);
QStringList availableFrameworkColoringOptions();

QString frameworkConnectionModeToString(FrameworkOptions::ConnectionMode);
FrameworkOptions::ConnectionMode frameworkConnectionModeFromString(const QString &s);
QStringList availableFrameworkConnectionModeOptions();

QString frameworkLabelDisplayToString(FrameworkOptions::LabelDisplay);
FrameworkOptions::LabelDisplay frameworkLabelDisplayFromString(const QString &s);
QStringList availableFrameworkLabelDisplayOptions();


