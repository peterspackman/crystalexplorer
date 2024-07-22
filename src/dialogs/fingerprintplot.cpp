#include "fingerprintplot.h"

#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPen>
#include <QRegularExpression>
#include <QStack>
#include <QStylePainter>
#include <QTime>
#include <QtDebug>

#include "elementdata.h"
#include "fingerprint_eps.h"
#include "settings.h"

FingerprintPlot::FingerprintPlot(QWidget *parent) : QWidget(parent) { init(); }

void FingerprintPlot::init() {
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  m_mesh = nullptr;
  setRange(FingerprintPlotRange::Standard);

  resetFilter();
}

void FingerprintPlot::resetFilter() {
  setFilter(FingerprintFilterMode::None, false, false, false, QString(),
            QString());
}

void FingerprintPlot::setFilter(FingerprintFilterMode filterMode,
                                bool includeReciprocalContacts,
                                bool filterInsideElement,
                                bool filterOutsideElement,
                                QString insideFilterElementSymbol,
                                QString outsideFilterElementSymbol) {
  m_filterMode = filterMode;
  m_includeReciprocalContacts = includeReciprocalContacts;
  m_insideFilterElementSymbol = insideFilterElementSymbol;
  m_outsideFilterElementSymbol = outsideFilterElementSymbol;
  m_filterInsideElement = -1;
  m_filterOutsideElement = -1;

  if (filterInsideElement) {
    auto *el = ElementData::elementFromSymbol(m_insideFilterElementSymbol);
    if (el)
      m_filterInsideElement = el->number();
  }

  if (filterOutsideElement) {
    auto *el = ElementData::elementFromSymbol(m_outsideFilterElementSymbol);
    if (el)
      m_filterOutsideElement = el->number();
  }
}

void FingerprintPlot::updateFilter(FingerprintFilterMode filterMode,
                                   bool includeReciprocalContacts,
                                   bool filterInsideElement,
                                   bool filterOutsideElement,
                                   QString insideFilterElementSymbol,
                                   QString outsideFilterElementSymbol) {
  setFilter(filterMode, includeReciprocalContacts, filterInsideElement,
            filterOutsideElement, insideFilterElementSymbol,
            outsideFilterElementSymbol);
  updateFingerprintPlot();
}

void FingerprintPlot::setRange(FingerprintPlotRange range) { 
  m_range = range; 
  m_settings = plotRangeSettings(m_range);

}

void FingerprintPlot::updatePlotRange(FingerprintPlotRange range) {
  setRange(range);
  updateFingerprintPlot();
}

void FingerprintPlot::setMesh(Mesh *mesh) {
  m_mesh = mesh;
  updateFingerprintPlot();
}

void FingerprintPlot::setPropertiesToPlot() {
  qDebug() << "vertex property di";
  m_x = m_mesh->vertexProperty("di").cast<double>();
  qDebug() << "vertex property de";
  m_y = m_mesh->vertexProperty("de").cast<double>();

  m_xmin = m_x.minCoeff();
  m_xmax = m_x.maxCoeff();
  m_ymin = m_y.minCoeff();
  m_ymax = m_y.maxCoeff();

  qDebug() << "face property di";
  m_xFace = m_mesh->averagedFaceProperty("di").cast<double>();
  qDebug() << "face property de";
  m_yFace = m_mesh->averagedFaceProperty("de").cast<double>();

  m_xFaceMin = m_xFace.minCoeff();
  m_xFaceMax = m_xFace.maxCoeff();
  m_yFaceMin = m_yFace.minCoeff();
  m_yFaceMax = m_yFace.maxCoeff();

  qDebug() << "Set axis labels";
  setAxisLabels();
}

void FingerprintPlot::setAxisLabels() {
  m_xAxisLabel = "di";
  m_yAxisLabel = "de";
}

void FingerprintPlot::updateFingerprintPlot() {
  if (m_mesh) {
    qDebug() << "Set properties to plot";
    setPropertiesToPlot();
    qDebug() << "Init binned areas";
    initBinnedAreas();
    qDebug() << "Init binned filter flags";
    initBinnedFilterFlags();
    qDebug() << "calculate binned areas";
    calculateBinnedAreas();
    qDebug() << "draw fingerprint";
    drawFingerprint();
  } else {
    drawEmptyFingerprint();
  }
  qDebug() << "Set fixed size";
  setFixedSize(plotSize());
  parentWidget()->adjustSize();
  update();
}

void FingerprintPlot::initBinnedAreas() {
  int numxBins = numUsedxBins();
  int numyBins = numUsedyBins();
  binnedAreas = Eigen::MatrixXd::Zero(numxBins, numyBins);
}

void FingerprintPlot::initBinnedFilterFlags() {
  int numxBins = numUsedxBins();
  int numyBins = numUsedyBins();
  binUsed = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>::Zero(numxBins,
                                                                      numyBins);
}

inline double gaussianKernel(double x, double y, double h) {
  double r2 = x * x + y * y;
  return std::exp(-r2 / (2 * h * h)) / (2 * M_PI * h * h);
}

double FingerprintPlot::calculateBinnedAreasKDE() {
  double nx = numUsedxBins();
  double ny = numUsedyBins();
  double xmax = usedxPlotMax();
  double xmin = usedxPlotMin();
  double ymax = usedyPlotMax();
  double ymin = usedyPlotMin();
  double dx = (xmax - xmin) / nx;
  double dy = (ymax - ymin) / ny;

  binnedAreas = Eigen::MatrixXd::Zero(nx, ny);
  binUsed = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>::Zero(nx, ny);

  const auto &vertexAreas = m_mesh->vertexAreas();
  double totalArea = vertexAreas.sum();

  // Calculate bandwidth (you may need to adjust this based on your data)
  double bandwidth = 0.01;

  // Apply KDE
  for (int i = 0; i < nx; ++i) {
    for (int j = 0; j < ny; ++j) {
      double x = xmin + (i + 0.5) * dx;
      double y = ymin + (j + 0.5) * dy;
      double density = 0.0;

      for (int v = 0; v < m_mesh->numberOfVertices(); ++v) {
        double kx = x - m_x(v);
        double ky = y - m_y(v);
        density += vertexAreas(v) * gaussianKernel(kx, ky, bandwidth);
      }

      binnedAreas(i, j) = density * dx * dy;
      binUsed(i, j) = density > 1e-3;
    }
  }

  // Normalize to preserve total area
  double scaleFactor = totalArea / binnedAreas.sum();
  binnedAreas *= scaleFactor;

  qDebug() << "Total surface area:" << totalArea;
  qDebug() << "Total binned area:" << binnedAreas.sum();
  qDebug() << "Mesh surface area:" << m_mesh->surfaceArea();

  return m_mesh->surfaceArea();
}

double FingerprintPlot::calculateBinnedAreasNoFilter() {
  // barycentric subsampling, samplesPerEdge = 1 reduces to just the points
  double nx = numUsedxBins();
  double ny = numUsedyBins();
  double xmax = usedxPlotMax();
  double xmin = usedxPlotMin();
  double normx = nx / (xmax - xmin);
  double ymin = usedyPlotMin();
  double ymax = usedyPlotMax();
  double normy = ny / (ymax - ymin);

  binnedAreas = Eigen::MatrixXd::Zero(nx, ny);
  binUsed = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>::Zero(nx, ny);

  double totalArea = 0.0;

  for (int faceIdx = 0; faceIdx < m_mesh->numberOfFaces(); ++faceIdx) {
    Eigen::Vector3i faceIndices = m_mesh->faces().col(faceIdx);

    double x1 = m_x(faceIndices[0]), y1 = m_y(faceIndices[0]);
    double x2 = m_x(faceIndices[1]), y2 = m_y(faceIndices[1]);
    double x3 = m_x(faceIndices[2]), y3 = m_y(faceIndices[2]);

    double faceArea = m_mesh->faceAreas()(faceIdx);
    totalArea += faceArea;

    // Interpolate points within the face
    for (int i = 0; i <= m_settings.samplesPerEdge; ++i) {
      for (int j = 0; j <= m_settings.samplesPerEdge - i; ++j) {
        double a = static_cast<double>(i) / m_settings.samplesPerEdge;
        double b = static_cast<double>(j) / m_settings.samplesPerEdge;
        double c = 1.0 - a - b;

        double x = a * x1 + b * x2 + c * x3;
        double y = a * y1 + b * y2 + c * y3;

        if (x >= xmin && x < xmax && y >= ymin && y < ymax) {
          int xIndex = static_cast<int>((x - xmin) * normx);
          int yIndex = static_cast<int>((y - ymin) * normy);

          double sampleArea = faceArea / ((m_settings.samplesPerEdge + 1) *
                                          (m_settings.samplesPerEdge + 2) / 2);

          binUsed(xIndex, yIndex) = true;
          binnedAreas(xIndex, yIndex) += sampleArea;
        }
      }
    }
  }

  qDebug() << "Total surface area (interpolated):" << totalArea;
  qDebug() << "Total binned area:" << binnedAreas.sum();
  qDebug() << "Mesh surface area:" << m_mesh->surfaceArea();

  return m_mesh->surfaceArea();
}

/*
double FingerprintPlot::calculateBinnedAreasNoFilter() {
  qDebug() << "Mesh" << m_mesh;
  // unmask all faces if any
  m_mesh->resetVertexMask(false);
  qDebug() << "resetVertexMask";

  double nx = numUsedxBins();
  double ny = numUsedyBins();
  double xmax = usedxPlotMax();
  double xmin = usedxPlotMin();
  double normx = nx / (xmax - xmin);
  double ymin = usedyPlotMin();
  double ymax = usedyPlotMax();
  double normy = ny / (ymax - ymin);

  qDebug() << "Vertex areas";
  const auto &vertexAreas = m_mesh->vertexAreas();
  qDebug() << "Vertex areas calc" << vertexAreas.rows();
  double totalArea = 0.0;
  for (int i = 0; i < vertexAreas.rows(); i++) {
    double x = m_x(i);
    double y = m_y(i);

    if (x >= xmin && x < xmax && y >= ymin && y < ymax) {
      int xIndex = static_cast<int>((x - xmin) * normx);
      int yIndex = static_cast<int>((y - ymin) * normy);
      double area = vertexAreas(i);
      totalArea += area;
      if (area > 0.0) {
        binUsed(xIndex, yIndex) = true;
        binnedAreas(xIndex, yIndex) += area;
      }
    }
  }
  qDebug() << "Bin size" << binSize();
  qDebug() << "x" << m_xmin << m_xmax;
  qDebug() << "x" << m_xFaceMin << m_xFaceMax;
  qDebug() << "y" << m_ymin << m_ymax;
  qDebug() << "y" << m_yFaceMin << m_yFaceMax;
  qDebug() << nx << ny << xmax << xmin << normx << ymin << ymax << normy;
  qDebug() << "Face areas done";
  qDebug() << "Total surface area" << totalArea << binnedAreas.sum();
  qDebug() << "Total surface area" << m_mesh->surfaceArea();
  return m_mesh->surfaceArea();
}
*/

double FingerprintPlot::calculateBinnedAreasWithFilter() {
  double totalFilteredArea = 0.0;

  double nx = numUsedxBins();
  double ny = numUsedyBins();
  double xmax = usedxPlotMax();
  double xmin = usedxPlotMin();
  double normx = nx / (xmax - xmin);
  double ymin = usedyPlotMin();
  double ymax = usedyPlotMax();
  double normy = ny / (ymax - ymin);

  binnedAreas = Eigen::MatrixXd::Zero(nx, ny);
  binUsed = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>::Zero(nx, ny);

  computeFaceMask();

  auto &faceMask = m_mesh->faceMask();

  for (int faceIdx = 0; faceIdx < m_mesh->numberOfFaces(); ++faceIdx) {
    Eigen::Vector3i faceIndices = m_mesh->faces().col(faceIdx);

    double x1 = m_x(faceIndices[0]), y1 = m_y(faceIndices[0]);
    double x2 = m_x(faceIndices[1]), y2 = m_y(faceIndices[1]);
    double x3 = m_x(faceIndices[2]), y3 = m_y(faceIndices[2]);

    double faceArea = m_mesh->faceAreas()(faceIdx);

    // Interpolate points within the face
    for (int i = 0; i <= m_settings.samplesPerEdge; ++i) {
      for (int j = 0; j <= m_settings.samplesPerEdge - i; ++j) {
        double a = static_cast<double>(i) / m_settings.samplesPerEdge;
        double b = static_cast<double>(j) / m_settings.samplesPerEdge;
        double c = 1.0 - a - b;

        double x = a * x1 + b * x2 + c * x3;
        double y = a * y1 + b * y2 + c * y3;

        if (x >= xmin && x < xmax && y >= ymin && y < ymax) {
          int xIndex = static_cast<int>((x - xmin) * normx);
          int yIndex = static_cast<int>((y - ymin) * normy);

          double sampleArea = faceArea / ((m_settings.samplesPerEdge + 1) *
                                          (m_settings.samplesPerEdge + 2) / 2);

          binUsed(xIndex, yIndex) = true;
          if (faceMask(faceIdx)) {
            totalFilteredArea += sampleArea;
            binnedAreas(xIndex, yIndex) += sampleArea;
          }
        }
      }
    }
  }
  return totalFilteredArea;
}

// Used to determine a complete fingerprint breakdown for the information window
QVector<double> FingerprintPlot::filteredAreas(QString insideElementSymbol,
                                               QStringList elementSymbolList) {
  QVector<double> result;
  if (!m_mesh)
    return result;

  // Save existing filter options
  auto savedFilterMode = m_filterMode;
  bool savedIncludeReciprocalContacts = m_includeReciprocalContacts;
  int savedFilterInsideElement = m_filterInsideElement;
  int savedFilterOutsideElement = m_filterOutsideElement;
  QString savedInsideFilterElementSymbol = m_insideFilterElementSymbol;
  QString savedOutsideFilterElementSymbol = m_outsideFilterElementSymbol;

  m_filterMode = FingerprintFilterMode::Element;
  m_includeReciprocalContacts = false;
  m_filterInsideElement = true;
  m_filterOutsideElement = true;
  m_insideFilterElementSymbol = insideElementSymbol;

  int symbolListSize = elementSymbolList.size();

  QVector<double> totalFilteredArea(symbolListSize);
  totalFilteredArea.fill(0.0);

  const auto &faceAreas = m_mesh->faceAreas();
  for (int i = 0; i < symbolListSize; ++i) {
    // Sum all the areas of the faces which contribute to a particular bin
    m_outsideFilterElementSymbol = elementSymbolList[i];
    computeFaceMask();
    const auto &mask = m_mesh->faceMask();
    for (int f = 0; f < faceAreas.rows(); f++) {
      if (mask(f)) {
        totalFilteredArea[i] += faceAreas(f);
      }
    }
  }

  // Calculate percentages
  for (const auto &filteredArea : totalFilteredArea) {
    double percentageFilteredArea =
        (filteredArea / m_mesh->surfaceArea()) * 100.0;
    result.append(percentageFilteredArea);
  }

  // Restore exisiting filter options
  m_filterMode = savedFilterMode;
  m_includeReciprocalContacts = savedIncludeReciprocalContacts;
  m_filterInsideElement = savedFilterInsideElement;
  m_filterOutsideElement = savedFilterOutsideElement;
  m_insideFilterElementSymbol = savedInsideFilterElementSymbol;
  m_outsideFilterElementSymbol = savedOutsideFilterElementSymbol;
  computeFaceMask();

  return result;
}

void FingerprintPlot::calculateBinnedAreas() {
  switch (m_filterMode) {
  case FingerprintFilterMode::None:
    m_totalFilteredArea = calculateBinnedAreasNoFilter();
    break;
  default:
    m_totalFilteredArea = calculateBinnedAreasWithFilter();
    break;
  }

  // outputFingerprintAsTable();
  // outputFingerprintAsJSON();

  emit surfaceAreaPercentageChanged(
      (m_totalFilteredArea / m_mesh->surfaceArea()) * 100);
  emit surfaceFeatureChanged();
}

void FingerprintPlot::outputFingerprintAsJSON() {
  QString filename = "fingerprint.json";

  const double stdAreaForSaturatedColor = 0.001;
  const double enhancementFactor = 1.0;
  double maxValue =
      (stdAreaForSaturatedColor / enhancementFactor) * m_mesh->surfaceArea();

  bool printComma = false;

  QFile fingerprintFile(filename);
  if (fingerprintFile.open(QIODevice::WriteOnly)) {
    QTextStream out(&fingerprintFile);

    out << "[" << Qt::endl;

    int min_i = smallestxBinInCurrentPlotRange();
    int min_j = smallestyBinInCurrentPlotRange();

    int numxBins = numxBinsInCurrentPlotRange();
    int numyBins = numyBinsInCurrentPlotRange();

    auto func = ColorMapFunc(m_colorScheme);
    func.lower = 0.0;
    func.upper = maxValue;

    for (int i = 0; i < numxBins; ++i) {
      for (int j = 0; j < numyBins; ++j) {
        int iBin = i + min_i;
        int jBin = j + min_j;

        if (binUsed(iBin, jBin)) {

          QColor color = func(binnedAreas(iBin, jBin));
          if (printComma) {
            out << "," << Qt::endl;
          }

          out << "\t{" << Qt::endl;
          out << "\t\t\"x\": " << iBin << "," << Qt::endl;
          out << "\t\t\"y\": " << jBin << "," << Qt::endl;
          out << "\t\t\"col\": \"rgb(" << color.red() << "," << color.green()
              << "," << color.blue() << ")\"" << Qt::endl;
          out << "\t}";
          printComma = true;
        }
      }
    }
    out << Qt::endl;
    out << "]" << Qt::endl;
  }
  fingerprintFile.close();
}

void FingerprintPlot::outputFingerprintAsTable() {
  QString filename = "fingerprint_table";
  QFile finFile(filename);
  if (finFile.open(QIODevice::WriteOnly)) {
    QTextStream ts(&finFile);

    const double stdAreaForSaturatedColor = 0.001;
    const double enhancementFactor = 1.0;
    double maxValue =
        (stdAreaForSaturatedColor / enhancementFactor) * m_mesh->surfaceArea();

    int min_i = smallestxBinInCurrentPlotRange();
    int min_j = smallestyBinInCurrentPlotRange();

    int numxBins = numxBinsInCurrentPlotRange();
    int numyBins = numyBinsInCurrentPlotRange();

    ts << "Total surface area (used to calculate max value): "
       << m_mesh->surfaceArea() << Qt::endl;
    ts << "Min value (used for scaling): " << 0.0 << Qt::endl;
    ts << "Max value (used for scaling): " << maxValue << Qt::endl;
    ts << "Number of pixels per bin (in each direction): "
       << m_settings.pixelsPerBin << Qt::endl;
    ts << "Number of bins in de: " << numyBins << Qt::endl;
    ts << "Number of bins in di: " << numxBins << Qt::endl;
    ts << "Min di in table: " << usedxPlotMin();
    ts << "Min de in table: " << usedyPlotMin();
    ts << "Bin size: " << binSize() << " ang" << Qt::endl;
    ts << Qt::endl;
    ts << "For each bin that contains something, output" << Qt::endl;
    ts << "* Bin index in di direction" << Qt::endl;
    ts << "* Bin index in de direction" << Qt::endl;
    ts << "* Unnormalised binned areas (i.e. the sum of the triangles that "
          "contribute to that bin)"
       << Qt::endl;
    ts << "* The corresponding color of that bin" << Qt::endl;
    ts << Qt::endl;

    auto func = ColorMapFunc(m_colorScheme);
    func.lower = 0;
    func.upper = maxValue;
    for (int i = 0; i < numxBins; ++i) {
      for (int j = 0; j < numyBins; ++j) {
        int iBin = i + min_i;
        int jBin = j + min_j;

        if (binUsed(iBin, jBin)) {
          QColor color = func(binnedAreas(iBin, jBin));
          ts << i << "\t" << j << "\t" << binnedAreas(iBin, jBin) << "\tRGB("
             << color.red() << "," << color.green() << "," << color.blue()
             << ")" << Qt::endl;
        }
      }
    }
  }
  finFile.close();
}

int FingerprintPlot::binIndex(double value, double plotMin, double plotMax,
                              int numBins) {
  double plotRange = plotMax - plotMin;
  double gradient = numBins / plotRange;
  int binIndex = gradient * (value - plotMin);

  if (binIndex < 0) {
    binIndex = UNDEFINED_BIN_INDEX;
  }
  if (binIndex >= numBins) {
    binIndex = UNDEFINED_BIN_INDEX;
  }

  return binIndex;
}

// xBinIndex/yBinIndex and tolerant_xBinIndex/tolerant_yBinIndex all return the
// bin indices for a given property value
// The difference is how they handle values that fall outside the ranges of bins
// used.
// xBinIndex/yBinIndex have asserts to prevent this happening and prevent
// CrystalExplorer from continuing
// tolerant_xBinIndex/tolerant_yBinIndex return UNDEFINED_BIN_INDEX and defer
// error handling (if any) to the caller.
// The former is used when binning the data (you can't put values into bins that
// don't exist)
// The later is used you handling clicking on the fingerprint for the purposes
// of highlighting the Hirshfeld surface with red cones.

int FingerprintPlot::xBinIndex(double value) {
  int index = binIndex(value, usedxPlotMin(), usedxPlotMax(), numUsedxBins());
  Q_ASSERT(index != UNDEFINED_BIN_INDEX);
  return index;
}

int FingerprintPlot::yBinIndex(double value) {
  int index = binIndex(value, usedyPlotMin(), usedyPlotMax(), numUsedyBins());
  Q_ASSERT(index != UNDEFINED_BIN_INDEX);
  return index;
}

int FingerprintPlot::tolerant_xBinIndex(double value) {
  return binIndex(value, usedxPlotMin(), usedxPlotMax(), numUsedxBins());
}

int FingerprintPlot::tolerant_yBinIndex(double value) {
  return binIndex(value, usedyPlotMin(), usedyPlotMax(), numUsedyBins());
}

void FingerprintPlot::computeFaceMask() {
  if (!m_mesh)
    return;
  auto &mask = m_mesh->faceMask();
  auto &vmask = m_mesh->vertexMask();
  mask.setConstant(true);
  vmask.setConstant(true);

  switch (m_filterMode) {
  case FingerprintFilterMode::None:
    break;
  case FingerprintFilterMode::Element: {
    auto *structure = qobject_cast<ChemicalStructure *>(m_mesh->parent());
    if (!structure)
      break;

    auto insideNums = structure->atomicNumbersForIndices(m_mesh->atomsInside());
    auto outsideNums =
        structure->atomicNumbersForIndices(m_mesh->atomsOutside());

    Eigen::VectorXi di_idx = m_mesh->vertexProperty("di_idx").cast<int>();
    Eigen::VectorXi de_idx = m_mesh->vertexProperty("de_idx").cast<int>();

    const auto &faces = m_mesh->faces();
    const auto &v2f = m_mesh->vertexToFace();

    if (di_idx.rows() == 0 || de_idx.rows() == 0) {
      qDebug() << "Have no interior/exterior atom infor";
      break;
    }

    auto check = [](int ref, int value) {
      return (ref == -1) || (value == ref);
    };

    const int m_i = m_filterInsideElement;
    const int m_o = m_filterOutsideElement;

    for(int v = 0; v < di_idx.rows(); v++) {
        int i = insideNums(di_idx(v));
        int o = outsideNums(de_idx(v));

        vmask(v) = check(m_i, i) && check(m_o, o);
        if (m_includeReciprocalContacts) {
          vmask(v) |= (check(m_i, o) && check(m_o, i));
        }

        // if any vertex isn't in, mask it
        if(!vmask(v)) {
          for(int f: v2f[v]) {
            mask(f) = false;
          }
        }
    }
    break;
  }
  }
}

void FingerprintPlot::drawEmptyFingerprint() {
  plotPixmap = QPixmap(plotSize());
  plotPixmap.fill(PLOT_BACKGROUND_COLOR);
  QPainter painter(&plotPixmap);
  drawNoFingerprintMessage(&painter);
}

void FingerprintPlot::drawNoFingerprintMessage(QPainter *painter) {
  painter->setPen(QPen(MESSAGE_COLOR, MESSAGE_PEN_WIDTH));
  painter->setFont(MESSAGE_FONT);

  QRect boundingRect =
      painter->boundingRect(QRect(), Qt::AlignCenter, NO_FINGERPRINT_MESSAGE);
  QPoint pos(plotSize().width() / 2.0, plotSize().height() / 2.0);
  painter->drawText(boundingRect.translated(pos), Qt::AlignCenter,
                    NO_FINGERPRINT_MESSAGE);
}

void FingerprintPlot::drawFingerprint() {
  plotPixmap = QPixmap(plotSize());
  plotPixmap.fill(PLOT_BACKGROUND_COLOR);
  QPainter painter(&plotPixmap);
  drawGrid(&painter);
  drawBins(&painter);
}

void FingerprintPlot::drawGrid(QPainter *painter) {
  drawGridlines(painter);
  drawScaleLabels(painter);
  drawAxisLabels(painter);
  drawGridBoundary(painter);
}

void FingerprintPlot::drawGridlines(QPainter *painter) {
  painter->setPen(QPen(GRID_LINES_COLOR, PEN_WIDTH));

  int xMax = graphSize().width() - 1;
  int yMax = graphSize().height() - 1;

  for (int i = 1; i < numberOfGridlines(); ++i) {
    painter->drawLine(
        t(i * gridSeparation().width(), 0),
        t(i * gridSeparation().width(), yMax)); // x-axis gridlines
    painter->drawLine(
        t(0, i * gridSeparation().height()),
        t(xMax, i * gridSeparation().height())); // y-axis gridlines
  }
}

void FingerprintPlot::drawScaleLabels(QPainter *painter) {
  painter->setPen(QPen(AXIS_SCALE_TEXT_COLOR, PEN_WIDTH));
  painter->setFont(AXIS_SCALE_FONT);

  for (int i = 1; i < numberOfGridlines(); ++i) {
    // x-axis scale labels
    QString xScaleText = QString::number(plotMin() + i * gridSize(), 'f', 1);
    QRect xBoundingRect =
        painter->boundingRect(QRect(), Qt::AlignHCenter, xScaleText);
    QPoint xPosition =
        t(i * gridSeparation().width() - (xBoundingRect.width() / 2),
          -AXIS_SCALE_FONT_SIZE);
    painter->drawText(xPosition, xScaleText);

    // y-axis scale labels
    QString yScaleText = QString::number(plotMin() + i * gridSize(), 'f', 1);
    QRect yBoundingRect =
        painter->boundingRect(QRect(), Qt::AlignVCenter, yScaleText);
    QPoint yPosition = t(-yBoundingRect.width() - AXIS_SCALE_TEXT_OFFSET,
                         i * gridSeparation().height());
    painter->drawText(yBoundingRect.translated(yPosition),
                      Qt::AlignRight | Qt::AlignVCenter, yScaleText);
  }
}

void FingerprintPlot::drawAxisLabels(QPainter *painter) {
  painter->setPen(QPen(AXIS_LABEL_TEXT_COLOR, PEN_WIDTH));
  painter->setFont(AXIS_LABEL_FONT);

  // x-axis label
  int xPos = graphSize().width() - gridSeparation().width();
  int yPos = gridSeparation().height();
  QRect xRect = QRect(t(xPos, yPos), gridSeparation());
  painter->drawText(xRect, Qt::AlignHCenter | Qt::AlignVCenter, m_xAxisLabel);

  // y-axis label
  QRect yRect = QRect(t(0, graphSize().height()), gridSeparation());
  painter->drawText(yRect, Qt::AlignHCenter | Qt::AlignVCenter, m_yAxisLabel);
}

void FingerprintPlot::drawGridBoundary(QPainter *painter) {
  painter->setPen(QPen(GRID_BOUNDARY_COLOR, PEN_WIDTH));

  int xMax = graphSize().width() - 1;
  int yMax = graphSize().height() - 1;

  painter->drawLine(t(0, 0), t(xMax, 0));
  painter->drawLine(t(xMax, 0), t(xMax, yMax));
  painter->drawLine(t(0, 0), t(0, yMax));
  painter->drawLine(t(0, yMax), t(xMax, yMax));
}

void FingerprintPlot::drawBins(QPainter *painter) {
  painter->setPen(Qt::NoPen);

  const double stdAreaForSaturatedColor = 0.001;
  const double enhancementFactor = 1.0;

  int nBins = numberOfBins();
  double pointRatio = graphRect().size().width() / static_cast<double>(nBins);
  double maxValue =
      (stdAreaForSaturatedColor / enhancementFactor) * m_mesh->surfaceArea();

  QColor color;

  int min_i = smallestxBinInCurrentPlotRange();
  int min_j = smallestyBinInCurrentPlotRange();

  int numxBins = numxBinsInCurrentPlotRange();
  int numyBins = numyBinsInCurrentPlotRange();

  int xOffset = xOffsetForCurrentPlotRange();
  int yOffset = yOffsetForCurrentPlotRange();

  auto func = ColorMapFunc(m_colorScheme);
  func.lower = 0.0;
  func.upper = maxValue;
  func.reverse = true;
  qDebug() << "Pixels per bin" << m_settings.pixelsPerBin;
  qDebug() << "Surface area" << m_mesh->surfaceArea();
  qDebug() << "Surface area (faces)" << m_mesh->faceAreas().sum();
  qDebug() << "Surface area (verts)" << m_mesh->vertexAreas().sum();

  for (int i = 0; i < numxBins; ++i) {
    for (int j = 0; j < numyBins; ++j) {
      int iBin = i + min_i;
      int jBin = j + min_j;
      if (binUsed(iBin, jBin)) {
        if (binnedAreas(iBin, jBin) > 0.0) {
          color = func(binnedAreas(iBin, jBin));
        } else {
          color = MASKED_BIN_COLOR;
        }
        painter->setBrush(QBrush(color, Qt::SolidPattern));
        QPoint pos = t((xOffset + i) * pointRatio, (yOffset + j) * pointRatio);
        painter->drawRect(pos.x(), pos.y() - (m_settings.pixelsPerBin / 2),
                          m_settings.pixelsPerBin, m_settings.pixelsPerBin);
      }
    }
  }
}

/*
 \brief Converts (x,y) from our graph-centric coordinate system to a QPoint in
 the Qt coordinate system.

 O----->X-----------------------|
 |                  "Plot"      |
 |                              |
 |    |--------------------|    |
 |    |                    |    |
 V    |                    |    |
 Y    |                    |    |
 |    |                    |    |
 |    Y                    |    |
 |    ^                    |    |
 |    |           "Graph"  |    |
 |    |                    |    |
 |    |                    |    |
 |    O'------>X-----------|    |
 |                              |
 |------------------------------|

 All of the drawing code for the fingerprint is done relative to:
 Origin O' with the x-axis increasing to the 'right' and the y-axis increasing
 'up'.

 However the coordinate system for Qt is relative to the top-left:
 Origin O with the x-axis increasing to the 'right' and the y-axis increasing
 'down'.

 This routine converts coordinates (x,y) from our more natural graph-centric
 coordinate system to the Qt
 coordinate system.
 */
QPoint FingerprintPlot::t(int x, int y) {
  int newX = x + leftMargin();
  int newY = plotSize().height() - y - bottomMargin() - 1;
  return QPoint(newX, newY);
}

/*! Inverse function to FingerprintPlot::t
 Converts from "plot" coordinates in the Qt system to "graph" coordinates in our
 system.
 */
QPoint FingerprintPlot::tinv(int x, int y) {
  int newX = x - leftMargin();
  int newY = plotSize().height() - bottomMargin() - y - 1;
  return QPoint(newX, newY);
}

void FingerprintPlot::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QStylePainter painter(this);
  painter.drawPixmap(0, 0, plotPixmap);
}

void FingerprintPlot::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_mesh->resetVertexHighlights();
    QPair<int, int> indices = binIndicesAtMousePosition(event->pos());
    if (indices.first != -1 && indices.second != -1) {
      highlightVerticesWithPropertyValues(indices);
    }
  }
}

QPair<int, int> FingerprintPlot::binIndicesAtMousePosition(QPoint pos) {
  QPoint graphPos = tinv(pos.x(), pos.y());

  return binIndicesAtGraphPos(graphPos);
}

QPair<int, int> FingerprintPlot::binIndicesAtGraphPos(QPoint graphPos) {
  QPair<int, int> binIndices;

  double plotRange = plotMax() - plotMin();
  double xGradient = plotRange / graphSize().width();
  double yGradient = plotRange / graphSize().height();

  double xProperty = graphPos.x() * xGradient + plotMin();
  double yProperty = graphPos.y() * yGradient + plotMin();

  binIndices.first = tolerant_xBinIndex(xProperty);
  binIndices.second = tolerant_yBinIndex(yProperty);

  return binIndices;
}

void FingerprintPlot::highlightVerticesWithPropertyValues(
    QPair<int, int> binIndicesAtMousePos) {

  double const D2_THRESHOLD = 4.1;

  // Consider the first vertex (index 0) of the surface and calculate the
  // squared distance of the bin indices to where the mouse click occurred. Keep
  // the face if it's less than D2_THRESHOLD.
  int vertex{-1};
  double dx = xBinIndex(m_x(0)) - binIndicesAtMousePos.first;
  double dy = yBinIndex(m_y(0)) - binIndicesAtMousePos.second;
  double d2min = dx * dx + dy * dy;
  if (d2min < D2_THRESHOLD) {
    vertex = 0;
  } else {
    vertex = -1;
    d2min = D2_THRESHOLD;
  }

  // Find the vertex with the smallest squared distance
  // and store it's index in "vertex"
  for (int v = 0; v < m_x.rows(); ++v) {
    double dx = xBinIndex(m_x(v)) - binIndicesAtMousePos.first;
    double dy = yBinIndex(m_y(v)) - binIndicesAtMousePos.second;
    double d2 = dx * dx + dy * dy;
    if (d2 < d2min) {
      d2min = d2;
      vertex = v;
    }
  }
  // Turn on all vertex that have the same bin indices as "vertex"
  if (vertex != -1) {
    int xBin = xBinIndex(m_x(vertex));
    int yBin = yBinIndex(m_y(vertex));

    for (int v = 0; v < m_x.rows(); ++v) {
      bool xBinTest = xBinIndex(m_x(v)) == xBin;
      bool yBinTest = yBinIndex(m_y(v)) == yBin;
      if (xBinTest && yBinTest) {
        m_mesh->highlightVertex(v);
      }
    }
    emit surfaceFeatureChanged();
  } else if (vertex == -1) {
    resetSurfaceFeatures(false);
  }
}

void FingerprintPlot::resetSurfaceFeatures(bool mask) {
  if (m_mesh) {
    m_mesh->resetVertexHighlights();
    if(mask) {
      m_mesh->resetFaceMask(true);
      m_mesh->resetVertexMask(true);
    }
    emit surfaceFeatureChanged();
  }
}

void FingerprintPlot::saveFingerprint(QString filename) {
  QFileInfo fi(filename);
  if (fi.suffix() == "eps") {
    QString title = QInputDialog::getText(
        nullptr, tr("Enter fingerprint title"),
        tr("(Leave blank for no title)"), QLineEdit::Normal);
    saveFingerprintAsEps(filename, title);
  } else if (fi.suffix() == "png") {
    saveFingerprintAsPNG(filename);
  } else if (fi.suffix() == "csv") {
    if (settings::readSetting(settings::keys::ALLOW_CSV_FINGERPRINT_EXPORT)
            .toBool()) {
      saveFingerprintAsCSV(filename);
    }
  } else { // Unknown file format for saving fingerprints
  }
}

////////////////////////////////////////////// PNG
/////////////////////////////////////////////////////////////

void FingerprintPlot::saveFingerprintAsPNG(QString filename) {
  if (!plotPixmap.save(filename, "PNG")) {
    QMessageBox::critical(this, tr("Unable to save image"),
                          tr("Error saving fingerprint plot."));
  }
}

////////////////////////////////////////////// Comma Separate Values functions
/////////////////////////////////////////////////////////////

void FingerprintPlot::saveFingerprintAsCSV(QString filename) {
  QFile datafile(filename);
  if (datafile.open(QIODevice::WriteOnly)) {
    QTextStream ts(&datafile);

    double binScale = (plotMax() - plotMin()) / numberOfBins();

    int min_i = smallestxBinInCurrentPlotRange();
    int min_j = smallestyBinInCurrentPlotRange();

    int numxBins = numxBinsInCurrentPlotRange();
    int numyBins = numyBinsInCurrentPlotRange();

    int xOffset = xOffsetForCurrentPlotRange();
    int yOffset = yOffsetForCurrentPlotRange();

    ts << "x,y,value" << Qt::endl;
    for (int i = 0; i < numxBins; ++i) {
      for (int j = 0; j < numyBins; ++j) {
        int iBin = i + min_i;
        int jBin = j + min_j;

        if (binUsed(iBin, jBin)) {
          if (binnedAreas(iBin, jBin) > 0.0) {
            double x = (xOffset + i) * binScale + plotMin();
            double y = (yOffset + j) * binScale + plotMin();
            ts << QString::number(x, 'f', 5) << ", "
               << QString::number(y, 'f', 5) << ", "
               << QString::number(binnedAreas(iBin, jBin), 'f', 5) << Qt::endl;
          }
        }
      }
    }
  }
}

void FingerprintPlot::saveFingerprintAsEps(QString filename, QString title) {
  QFile file(filename);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream ts(&file);

    FingerprintEpsWriter epsWriter(numberOfBins(), plotMin(), plotMax(),
                                   binSize(), numberOfGridlines(), gridSize());

    epsWriter.setXOffset(xOffsetForCurrentPlotRange());
    epsWriter.setYOffset(yOffsetForCurrentPlotRange());
    epsWriter.writeEpsFile(ts, title, binnedAreas, MASKED_BIN_COLOR);

    file.close();
  }
}

/////////////////////////////////////////// Determining actual plot size
////////////////////////////////////////////////////
// This is the range of the used bins of the x- and y-property
// Unlike the routines in the "Determining plot size" section determine the plot
// based on the plot range (standard, expanded, translated)

double FingerprintPlot::findLowerBound(double value, double min,
                                       double stepSize) {
  Q_ASSERT(value >= min);
  Q_ASSERT(stepSize > 0.0);

  int i = 0;
  while (true) {
    i++;
    double bound = min + i * stepSize;
    if (value < bound) {
      break;
    }
  }
  return min + (i - 1) * stepSize;
}

double FingerprintPlot::usedxPlotMin() {
  return findLowerBound(m_xmin, 0.0, binSize());
}

double FingerprintPlot::usedxPlotMax() {
  return findLowerBound(m_xmax, usedxPlotMin(), binSize()) + binSize();
}

double FingerprintPlot::usedyPlotMin() {
  return findLowerBound(m_ymin, 0.0, binSize());
}

double FingerprintPlot::usedyPlotMax() {
  return findLowerBound(m_ymax, usedyPlotMin(), binSize()) + binSize();
}

int FingerprintPlot::numUsedxBins() {
  double plotRange = usedxPlotMax() - usedxPlotMin();
  return plotRange / binSize();
}

int FingerprintPlot::numUsedyBins() {
  double plotRange = usedyPlotMax() - usedyPlotMin();
  return plotRange / binSize();
}

int FingerprintPlot::xOffset() {
  return (usedxPlotMin() - plotMin()) / binSize();
}

int FingerprintPlot::yOffset() {
  return (usedyPlotMin() - plotMin()) / binSize();
}

int FingerprintPlot::smallestxBinInCurrentPlotRange() {
  int xOff = xOffset();
  return (xOff < 0) ? abs(xOff) : 0;
}

int FingerprintPlot::smallestyBinInCurrentPlotRange() {
  int yOff = yOffset();
  return (yOff < 0) ? abs(yOff) : 0;
}

int FingerprintPlot::numxBinsInCurrentPlotRange() {
  int xOff = xOffset();
  if (xOff > 0) {
    return qMin(numberOfBins() - xOff, numUsedxBins());
  } else {
    return qMin(numberOfBins(), numUsedxBins() + xOff);
  }
}

int FingerprintPlot::numyBinsInCurrentPlotRange() {
  int yOff = yOffset();
  if (yOff > 0) {
    return qMin(numberOfBins() - yOff, numUsedyBins());
  } else {
    return qMin(numberOfBins(), numUsedyBins() + yOff);
  }
}

int FingerprintPlot::xOffsetForCurrentPlotRange() {
  int xOff = xOffset();
  return (xOff < 0) ? 0 : xOff;
}

int FingerprintPlot::yOffsetForCurrentPlotRange() {
  int yOff = yOffset();
  return (yOff < 0) ? 0 : yOff;
}

/////////////////////////////////////////// Determining plot size
///////////////////////////////////////////////////////////

int FingerprintPlot::numberOfGridlines() const {
  return round((plotMax() - plotMin()) / gridSize());
}

/*! Returns the number of bins for a given plot range and bin size
 The original version of CrystalExplorer used integer truncation
 which we have preserved here to be consistent.
 However rounding seems more natural to me -- mjt
 */
int FingerprintPlot::numberOfBins() const {
  double plotRange = plotMax() - plotMin();
  return plotRange / binSize();
}

int FingerprintPlot::leftMargin() const { return AXIS_SCALE_OFFSET; }

int FingerprintPlot::rightMargin() const { return 0.0 * graphSize().width(); }

int FingerprintPlot::topMargin() const { return 0.0 * graphSize().width(); }

int FingerprintPlot::bottomMargin() const { return AXIS_SCALE_OFFSET; }

QRect FingerprintPlot::plotRect() const {
  int plotWidth = leftMargin() + graphSize().width() + rightMargin();
  int plotHeight = topMargin() + graphSize().height() + bottomMargin();

  return QRect(QPoint(0, 0), QSize(plotWidth, plotHeight));
}

QRect FingerprintPlot::graphRect() const {
  int graphWidthInPixels = m_settings.pixelsPerBin * numberOfBins();
  QSize graphSize =
      QSize(graphWidthInPixels,
            graphWidthInPixels); // Make the graph height equal to the width

  return QRect(QPoint(0, 0), graphSize);
}

QSize FingerprintPlot::graphSize() const { return graphRect().size(); }

QSize FingerprintPlot::plotSize() const { return plotRect().size(); }

double FingerprintPlot::plotMin() const { return m_settings.rangeMinimum; }

double FingerprintPlot::plotMax() const { return m_settings.rangeMaximum; }

double FingerprintPlot::binSize() const { return m_settings.binSize; }

double FingerprintPlot::gridSize() const { return m_settings.gridSize; }

QSize FingerprintPlot::gridSeparation() const {
  return QSize(graphSize().width() / numberOfGridlines(),
               graphSize().height() / numberOfGridlines());
}
