#include "fingerprintplot.h"

#include <QPen>
#include <QStylePainter>
#include <QInputDialog>
#include <QRegularExpression>
#include <QMessageBox>
#include <QTime>
#include <QtDebug>

#include "colorschemer.h"
#include "settings.h"

FingerprintPlot::FingerprintPlot(QWidget *parent) : QWidget(parent) { init(); }

void FingerprintPlot::init() {
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  _crystal = nullptr;
  _surface = nullptr;
  setRange(DEFAULT_PLOT_RANGE);
  setPlotType(DEFAULT_PLOT_TYPE);

  resetFilter();
}

void FingerprintPlot::resetFilter() {
  setFilter(noFilter, false, false, false, QString(), QString());
}

void FingerprintPlot::setFilter(FingerprintFilterMode filterMode,
                                bool includeReciprocalContacts,
                                bool filterInsideElement,
                                bool filterOutsideElement,
                                QString insideFilterElementSymbol,
                                QString outsideFilterElementSymbol) {
  _filterMode = filterMode;
  _includeReciprocalContacts = includeReciprocalContacts;
  _filterInsideElement = filterInsideElement;
  _filterOutsideElement = filterOutsideElement;
  _insideFilterElementSymbol = insideFilterElementSymbol;
  _outsideFilterElementSymbol = outsideFilterElementSymbol;
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

void FingerprintPlot::setPlotType(PlotType plotType) {
  _plotType = plotType;
  switch (_plotType) {
  case dPlot:
    _plotMinForRange = dPlotMinForRange;
    _plotMaxForRange = dPlotMaxForRange;
    _plotBinSizeForRange = dPlotBinSizeForRange;
    _plotGridSizeForRange = dPlotGridSizeForRange;
    break;
  case dnormPlot:
    _plotMinForRange = dnormPlotMinForRange;
    _plotMaxForRange = dnormPlotMaxForRange;
    _plotBinSizeForRange = dnormPlotBinSizeForRange;
    _plotGridSizeForRange = dnormPlotGridSizeForRange;
    break;
  }
  _plotTypeChanged = true;
}

void FingerprintPlot::updatePlotType(PlotType plotType) {
  setPlotType(plotType);
  updateFingerprintPlot();
}

void FingerprintPlot::setRange(PlotRange range) { _range = range; }

void FingerprintPlot::updatePlotRange(PlotRange range) {
  setRange(range);
  updateFingerprintPlot();
}

void FingerprintPlot::setCrystalAndSurface(DeprecatedCrystal *crystal,
                                           Surface *surface) {
  _crystal = crystal;
  _surface = surface;
  // Ugly hack. This forces the properties to be reset for the new crystal
  // See updateFingerprintPlot
  _plotTypeChanged = true;

  updateFingerprintPlot();
}

void FingerprintPlot::setPropertiesToPlot() {
  int xPropertyIndex = xPropertyIndices[_plotType];
  int yPropertyIndex = yPropertyIndices[_plotType];

  // Can't use properties that are not part of the surface
  Q_ASSERT(xPropertyIndex >= 0 &&
           xPropertyIndex < _surface->numberOfProperties());
  Q_ASSERT(yPropertyIndex >= 0 &&
           yPropertyIndex < _surface->numberOfProperties());

  _xPropertyIndex = xPropertyIndex;
  _yPropertyIndex = yPropertyIndex;

  _xPropertyAtFace.clear();
  _yPropertyAtFace.clear();

  // Calculate and store the property values for each face (avg of three
  // vertices)

  _xPropertyMin = _surface->propertyAtIndex(_xPropertyIndex)->max();
  _xPropertyMax = 0.0;
  _yPropertyMin = _surface->propertyAtIndex(_yPropertyIndex)->max();
  _yPropertyMax = 0.0;

  for (int f = 0; f < _surface->numberOfFaces(); ++f) {
    double xValue = _surface->valueForPropertyAtFace(f, _xPropertyIndex);
    double yValue = _surface->valueForPropertyAtFace(f, _yPropertyIndex);

    _xPropertyAtFace.append(xValue);
    _yPropertyAtFace.append(yValue);

    _xPropertyMin = qMin(_xPropertyMin, xValue);
    _xPropertyMax = qMax(_xPropertyMax, xValue);
    _yPropertyMin = qMin(_yPropertyMin, yValue);
    _yPropertyMax = qMax(_yPropertyMax, yValue);
  }

  setAxisLabels();

  _plotTypeChanged = false; // reset flag
}

void FingerprintPlot::setAxisLabels() {
  QStringList propertyLabels = _surface->listOfProperties();

  _xAxisLabel = propertyLabels[_xPropertyIndex].split(" ")[0];
  _yAxisLabel = propertyLabels[_yPropertyIndex].split(" ")[0];
}

void FingerprintPlot::updateFingerprintPlot() {
  if (_surface != nullptr && _surface->isHirshfeldBased()) {
    if (_plotTypeChanged) {
      setPropertiesToPlot();
    }
    initBinnedAreas();
    initBinnedFilterFlags();
    calculateBinnedAreas();
    drawFingerprint();
  } else {
    drawEmptyFingerprint();
  }
  setFixedSize(plotSize());
  parentWidget()->adjustSize();
  update();
}
/*
void FingerprintPlot::initBinnedAreas()
{
        int nBins = numberOfBins();
        binnedAreas = QVector<QVector<double> >(nBins);

    for (int i = 0; i < nBins; i++) {
                QVector<double> innerVec = QVector<double>(nBins);
                qFill(innerVec.begin(), innerVec.end(), 0.0);
                binnedAreas[i] = innerVec;
        }
}
*/
void FingerprintPlot::initBinnedAreas() {
  int numxBins = numUsedxBins();
  int numyBins = numUsedyBins();
  binnedAreas = MatrixXq::Zero(numxBins, numyBins);
}
/*
void FingerprintPlot::initBinnedFilterFlags()
{
        int nBins = numberOfBins();
        binUsed = QVector<QVector<bool> >(nBins);

    for (int i = 0; i < nBins; i++) {
                QVector<bool> innerVec = QVector<bool>(nBins);
                qFill(innerVec.begin(), innerVec.end(), false);
                binUsed[i] = innerVec;
        }
}
*/

void FingerprintPlot::initBinnedFilterFlags() {
  int numxBins = numUsedxBins();
  int numyBins = numUsedyBins();
  binUsed = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>::Zero(numxBins,
                                                                      numyBins);
}

double FingerprintPlot::calculateBinnedAreasNoFilter() {
  if (_surface->hasMaskedFaces()) {
    _surface->resetMaskedFaces();
  }

  double nx = numUsedxBins();
  double ny = numUsedyBins();
  double xmax = usedxPlotMax();
  double xmin = usedxPlotMin();
  double normx = nx / (xmax - xmin);
  double ymin = usedyPlotMin();
  double ymax = usedyPlotMax();
  double normy = ny / (ymax - ymin);

  const auto &face_areas = _surface->faceAreas();
  for (int f = 0; f < _surface->numberOfFaces(); ++f) {
    double x = _xPropertyAtFace[f];
    double y = _yPropertyAtFace[f];

    if (x >= xmin && x < xmax && y >= ymin && y < ymax) {
      int xIndex = static_cast<int>((x - xmin) * normx);
      int yIndex = static_cast<int>((y - xmin) * normy);
      double area = face_areas[f];
      if (area > 0.0) {
        binUsed(xIndex, yIndex) = true;
        binnedAreas(xIndex, yIndex) += area;
      }
    }
  }
  return _surface->area();
}

double FingerprintPlot::calculateBinnedAreasWithFilter() {
  double totalFilteredArea = 0.0;

  // mask all faces
  _surface->resetMaskedFaces(true);

  double nx = numUsedxBins();
  double ny = numUsedyBins();
  double xmax = usedxPlotMax();
  double xmin = usedxPlotMin();
  double normx = nx / (xmax - xmin);
  double ymin = usedyPlotMin();
  double ymax = usedyPlotMax();
  double normy = ny / (ymax - ymin);

  const auto &face_areas = _surface->faceAreas();
  for (int f = 0; f < _surface->numberOfFaces(); ++f) {
    double x = _xPropertyAtFace[f];
    double y = _yPropertyAtFace[f];

    if (x >= xmin && x < xmax && y >= ymin && y < ymax) {
      int xIndex = static_cast<int>((x - xmin) * normx);
      int yIndex = static_cast<int>((y - xmin) * normy);
      double area = face_areas[f];
      if (area > 0.0) {
        binUsed(xIndex, yIndex) = true;
        if (includeArea(f)) {
          totalFilteredArea += area;
          binnedAreas(xIndex, yIndex) += area;
          _surface->unmaskFace(f);
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
  if (!_surface)
    return result;

  // Save existing filter options
  FingerprintFilterMode savedFilterMode = _filterMode;
  bool savedIncludeReciprocalContacts = _includeReciprocalContacts;
  bool savedFilterInsideElement = _filterInsideElement;
  bool savedFilterOutsideElement = _filterOutsideElement;
  QString savedInsideFilterElementSymbol = _insideFilterElementSymbol;
  QString savedOutsideFilterElementSymbol = _outsideFilterElementSymbol;

  _filterMode = elementFilter;
  _includeReciprocalContacts = false;
  _filterInsideElement = true;
  _filterOutsideElement = true;
  _insideFilterElementSymbol = insideElementSymbol;

  int symbolListSize = elementSymbolList.size();

  QVector<double> totalFilteredArea(symbolListSize);
  totalFilteredArea.fill(0.0);

  // Sum all the areas of the faces which contribute to a particular bin
  for (int f = 0; f < _surface->numberOfFaces(); ++f) {
    for (int i = 0; i < symbolListSize; ++i) {
      _outsideFilterElementSymbol = elementSymbolList[i];
      if (includeArea(f)) {
        totalFilteredArea[i] += _surface->areaOfFace(f);
      }
    }
  }

  // Calculate percentages
  for (const auto &filteredArea : totalFilteredArea) {
    double percentageFilteredArea = (filteredArea / _surface->area()) * 100.0;
    result.append(percentageFilteredArea);
  }

  // Restore exisiting filter options
  _filterMode = savedFilterMode;
  _includeReciprocalContacts = savedIncludeReciprocalContacts;
  _filterInsideElement = savedFilterInsideElement;
  _filterOutsideElement = savedFilterOutsideElement;
  _insideFilterElementSymbol = savedInsideFilterElementSymbol;
  _outsideFilterElementSymbol = savedOutsideFilterElementSymbol;

  return result;
}

void FingerprintPlot::calculateBinnedAreas() {
  Q_ASSERT(_xPropertyAtFace.size() == _surface->numberOfFaces());
  Q_ASSERT(_yPropertyAtFace.size() == _surface->numberOfFaces());

  switch (_filterMode) {
  case noFilter:
    _totalFilteredArea = calculateBinnedAreasNoFilter();
    break;
  default:
    _totalFilteredArea = calculateBinnedAreasWithFilter();
    break;
  }

  // outputFingerprintAsTable();
  // outputFingerprintAsJSON();

  emit surfaceAreaPercentageChanged((_totalFilteredArea / _surface->area()) *
                                    100);
  emit surfaceFeatureChanged();
}

void FingerprintPlot::outputFingerprintAsJSON() {
  const QString EXTENSION = "json";
  QString filename = _crystal->crystalName() + "." + EXTENSION;

  const double stdAreaForSaturatedColor = 0.001;
  const double enhancementFactor = 1.0;
  double maxValue =
      (stdAreaForSaturatedColor / enhancementFactor) * _surface->area();

  bool printComma = false;

  QFile fingerprintFile(filename);
  if (fingerprintFile.open(QIODevice::WriteOnly)) {
    QTextStream out(&fingerprintFile);

    out << "[" << Qt::endl;

    int min_i = smallestxBinInCurrentPlotRange();
    int min_j = smallestyBinInCurrentPlotRange();

    int numxBins = numxBinsInCurrentPlotRange();
    int numyBins = numyBinsInCurrentPlotRange();

    for (int i = 0; i < numxBins; ++i) {
      for (int j = 0; j < numyBins; ++j) {
        int iBin = i + min_i;
        int jBin = j + min_j;

        if (binUsed(iBin, jBin)) {
          QColor color = ColorSchemer::color(
              m_colorScheme, binnedAreas(iBin, jBin), 0.0, maxValue, true);

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
        (stdAreaForSaturatedColor / enhancementFactor) * _surface->area();

    int min_i = smallestxBinInCurrentPlotRange();
    int min_j = smallestyBinInCurrentPlotRange();

    int numxBins = numxBinsInCurrentPlotRange();
    int numyBins = numyBinsInCurrentPlotRange();

    ts << "Total surface area (used to calculate max value): "
       << _surface->area() << Qt::endl;
    ts << "Min value (used for scaling): " << 0.0 << Qt::endl;
    ts << "Max value (used for scaling): " << maxValue << Qt::endl;
    ts << "Number of pixels per bin (in each direction): " << PIXELS_PER_BIN
       << Qt::endl;
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

    for (int i = 0; i < numxBins; ++i) {
      for (int j = 0; j < numyBins; ++j) {
        int iBin = i + min_i;
        int jBin = j + min_j;

        if (binUsed(iBin, jBin)) {
          QColor color = ColorSchemer::color(
              m_colorScheme, binnedAreas(iBin, jBin), 0.0, maxValue, true);
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

bool FingerprintPlot::includeArea(int face) {
  bool incArea = true;

  switch (_filterMode) {
  case noFilter:
    break;
  case selectionFilter:
    incArea = includeAreaFilteredBySelection(face);
    break;
  case elementFilter:
    incArea = includeAreaFilteredByElement(face);
    break;
  }
  return incArea;
}

bool FingerprintPlot::includeAreaFilteredBySelection(int face) {
  return true;
}

bool FingerprintPlot::includeAreaFilteredByElement(int face) {
  bool incArea;

  const QVector<Atom> &unitCellAtoms = _crystal->unitCellAtoms();

  QString insideElement =
      unitCellAtoms[_surface->insideAtomIdForFace(face).unitCellIndex].symbol();
  QString outsideElement =
      unitCellAtoms[_surface->outsideAtomIdForFace(face).unitCellIndex]
          .symbol();

  bool insideMatch = _insideFilterElementSymbol == insideElement;
  bool outsideMatch = _outsideFilterElementSymbol == outsideElement;

  if (_includeReciprocalContacts) {

    incArea = (insideMatch && outsideMatch) ||
              (outsideElement == _insideFilterElementSymbol &&
               insideElement == _outsideFilterElementSymbol);

    /* Note: The above is NOT equivalent to the following: */
    // incArea = (insideElement == _insideFilterElementSymbol || insideElement
    // == _outsideFilterElementSymbol)
    // && (outsideElement == _insideFilterElementSymbol || outsideElement ==
    // _outsideFilterElementSymbol);
  } else {
    if (_filterInsideElement && _filterOutsideElement) {
      incArea = insideMatch && outsideMatch;
    } else if (_filterInsideElement && !_filterOutsideElement) {
      incArea = insideMatch;
    } else if (!_filterInsideElement && _filterOutsideElement) {
      incArea = outsideMatch;
    } else { // filter neither inside or outside element
      incArea = true;
    }
  }
  return incArea;
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
  painter->drawText(xRect, Qt::AlignHCenter | Qt::AlignVCenter, _xAxisLabel);

  // y-axis label
  QRect yRect = QRect(t(0, graphSize().height()), gridSeparation());
  painter->drawText(yRect, Qt::AlignHCenter | Qt::AlignVCenter, _yAxisLabel);
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
  double pointRatio = graphRect().size().width() / (double)nBins;
  double maxValue =
      (stdAreaForSaturatedColor / enhancementFactor) * _surface->area();

  QColor color;

  int min_i = smallestxBinInCurrentPlotRange();
  int min_j = smallestyBinInCurrentPlotRange();

  int numxBins = numxBinsInCurrentPlotRange();
  int numyBins = numyBinsInCurrentPlotRange();

  int xOffset = xOffsetForCurrentPlotRange();
  int yOffset = yOffsetForCurrentPlotRange();

  for (int i = 0; i < numxBins; ++i) {
    for (int j = 0; j < numyBins; ++j) {
      int iBin = i + min_i;
      int jBin = j + min_j;
      if (binUsed(iBin, jBin)) {
        if (binnedAreas(iBin, jBin) > 0.0) {
          color = ColorSchemer::color(m_colorScheme, binnedAreas(iBin, jBin),
                                      0.0, maxValue, true);
        } else {
          color = MASKED_BIN_COLOR;
        }
        painter->setBrush(QBrush(color, Qt::SolidPattern));
        QPoint pos = t((xOffset + i) * pointRatio, (yOffset + j) * pointRatio);
        painter->drawRect(pos.x(), pos.y() - (PIXELS_PER_BIN / 2),
                          PIXELS_PER_BIN, PIXELS_PER_BIN);
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
    _surface->resetFaceHighlights();
    QPair<int, int> indices = binIndicesAtMousePosition(event->pos());
    if (indices.first != -1 && indices.second != -1) {
      highlightFacesWithPropertyValues(indices);
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

// The logic of this routine was taken from the previous version of
// CrystalExplorer
// from function FingerprintPlot::setOnFacesFromXY
// Comments below may not be correct
void FingerprintPlot::highlightFacesWithPropertyValues(
    QPair<int, int> binIndicesAtMousePos) {
  Q_ASSERT(_surface != nullptr);

  double const D2_THRESHOLD = 4.1;

  // Consider the first face (index 0) of the surface and calculate the squared
  // distance of the bin indices
  // to where the mouse click occurred. Keep the face if it's less than
  // D2_THRESHOLD.
  int face;
  double dx = xBinIndex(_xPropertyAtFace[0]) - binIndicesAtMousePos.first;
  double dy = yBinIndex(_yPropertyAtFace[0]) - binIndicesAtMousePos.second;
  double d2min = dx * dx + dy * dy;
  if (d2min < D2_THRESHOLD) {
    face = 0;
  } else {
    face = -1;
    d2min = D2_THRESHOLD;
  }

  // Find the face with the smallest squared distance
  // and store it's index in "face"
  for (int f = 0; f < _surface->numberOfFaces(); ++f) {
    double dx = xBinIndex(_xPropertyAtFace[f]) - binIndicesAtMousePos.first;
    double dy = yBinIndex(_yPropertyAtFace[f]) - binIndicesAtMousePos.second;
    double d2 = dx * dx + dy * dy;
    if (d2 < d2min) {
      d2min = d2;
      face = f;
    }
  }
  // Turn on all faces that have the same bin indices as "face"
  if (face != -1) {
    int xBin = xBinIndex(_xPropertyAtFace[face]);
    int yBin = yBinIndex(_yPropertyAtFace[face]);

    for (int f = 0; f < _surface->numberOfFaces(); ++f) {
      bool xBinTest = xBinIndex(_xPropertyAtFace[f]) == xBin;
      bool yBinTest = yBinIndex(_yPropertyAtFace[f]) == yBin;
      if (xBinTest && yBinTest) {
        _surface->highlightFace(f);
      }
    }
    emit surfaceFeatureChanged();
  } else if (face == -1) {
    // Switch of all face highlights if blank area of fingerprint plot is
    // clicked on.
    emit resetSurfaceFeatures();
  }
}

void FingerprintPlot::saveFingerprint(QString filename) {
  QFileInfo fi(filename);
  if (fi.suffix() == "eps") {
    QString title = QInputDialog::getText(
        nullptr, tr("Enter fingerprint title"),
        tr("(Leave blank for no title)"), QLineEdit::Normal);
    saveFingerprintAsEps(filename, title);
  } else if (fi.suffix() == "svg") {
    saveFingerprintAsSVG(filename);
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

////////////////////////////////////////////// Postscript functions
/////////////////////////////////////////////////////////////

/*! Save the fingerprint as Encapsulated Postscript to filename
 */
void FingerprintPlot::saveFingerprintAsEps(QString filename, QString title) {
  QFile epsfile(filename);
  if (epsfile.open(QIODevice::WriteOnly)) {
    QTextStream ts(&epsfile);

    writeEpsHeader(ts, title);
    writeEpsTitle(ts, title);
    writeEpsGridBoundary(ts);
    writeEpsAxisLabels(ts);
    writeEpsGridlinesAndScaleLabels(ts);
    writeEpsBins(ts);
    writeEpsFooter(ts);

    epsfile.close();
  }
}

void FingerprintPlot::writeEpsHeader(QTextStream &ts, QString title) {
  // Create title that goes into EPS header
  QString shortTitle = title;
  shortTitle.remove(QRegularExpression("[_^{}]"));

  // Calculating bounding box dimensions
  int x0 = int((EPS_OFFSETX - EPS_MARGIN_LEFT) * EPS_DPCM);
  int y0 = int((EPS_OFFSETY - EPS_MARGIN_BOTTOM) * EPS_DPCM);
  int x1 = int((EPS_OFFSETX + EPS_SIZE + EPS_MARGIN_RIGHT) * EPS_DPCM);
  int y1 = int((EPS_OFFSETY + EPS_SIZE + EPS_MARGIN_TOP) * EPS_DPCM);

  ts << "%!PS-Adobe-3.0 EPSF-3.0" << Qt::endl;
  ts << "%%Creator: CrystalExplorer" << Qt::endl;
  ts << "%%Title: " << shortTitle.toLatin1() << Qt::endl;
  ts << "%%BoundingBox: " << x0 << " " << y0 << " " << x1 << " " << y1
     << Qt::endl;
  ts << "%%LanguageLevel: 2" << Qt::endl;
  ts << "%%Pages: 1" << Qt::endl;
  ts << "%%EndComments" << Qt::endl;
  ts << "%%EndProlog" << Qt::endl;
  ts << "%%Page: 1 1" << Qt::endl;
  ts << "gsave" << Qt::endl;
  ts << Qt::endl;

  ts << "% Use the ISOLatin1 encoding to get the Angstrom symbol" << Qt::endl;
  ts << "/LucidaSansLatin-Italic" << Qt::endl;
  ts << " << /LucidaSans-Italic findfont {} forall >>" << Qt::endl;
  ts << " begin" << Qt::endl;
  ts << "  /Encoding ISOLatin1Encoding 256 array copy def currentdict"
     << Qt::endl;
  ts << " end" << Qt::endl;
  ts << "definefont pop" << Qt::endl;
  ts << "/LucidaSansLatin" << Qt::endl;
  ts << " << /LucidaSans findfont {} forall >>" << Qt::endl;
  ts << " begin" << Qt::endl;
  ts << "  /Encoding ISOLatin1Encoding 256 array copy def currentdict"
     << Qt::endl;
  ts << " end" << Qt::endl;
  ts << "definefont pop" << Qt::endl;
  ts << Qt::endl;

  ts << "% Macros" << Qt::endl;
  ts << "/a {stroke} bind def" << Qt::endl;
  ts << "/b {sethsbcolor} bind def" << Qt::endl;

  double rectSize = EPS_SIZE / double(numberOfBins());
  ts << "/c {" << rectSize << " " << rectSize << " rectfill newpath} bind def"
     << Qt::endl;
  ts << "/d {closepath stroke} bind def" << Qt::endl;
  ts << "/e {newpath moveto} bind def" << Qt::endl;
  ts << "/f {lineto} bind def" << Qt::endl;
  ts << "/g {rlineto stroke} bind def" << Qt::endl;

  ts << EPS_DPCM << " " << EPS_DPCM << " scale" << Qt::endl;
}

void FingerprintPlot::writeEpsTitle(QTextStream &ts, QString title) {
  if (title.isEmpty()) {
    return;
  }

  QStack<EpsTitleState> stack;
  double fontSize = EPS_TITLE_FONT_SIZE;

  ts << "% Fingerprint title" << Qt::endl;
  ts << "0 0 0 setrgbcolor" << Qt::endl;
  ts << "/LucidaSansLatin-Italic findfont" << Qt::endl;
  ts << fontSize << " scalefont setfont" << Qt::endl;
  ts << EPS_OFFSETX + 0.3 << " " << EPS_OFFSETY + 0.3 << " e " << Qt::endl;

  ts << "(";
  for (int i = 0; i < title.length(); ++i) {
    QChar c = title[i];
    if (c == '_') { // Start new level of subscripting
      stack.push(SUBSCRIPT);
      fontSize = EPS_TITLE_FONT_SIZE * pow(0.6, stack.size());
      ts << ") show" << Qt::endl;
      ts << "/LucidaSansLatin-Italic findfont" << Qt::endl;
      ts << fontSize << " scalefont setfont" << Qt::endl;
      ts << "0 -" << fontSize * 0.3 << " rmoveto" << Qt::endl;
      ts << "(";
    } else if (c == '^') { // Start new level of superscripting
      stack.push(SUPERSCRIPT);
      fontSize = EPS_TITLE_FONT_SIZE * pow(0.6, stack.size());
      ts << ") show" << Qt::endl;
      ts << "/LucidaSansLatin-Italic findfont" << Qt::endl;
      ts << fontSize << " scalefont setfont" << Qt::endl;
      ts << "0 " << fontSize * 0.7 << " rmoveto" << Qt::endl;
      ts << "(";
    } else if (c == '}') { // revert to previous level of super-/sub-scripting
      ts << ") show" << Qt::endl;
      if (!stack.isEmpty()) {
        switch (stack.pop()) {
        case SUBSCRIPT:
          ts << "0 " << fontSize * 0.3 << " rmoveto" << Qt::endl;
          break;
        case SUPERSCRIPT:
          ts << "0 " << -fontSize * 0.7 << " rmoveto" << Qt::endl;
          break;
        }
      }
      fontSize = EPS_TITLE_FONT_SIZE * pow(0.6, stack.size());
      if (stack.size() == 0) {
        ts << "/LucidaSansLatin-Italic findfont" << Qt::endl;
        ts << fontSize << " scalefont setfont" << Qt::endl;
      }
      ts << "(";
    } else if (c == '{') { // ignore this
    } else {               // print character
      ts << c;
    }
  }
  ts << ") show" << Qt::endl;
}

void FingerprintPlot::writeEpsGridBoundary(QTextStream &ts) {
  double lowx = EPS_OFFSETX;
  double lowy = EPS_OFFSETY;
  double highx = lowx + EPS_SIZE;
  double highy = lowy + EPS_SIZE;

  ts << "% Grid boundary" << Qt::endl;
  ts << "0 0 0 setrgbcolor" << Qt::endl;
  ts << EPS_GRIDBOUNDARY_LINEWIDTH << " setlinewidth" << Qt::endl;
  ts << lowx << " " << lowy << " e" << Qt::endl;
  ts << highx << " " << lowy << " f" << Qt::endl;
  ts << highx << " " << highy << " f" << Qt::endl;
  ts << lowx << " " << highy << " f" << Qt::endl;
  ts << lowx << " " << lowy << " f d" << Qt::endl;
}

void FingerprintPlot::writeEpsAxisLabels(QTextStream &ts) {
  double scale_cm = EPS_SIZE / (plotMax() - plotMin());

  // Angstrom symbol
  ts << "% Angstrom symbol" << Qt::endl;
  ts << "0 0 0 setrgbcolor" << Qt::endl;
  ts << "/LucidaSansLatin findfont" << Qt::endl;
  ts << EPS_ANGSTROM_FONT_SIZE << " scalefont setfont" << Qt::endl;
  ts << EPS_OFFSETX - 0.5 << " " << EPS_OFFSETY - 0.5 << " e ((\305)) show"
     << Qt::endl;

  // x-axis Label
  double x = EPS_OFFSETX + EPS_SIZE - 0.15 * scale_cm;
  ts << "% x-axis label" << Qt::endl;
  ts << "0 0 0 setrgbcolor" << Qt::endl;
  ts << "/LucidaSansLatin-Italic findfont" << Qt::endl;
  ts << EPS_AXIS_LABEL_FONT_SIZE << " scalefont setfont" << Qt::endl;
  ts << x << " " << EPS_OFFSETY + 0.25 << " e ";
  ts << "(d) show" << Qt::endl;
  ts << "/LucidaSansLatin-Italic findfont" << Qt::endl;
  ts << EPS_AXIS_LABEL_FONT_SIZE * 0.6 << " scalefont setfont" << Qt::endl;
  ts << "0 -0.08 rmoveto (i) show" << Qt::endl;

  // y-axis Label
  double y = EPS_OFFSETY + EPS_SIZE - 0.15 * scale_cm;
  ts << "% y-axis label" << Qt::endl;
  ts << "0 0 0 setrgbcolor" << Qt::endl;
  ts << "/LucidaSansLatin-Italic findfont" << Qt::endl;
  ts << EPS_AXIS_LABEL_FONT_SIZE << " scalefont setfont" << Qt::endl;
  ts << EPS_OFFSETX + 0.15 << " " << y << " e ";
  ts << "(d) show" << Qt::endl;
  ts << "/LucidaSansLatin-Italic findfont" << Qt::endl;
  ts << EPS_AXIS_LABEL_FONT_SIZE * 0.6 << " scalefont setfont" << Qt::endl;
  ts << "0 -0.08 rmoveto (e) show" << Qt::endl;
}

void FingerprintPlot::writeEpsGridlinesAndScaleLabels(QTextStream &ts) {
  double scale_cm = EPS_SIZE / (plotMax() - plotMin());

  ts << "% Scale label font" << Qt::endl;
  ts << "/LucidaSans findfont" << Qt::endl;
  ts << EPS_AXIS_SCALE_FONT_SIZE << " scalefont setfont" << Qt::endl;
  ts << "0 0 0 setrgbcolor" << Qt::endl;
  ts << "% x gridlines and scale labels" << Qt::endl;
  ts << EPS_GRID_LINEWIDTH << " setlinewidth" << Qt::endl;

  for (int i = 1; i < numberOfGridlines(); ++i) {
    double x = i * gridSize() * scale_cm + EPS_OFFSETX;
    double y = i * gridSize() * scale_cm + EPS_OFFSETY;

    ts << x << " " << EPS_OFFSETY << " e 0 " << EPS_SIZE << " g"
       << Qt::endl; // x-axis gridlines
    ts << EPS_OFFSETX << " " << y << " e " << EPS_SIZE << " 0 g"
       << Qt::endl; // y-axis gridlines

    QString scaleLabel = QString::number(plotMin() + i * gridSize(), 'f', 1);
    ts << x - EPS_AXIS_SCALE_FONT_SIZE * scaleLabel.length() * 0.25 << " "
       << EPS_OFFSETY - 0.5 << " e ";
    ts << "(" << scaleLabel << ") show" << Qt::endl; // x-axis scale labels
    ts << EPS_OFFSETX - EPS_AXIS_SCALE_FONT_SIZE * scaleLabel.length() + 0.25
       << " " << y - 0.10 << " e ";
    ts << "(" << scaleLabel << ") show" << Qt::endl; // y-axis scale labels
  }
}

void FingerprintPlot::writeEpsBins(QTextStream &ts) {
  double binScale = EPS_SIZE / ((plotMax() - plotMin()) / binSize());

  QColor color;
  const double stdAreaForSaturatedColor = 0.001;
  const double enhancementFactor = 1.0;
  double maxValue =
      (stdAreaForSaturatedColor / enhancementFactor) * _surface->area();

  int min_i = smallestxBinInCurrentPlotRange();
  int min_j = smallestyBinInCurrentPlotRange();

  int numxBins = numxBinsInCurrentPlotRange();
  int numyBins = numyBinsInCurrentPlotRange();

  int xOffset = xOffsetForCurrentPlotRange();
  int yOffset = yOffsetForCurrentPlotRange();

  for (int i = 0; i < numxBins; ++i) {
    for (int j = 0; j < numyBins; ++j) {
      int iBin = i + min_i;
      int jBin = j + min_j;

      if (binUsed(iBin, jBin)) {
        if (binnedAreas(iBin, jBin) > 0.0) {
          color = ColorSchemer::color(m_colorScheme, binnedAreas(iBin, jBin),
                                      0.0, maxValue, true);
        } else {
          color = MASKED_BIN_COLOR;
        }
        double x = (xOffset + i) * binScale + EPS_OFFSETX;
        double y = (yOffset + j) * binScale + EPS_OFFSETY;
        ts << x << " " << y << " " << color.hue() / 359.0 << " "
           << color.saturation() / 255.0 << " " << color.value() / 255.0
           << " b c" << Qt::endl;
      }
    }
  }
}

void FingerprintPlot::writeEpsFooter(QTextStream &ts) {
  ts << "grestore" << Qt::endl;
  ts << "%%EOF" << Qt::endl;
}

////////////////////////////////////////////// SVG functions
/////////////////////////////////////////////////////////////

/*! Save the fingerprint as Scalable Vector Graphic to filename
 */
void FingerprintPlot::saveFingerprintAsSVG(QString filename) {
  Q_UNUSED(filename);
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
  return findLowerBound(_xPropertyMin, 0.0, binSize());
}

double FingerprintPlot::usedxPlotMax() {
  return findLowerBound(_xPropertyMax, usedxPlotMin(), binSize()) + binSize();
}

double FingerprintPlot::usedyPlotMin() {
  return findLowerBound(_yPropertyMin, 0.0, binSize());
}

double FingerprintPlot::usedyPlotMax() {
  return findLowerBound(_yPropertyMax, usedyPlotMin(), binSize()) + binSize();
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
  int graphWidthInPixels = PIXELS_PER_BIN * numberOfBins();
  QSize graphSize =
      QSize(graphWidthInPixels,
            graphWidthInPixels); // Make the graph height equal to the width

  return QRect(QPoint(0, 0), graphSize);
}

QSize FingerprintPlot::graphSize() const { return graphRect().size(); }

QSize FingerprintPlot::plotSize() const { return plotRect().size(); }

double FingerprintPlot::plotMin() const { return _plotMinForRange[_range]; }

double FingerprintPlot::plotMax() const { return _plotMaxForRange[_range]; }

double FingerprintPlot::binSize() const { return _plotBinSizeForRange[_range]; }

double FingerprintPlot::gridSize() const {
  return _plotGridSizeForRange[_range];
}

QSize FingerprintPlot::gridSeparation() const {
  return QSize(graphSize().width() / numberOfGridlines(),
               graphSize().height() / numberOfGridlines());
}
