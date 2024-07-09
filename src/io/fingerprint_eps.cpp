// eps_writer.cpp
#include "fingerprint_eps.h"
#include <QColor>
#include <QRegularExpression>
#include <QStack>
#include <cmath>
#include <string>

inline QTextStream& operator<<(QTextStream& stream, const std::string& str) {
    return stream << QString::fromStdString(str);
}

FingerprintEpsWriter::FingerprintEpsWriter(int numberOfBins, double plotMin,
                                           double plotMax, double binSize,
                                           int numberOfGridlines,
                                           double gridSize)
    : m_numberOfBins(numberOfBins), m_plotMin(plotMin), m_plotMax(plotMax),
      m_binSize(binSize), m_numberOfGridlines(numberOfGridlines),
      m_gridSize(gridSize) {}

void FingerprintEpsWriter::writeEpsFile(QTextStream &ts, const QString &title,
                                        const Eigen::MatrixXd &binnedAreas,
                                        const QColor &maskedBinColor) {
  writeHeader(ts, title);
  writeTitle(ts, title);
  writeGridBoundary(ts);
  writeAxisLabels(ts);
  writeGridlinesAndScaleLabels(ts);
  writeBins(ts, binnedAreas, maskedBinColor);
  writeFooter(ts);
}

void FingerprintEpsWriter::writeHeader(QTextStream &ts, const QString &title) {
  QString shortTitle = title;
  shortTitle.remove(QRegularExpression("[_^{}]"));

  int x0 = int((EPS_OFFSETX - EPS_MARGIN_LEFT) * EPS_DPCM);
  int y0 = int((EPS_OFFSETY - EPS_MARGIN_BOTTOM) * EPS_DPCM);
  int x1 = int((EPS_OFFSETX + EPS_SIZE + EPS_MARGIN_RIGHT) * EPS_DPCM);
  int y1 = int((EPS_OFFSETY + EPS_SIZE + EPS_MARGIN_TOP) * EPS_DPCM);

  ts << fmt::format("%!PS-Adobe-3.0 EPSF-3.0\n"
                    "%%Creator: CrystalExplorer\n"
                    "%%Title: {}\n"
                    "%%BoundingBox: {} {} {} {}\n"
                    "%%LanguageLevel: 2\n"
                    "%%Pages: 1\n"
                    "%%EndComments\n"
                    "%%EndProlog\n"
                    "%%Page: 1 1\n"
                    "gsave\n\n",
                    shortTitle.toStdString(), x0, y0, x1, y1);

  ts << fmt::format(
      "% Use the ISOLatin1 encoding to get the Angstrom symbol\n"
      "/LucidaSansLatin-Italic\n"
      " << /LucidaSans-Italic findfont {{}} forall >>\n"
      " begin\n"
      "  /Encoding ISOLatin1Encoding 256 array copy def currentdict\n"
      " end\n"
      "definefont pop\n"
      "/LucidaSansLatin\n"
      " << /LucidaSans findfont {{}} forall >>\n"
      " begin\n"
      "  /Encoding ISOLatin1Encoding 256 array copy def currentdict\n"
      " end\n"
      "definefont pop\n\n");

  ts << fmt::format("% Macros\n"
                    "/a {{stroke}} bind def\n"
                    "/b {{sethsbcolor}} bind def\n"
                    "/c {{{:.4f} {:.4f} rectfill newpath}} bind def\n"
                    "/d {{closepath stroke}} bind def\n"
                    "/e {{newpath moveto}} bind def\n"
                    "/f {{lineto}} bind def\n"
                    "/g {{rlineto stroke}} bind def\n"
                    "{} {} scale\n",
                    EPS_SIZE / m_numberOfBins, EPS_SIZE / m_numberOfBins,
                    EPS_DPCM, EPS_DPCM);
}

void FingerprintEpsWriter::writeTitle(QTextStream &ts, const QString &title) {
  if (title.isEmpty()) {
    return;
  }

  QStack<int> stack;
  double fontSize = EPS_TITLE_FONT_SIZE;

  ts << fmt::format("% Fingerprint title\n"
                    "0 0 0 setrgbcolor\n"
                    "/LucidaSansLatin-Italic findfont\n"
                    "{} scalefont setfont\n"
                    "{} {} e\n",
                    fontSize, EPS_OFFSETX + 0.3, EPS_OFFSETY + 0.3);

  ts << "(";
  for (int i = 0; i < title.length(); ++i) {
    QChar c = title[i];
    if (c == '_') {
      stack.push(0); // SUBSCRIPT
      fontSize = EPS_TITLE_FONT_SIZE * std::pow(0.6, stack.size());
      ts << fmt::format(") show\n"
                        "/LucidaSansLatin-Italic findfont\n"
                        "{} scalefont setfont\n"
                        "0 -{} rmoveto\n"
                        "(",
                        fontSize, fontSize * 0.3);
    } else if (c == '^') {
      stack.push(1); // SUPERSCRIPT
      fontSize = EPS_TITLE_FONT_SIZE * std::pow(0.6, stack.size());
      ts << fmt::format(") show\n"
                        "/LucidaSansLatin-Italic findfont\n"
                        "{} scalefont setfont\n"
                        "0 {} rmoveto\n"
                        "(",
                        fontSize, fontSize * 0.7);
    } else if (c == '}') {
      ts << ") show\n";
      if (!stack.isEmpty()) {
        int state = stack.pop();
        if (state == 0) { // SUBSCRIPT
          ts << fmt::format("0 {} rmoveto\n", fontSize * 0.3);
        } else { // SUPERSCRIPT
          ts << fmt::format("0 -{} rmoveto\n", fontSize * 0.7);
        }
      }
      fontSize = EPS_TITLE_FONT_SIZE * std::pow(0.6, stack.size());
      if (stack.size() == 0) {
        ts << fmt::format("/LucidaSansLatin-Italic findfont\n"
                          "{} scalefont setfont\n",
                          fontSize);
      }
      ts << "(";
    } else if (c != '{') {
      ts << c;
    }
  }
  ts << ") show\n";
}

void FingerprintEpsWriter::writeGridBoundary(QTextStream &ts) {
  double lowx = EPS_OFFSETX;
  double lowy = EPS_OFFSETY;
  double highx = lowx + EPS_SIZE;
  double highy = lowy + EPS_SIZE;

  ts << fmt::format("% Grid boundary\n"
                    "0 0 0 setrgbcolor\n"
                    "{} setlinewidth\n"
                    "{} {} e\n"
                    "{} {} f\n"
                    "{} {} f\n"
                    "{} {} f\n"
                    "{} {} f d\n",
                    EPS_GRIDBOUNDARY_LINEWIDTH, lowx, lowy, highx, lowy, highx,
                    highy, lowx, highy, lowx, lowy);
}

void FingerprintEpsWriter::writeAxisLabels(QTextStream &ts) {
  double scale_cm = EPS_SIZE / (m_plotMax - m_plotMin);

  ts << fmt::format("% Angstrom symbol\n"
                    "0 0 0 setrgbcolor\n"
                    "/LucidaSansLatin findfont\n"
                    "{} scalefont setfont\n"
                    "{} {} e ((\\305)) show\n",
                    EPS_ANGSTROM_FONT_SIZE, EPS_OFFSETX - 0.5,
                    EPS_OFFSETY - 0.5);

  double x = EPS_OFFSETX + EPS_SIZE - 0.15 * scale_cm;
  ts << fmt::format("% x-axis label\n"
                    "0 0 0 setrgbcolor\n"
                    "/LucidaSansLatin-Italic findfont\n"
                    "{} scalefont setfont\n"
                    "{} {} e (d) show\n"
                    "/LucidaSansLatin-Italic findfont\n"
                    "{} scalefont setfont\n"
                    "0 -0.08 rmoveto (i) show\n",
                    EPS_AXIS_LABEL_FONT_SIZE, x, EPS_OFFSETY + 0.25,
                    EPS_AXIS_LABEL_FONT_SIZE * 0.6);

  double y = EPS_OFFSETY + EPS_SIZE - 0.15 * scale_cm;
  ts << fmt::format("% y-axis label\n"
                    "0 0 0 setrgbcolor\n"
                    "/LucidaSansLatin-Italic findfont\n"
                    "{} scalefont setfont\n"
                    "{} {} e (d) show\n"
                    "/LucidaSansLatin-Italic findfont\n"
                    "{} scalefont setfont\n"
                    "0 -0.08 rmoveto (e) show\n",
                    EPS_AXIS_LABEL_FONT_SIZE, EPS_OFFSETX + 0.15, y,
                    EPS_AXIS_LABEL_FONT_SIZE * 0.6);
}

void FingerprintEpsWriter::writeGridlinesAndScaleLabels(QTextStream &ts) {
  double scale_cm = EPS_SIZE / (m_plotMax - m_plotMin);

  ts << fmt::format("% Scale label font\n"
                    "/LucidaSans findfont\n"
                    "{} scalefont setfont\n"
                    "0 0 0 setrgbcolor\n"
                    "% x gridlines and scale labels\n"
                    "{} setlinewidth\n",
                    EPS_AXIS_SCALE_FONT_SIZE, EPS_GRID_LINEWIDTH);

  for (int i = 1; i < m_numberOfGridlines; ++i) {
    double x = i * m_gridSize * scale_cm + EPS_OFFSETX;
    double y = i * m_gridSize * scale_cm + EPS_OFFSETY;

    ts << fmt::format("{} {} e 0 {} g\n"
                      "{} {} e {} 0 g\n",
                      x, EPS_OFFSETY, EPS_SIZE, EPS_OFFSETX, y, EPS_SIZE);

    QString scaleLabel = QString::number(m_plotMin + i * m_gridSize, 'f', 1);
    ts << fmt::format("{} {} e ({}) show\n"
                      "{} {} e ({}) show\n",
                      x - EPS_AXIS_SCALE_FONT_SIZE * scaleLabel.length() * 0.25,
                      EPS_OFFSETY - 0.5, scaleLabel.toStdString(),
                      EPS_OFFSETX -
                          EPS_AXIS_SCALE_FONT_SIZE * scaleLabel.length() + 0.25,
                      y - 0.10, scaleLabel.toStdString());
  }
}

void FingerprintEpsWriter::writeBins(QTextStream &ts,
                                     const Eigen::MatrixXd &binnedAreas,
                                     const QColor &maskedBinColor) {
  double binScale = EPS_SIZE / ((m_plotMax - m_plotMin) / m_binSize);

  const double stdAreaForSaturatedColor = 0.001;
  const double enhancementFactor = 1.0;
  double maxValue =
      (stdAreaForSaturatedColor / enhancementFactor) * binnedAreas.sum();

  auto func = ColorMapFunc(m_colorScheme);
  func.lower = 0.0;
  func.upper = maxValue;
  func.reverse = true;

  for (int i = 0; i < binnedAreas.rows(); ++i) {
    for (int j = 0; j < binnedAreas.cols(); ++j) {
      double value = binnedAreas(i, j);
      if (value != 0.0) {
        QColor color = func(value);
        double x = (m_xOffset + i) * binScale + EPS_OFFSETX;
        double y = (m_yOffset + j) * binScale + EPS_OFFSETY;
        ts << fmt::format("{:.4f} {:.4f} {:.4f} {:.4f} {:.4f} b c\n", x, y, color.hueF(),
                          color.saturationF(), color.valueF());
      }
    }
  }
}

void FingerprintEpsWriter::writeFooter(QTextStream &ts) {
  ts << "grestore\n%%EOF\n";
}
