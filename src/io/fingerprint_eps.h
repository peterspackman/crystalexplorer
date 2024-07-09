#pragma once

#include <Eigen/Dense>
#include <QColor>
#include <QString>
#include <QTextStream>
#include <fmt/format.h>
#include "colormap.h"

class FingerprintEpsWriter {
public:
  FingerprintEpsWriter(int numberOfBins, double plotMin, double plotMax,
                       double binSize, int numberOfGridlines, double gridSize);

  void writeEpsFile(QTextStream &ts, const QString &title,
                    const Eigen::MatrixXd &binnedAreas,
                    const QColor &maskedBinColor);

  inline void setXOffset(int x) { m_xOffset = x; }
  inline void setYOffset(int y) { m_yOffset = y; }

  inline void setColorScheme(ColorMapName cmap) { m_colorScheme = cmap; }

private:
  void writeHeader(QTextStream &ts, const QString &title);
  void writeTitle(QTextStream &ts, const QString &title);
  void writeGridBoundary(QTextStream &ts);
  void writeAxisLabels(QTextStream &ts);
  void writeGridlinesAndScaleLabels(QTextStream &ts);
  void writeBins(QTextStream &ts, const Eigen::MatrixXd &binnedAreas,
                 const QColor &maskedBinColor);
  void writeFooter(QTextStream &ts);

  int m_numberOfBins;
  double m_plotMin;
  double m_plotMax;
  double m_binSize;
  int m_numberOfGridlines;
  double m_gridSize;
  int m_xOffset{0};
  int m_yOffset{0};

  ColorMapName m_colorScheme{ColorMapName::CE_rgb};

  // Constants (you might want to make these configurable)
  static constexpr double EPS_DPCM = 28.36;
  static constexpr double EPS_SIZE = 10.0;
  static constexpr double EPS_OFFSETX = 2.0;
  static constexpr double EPS_OFFSETY = 2.0;
  static constexpr double EPS_MARGIN_LEFT = 1.5;
  static constexpr double EPS_MARGIN_RIGHT = 0.5;
  static constexpr double EPS_MARGIN_TOP = 0.5;
  static constexpr double EPS_MARGIN_BOTTOM = 1.5;
  static constexpr double EPS_TITLE_FONT_SIZE = 0.5;
  static constexpr double EPS_ANGSTROM_FONT_SIZE = 0.35;
  static constexpr double EPS_AXIS_LABEL_FONT_SIZE = 0.4;
  static constexpr double EPS_AXIS_SCALE_FONT_SIZE = 0.3;
  static constexpr double EPS_GRIDBOUNDARY_LINEWIDTH = 0.02;
  static constexpr double EPS_GRID_LINEWIDTH = 0.005;
};
