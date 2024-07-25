#pragma once
#include <QString>

struct FrameworkOptions{
  enum class Display {
    None,
    Tubes,
    Lines
  };

  Display display{Display::None};
  QString model{"CE-1P"};
  QString component{"total"};
  double scale{0.001}; // A per kJ/mol
  double cutoff{0.0};
};
