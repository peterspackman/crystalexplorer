#pragma once
#include <QFont>
#include <QPixmap>
#include <QVector>
#include <QWidget>

#include "qeigen.h"
#include "meshinstance.h"
#include "colormap.h"

const QString plotTypeLabel = "dᵢ vs. dₑ"; // Used by the fingerprint options widget

// dPlot - plots property 1 on x-axis and property 2 on y-axis i.e. di vs de
// dnormPlot - plots property 3 on x-axis and property 4 on y-axis i.e. dnormi
// vs dnorme

struct FingerprintPlotSettings {
    QString label{"Standard"};
    double rangeMinimum{0.4};
    double rangeMaximum{2.6};
    double binSize{0.01};
    double gridSize{0.2};
    int pixelsPerBin{2};
};

enum class FingerprintPlotRange { Standard, Translated, Expanded};


inline FingerprintPlotSettings plotRangeSettings(FingerprintPlotRange r) {
    switch(r) {
        case FingerprintPlotRange::Standard: 
            return FingerprintPlotSettings{
                "Standard",
                0.4,
                2.6,
                0.01,
                0.2
            };
        case FingerprintPlotRange::Translated:
            return FingerprintPlotSettings{
                "Translated",
                0.8,
                3.0,
                0.01,
                0.2
            };
        case FingerprintPlotRange::Expanded:
            return FingerprintPlotSettings{
                "Expanded",
                0.4,
                3.0,
                0.01,
                0.2
            };
    }
}


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
enum class FingerprintFilterMode {None, Element};

inline const QStringList fingerprintFilterLabels{
    "None",
    "By Element",
};

inline const QVector<FingerprintFilterMode> requestableFilters{
    FingerprintFilterMode::None,
    FingerprintFilterMode::Element
};

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
  void setMesh(Mesh *);
  void updateFingerprintPlot();
  QVector<double> filteredAreas(QString, QStringList);

public slots:
  void updateFilter(FingerprintFilterMode, bool, bool, bool, QString, QString);
  void updatePlotRange(FingerprintPlotRange);
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
  void setRange(FingerprintPlotRange);
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

  FingerprintPlotRange m_range{FingerprintPlotRange::Standard};
  QPixmap plotPixmap;
  Mesh * m_mesh{nullptr};
  QString m_xAxisLabel{"di"};
  QString m_yAxisLabel{"de"};

  Eigen::VectorXd m_x, m_xFace;
  Eigen::VectorXd m_y, m_yFace;
  double m_xmin{0.0}, m_xmax{0.0};
  double m_ymin{0.0}, m_ymax{0.0};

  double m_xFaceMin{0.0};
  double m_xFaceMax{0.0};
  double m_yFaceMin{0.0};
  double m_yFaceMax{0.0};

  Eigen::MatrixXd binnedAreas;
  Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic> binUsed;
  double m_totalFilteredArea{0.0};


  FingerprintPlotSettings m_settings;

  // Filter options
  FingerprintFilterMode m_filterMode{FingerprintFilterMode::None};
  bool m_includeReciprocalContacts{false};
  bool m_filterInsideElement{false};
  bool m_filterOutsideElement{false};
  QString m_insideFilterElementSymbol{"H"};
  QString m_outsideFilterElementSymbol{"H"};

  ColorMapName m_colorScheme{ColorMapName::CE_rgb};
};
