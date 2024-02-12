#pragma once
#include <QFont>
#include <QPixmap>
#include <QVector>
#include <QWidget>

#include "deprecatedcrystal.h"
#include "qeigen.h"
#include "surface.h"

enum PlotType { dPlot, dnormPlot };
// const QStringList plotTypeLabels = QStringList() << "di vs de" << "dnormi vs
// dnorme"; // Used by the fingerprint options widget
const QStringList plotTypeLabels =
    QStringList() << "dᵢ vs. dₑ"; // Used by the fingerprint options widget

// dPlot - plots property 1 on x-axis and property 2 on y-axis i.e. di vs de
// dnormPlot - plots property 3 on x-axis and property 4 on y-axis i.e. dnormi
// vs dnorme
const QVector<int> xPropertyIndices = QVector<int>() << 1 << 3;
const QVector<int> yPropertyIndices = QVector<int>() << 2 << 4;

enum PlotRange { standardPlot, translatedPlot, expandedPlot };
const QStringList plotRangeLabels =
    QStringList() << "Standard"
                  << "Translated"
                  << "Expanded"; // Used by the fingerprint options widget

// Parameters for di/de plot
const QVector<double> dPlotMinForRange = QVector<double>() << 0.4 << 0.8 << 0.4;
const QVector<double> dPlotMaxForRange = QVector<double>() << 2.6 << 3.0 << 3.0;
const QVector<double> dPlotBinSizeForRange = QVector<double>()
                                             << 0.01 << 0.01 << 0.01;
const QVector<double> dPlotGridSizeForRange = QVector<double>()
                                              << 0.2 << 0.2 << 0.2;

// Parameters for dnormi/dnorme plot
const QVector<double> dnormPlotMinForRange = QVector<double>()
                                             << -0.4 << -0.2 << -0.4;
const QVector<double> dnormPlotMaxForRange = QVector<double>()
                                             << 0.8 << 1.0 << 1.0;
const QVector<double> dnormPlotBinSizeForRange = QVector<double>()
                                                 << 0.005 << 0.005 << 0.005;
const QVector<double> dnormPlotGridSizeForRange = QVector<double>()
                                                  << 0.1 << 0.1 << 0.1;

const PlotType DEFAULT_PLOT_TYPE = dPlot;
const PlotRange DEFAULT_PLOT_RANGE = standardPlot;

const int PIXELS_PER_BIN = 2;

const int UNDEFINED_BIN_INDEX = -1;

// Colors
const Qt::GlobalColor PLOT_BACKGROUND_COLOR = Qt::white;
const Qt::GlobalColor GRID_LINES_COLOR = Qt::gray;
const Qt::GlobalColor GRID_BOUNDARY_COLOR = Qt::black;
const Qt::GlobalColor AXIS_SCALE_TEXT_COLOR = Qt::black;
const Qt::GlobalColor AXIS_LABEL_TEXT_COLOR = Qt::black;
const Qt::GlobalColor TITLE_TEXT_COLOR = Qt::black;
const QColor MASKED_BIN_COLOR = QColor(180, 180, 180);
const Qt::GlobalColor MESSAGE_COLOR = Qt::red;

// Fonts
const int AXIS_SCALE_FONT_SIZE = 12;
const int AXIS_LABEL_FONT_SIZE = 12;
#if defined(Q_OS_WIN)
    inline const char * fingerprint_sans_font = "Verdana";
    inline const char * fingerprint_serif_font = "Times New Roman";
#elif defined(Q_OS_MACOS)
    inline const char * fingerprint_sans_font = "Helvetica";
    inline const char * fingerprint_serif_font = "Georgia";
#elif defined(Q_OS_LINUX)
    inline const char * fingerprint_sans_font = "Ubuntu";
    inline const char * fingerprint_serif_font = "Times New Roman";
#endif

const QFont TITLE_FONT = QFont(fingerprint_sans_font, 14, QFont::Bold, true);
const QFont AXIS_SCALE_FONT =
    QFont(fingerprint_sans_font, AXIS_SCALE_FONT_SIZE, QFont::Normal, false);
const QFont AXIS_LABEL_FONT =
    QFont(fingerprint_sans_font, AXIS_LABEL_FONT_SIZE, QFont::Bold, true);
const QFont AXIS_LABEL_FONT_SUBSCRIPT =
    QFont(fingerprint_sans_font, AXIS_LABEL_FONT_SIZE, QFont::Bold, true);
const QFont UNITS_FONT = QFont(fingerprint_serif_font, 12, QFont::Normal, false);
const QFont MESSAGE_FONT = QFont(fingerprint_sans_font, 30, QFont::Normal, false);

// Pen
const int PEN_WIDTH = 1;
const int MESSAGE_PEN_WIDTH = 2;

// Positioning
const int AXIS_SCALE_OFFSET = 30;
const int AXIS_SCALE_TEXT_OFFSET = 2;

// Fingerprint filtering
enum FingerprintFilterMode { noFilter, elementFilter, selectionFilter };
const QStringList fingerprintFilterLabels = QStringList()
                                            << "None"
                                            << "By Element"
                                            << "By Selection [Graphics Window]";
const QVector<FingerprintFilterMode> requestableFilters =
    QVector<FingerprintFilterMode>()
    << noFilter << elementFilter; // << selectionFilter;

const QString NO_FINGERPRINT_MESSAGE = "Fingerprint Plot Unavailable";

// Encapsulated Postscript parameters
const double EPS_SIZE = 11.0; // Overall size of "graph" on paper in cm
const double EPS_DPI = 300;   // dots per inch
const double EPS_DPCM = EPS_DPI / 2.54; // dots per cm
const double EPS_MARGIN_LEFT = 1.0;
const double EPS_MARGIN_RIGHT = 0.5;
const double EPS_MARGIN_TOP = 0.5;
const double EPS_MARGIN_BOTTOM = 1.0;
const double EPS_OFFSETX =
    4; // The choice of these offsets is *completely* arbitrary
const double EPS_OFFSETY = 2;
const double EPS_GRIDBOUNDARY_LINEWIDTH = 0.02;
const double EPS_GRID_LINEWIDTH = 0.0025;
const double EPS_AXIS_SCALE_FONT_SIZE = 0.4;
const double EPS_ANGSTROM_FONT_SIZE = EPS_AXIS_SCALE_FONT_SIZE;
const double EPS_AXIS_LABEL_FONT_SIZE = 0.8;
const double EPS_TITLE_FONT_SIZE = 0.6;
enum EpsTitleState { SUBSCRIPT, SUPERSCRIPT };

//      <---------- PW -------------->
//     |------------------------------|
//  ^  |            TM     "Plot"     |
//  |  |                              |
//  |  |    |--------------------|    |
//     |    |      ^             |    |
//  PH | LM |      |             | RM |
//  |  |    |      |             |    |
//  |  |    | <------- GW -----> |    |
//  |  |    |      |             |    |
//  |  |    |     GH             |    |
//  |  |    |      |    "Graph"  |    |
//  |  |    |      |             |    |
//  |  |    |      v             |    |
//  |  |    |--------------------|    |
//  |  |             BM               |
//  v  |------------------------------|
//
//
// Plot = Whole area of the plot (i.e. Graph + four margins).
// plotSize() = PW x PH. See also plotRect()
//
// Graph = area where we draw the grid and plot the fingerprint bins
// graphSize() = GW x GH. See also graphRect()
//
// LM = left margin given by leftMargin()
// RM = right margin given by rightMargin()
// TM = top margin given by topMargin()
// BM = bottom margin given by bottomMargin()

class FingerprintPlot : public QWidget {
  Q_OBJECT

public:
  FingerprintPlot(QWidget *parent = 0);
  void setCrystalAndSurface(DeprecatedCrystal *, Surface *);
  void updateFingerprintPlot();
  QVector<double> filteredAreas(QString, QStringList);

public slots:
  void updateFilter(FingerprintFilterMode, bool, bool, bool, QString, QString);
  void updatePlotType(PlotType);
  void updatePlotRange(PlotRange);
  void saveFingerprint(QString);

signals:
  void surfaceAreaPercentageChanged(double);
  void surfaceFeatureChanged();
  void resetSurfaceFeatures();

protected:
  void paintEvent(QPaintEvent *);
  void mousePressEvent(QMouseEvent *);

private:
  void init();
  void resetFilter();
  void setFilter(FingerprintFilterMode, bool, bool, bool, QString, QString);
  void setPlotType(PlotType);
  void setRange(PlotRange);
  void setPropertiesToPlot();
  void setAxisLabels();
  void initBinnedAreas();
  void initBinnedFilterFlags();
  void drawEmptyFingerprint();
  void drawNoFingerprintMessage(QPainter *);
  void drawFingerprint();
  double calculateBinnedAreasNoFilter();
  double calculateBinnedAreasWithFilter();
  void calculateBinnedAreas();
  int binIndex(double, double, double, int);
  int xBinIndex(double);
  int yBinIndex(double);
  int tolerant_xBinIndex(double);
  int tolerant_yBinIndex(double);
  bool includeArea(int);
  bool includeAreaFilteredByElement(int);
  bool includeAreaFilteredBySelection(int);
  void drawGrid(QPainter *);
  void drawGridlines(QPainter *painter);
  void drawScaleLabels(QPainter *painter);
  void drawAxisLabels(QPainter *painter);
  void drawGridBoundary(QPainter *);
  void drawBins(QPainter *);

  QPoint t(int, int);
  QPoint tinv(int, int);

  QPair<int, int> binIndicesAtMousePosition(QPoint);
  QPair<int, int> binIndicesAtGraphPos(QPoint);

  void highlightFacesWithPropertyValues(QPair<int, int>);

  // Postscript functions
  void saveFingerprintAsEps(QString, QString);
  void writeEpsHeader(QTextStream &, QString);
  void writeEpsTitle(QTextStream &, QString);
  void writeEpsGridBoundary(QTextStream &);
  void writeEpsAxisLabels(QTextStream &);
  void writeEpsGridlinesAndScaleLabels(QTextStream &);
  void writeEpsBins(QTextStream &);
  void writeEpsFooter(QTextStream &);

  void saveFingerprintAsSVG(QString);
  void saveFingerprintAsCSV(QString);
  void saveFingerprintAsPNG(QString);

  void outputFingerprintAsJSON();
  void outputFingerprintAsTable();

  // Determining actual plot size
  double findLowerBound(double, double, double);
  double usedxPlotMin();
  double usedxPlotMax();
  double usedyPlotMin();
  double usedyPlotMax();
  int numUsedxBins();
  int numUsedyBins();
  int xOffset();
  int yOffset();
  int smallestxBinInCurrentPlotRange();
  int smallestyBinInCurrentPlotRange();
  int numxBinsInCurrentPlotRange();
  int numyBinsInCurrentPlotRange();
  int xOffsetForCurrentPlotRange();
  int yOffsetForCurrentPlotRange();

  // Determining plot size
  int numberOfGridlines() const;
  int numberOfBins() const;
  double plotMin() const;
  double plotMax() const;
  double binSize() const;
  double gridSize() const;
  int leftMargin() const;
  int rightMargin() const;
  int topMargin() const;
  int bottomMargin() const;
  QRect plotRect() const;
  QSize plotSize() const;
  QRect graphRect() const;
  QSize graphSize() const;
  QSize gridSeparation() const;

  PlotType _plotType;
  PlotRange _range;
  QPixmap plotPixmap;
  DeprecatedCrystal *_crystal;
  Surface *_surface;
  int _xPropertyIndex;
  int _yPropertyIndex;
  QString _xAxisLabel;
  QString _yAxisLabel;
  QVector<double> _xPropertyAtFace;
  QVector<double> _yPropertyAtFace;
  double _xPropertyMin;
  double _xPropertyMax;
  double _yPropertyMin;
  double _yPropertyMax;
  MatrixXq binnedAreas;
  Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic> binUsed;
  double _totalFilteredArea;

  // Plot parameters; these depend on PlotType
  QVector<double> _plotMinForRange;
  QVector<double> _plotMaxForRange;
  QVector<double> _plotBinSizeForRange;
  QVector<double> _plotGridSizeForRange;

  // Filter options
  FingerprintFilterMode _filterMode;
  bool _includeReciprocalContacts;
  bool _filterInsideElement;
  bool _filterOutsideElement;
  QString _insideFilterElementSymbol;
  QString _outsideFilterElementSymbol;

  ColorScheme m_colorScheme{ColorScheme::RedGreenBlue};
  bool _plotTypeChanged;
};
