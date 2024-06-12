#include "fingerprintwindow.h"

FingerprintWindow::FingerprintWindow(QWidget *parent) : QMainWindow(parent) {
  init();
  initConnections();
}

void FingerprintWindow::init() {
  // A "Tool" is similar to a QMainWindow with the useful property that it stays
  // above
  // it's parent window (crystalx in this case). It also has a smaller title
  // bar.
  setWindowFlags(Qt::Tool);

  createFingerprintPlot();
  createOptionsDockWidget();
}

void FingerprintWindow::initConnections() {
  // Fingerprint option connections
  connect(fingerprintOptions, SIGNAL(plotTypeChanged(PlotType)),
          fingerprintPlot, SLOT(updatePlotType(PlotType)));
  connect(fingerprintOptions, SIGNAL(plotRangeChanged(PlotRange)),
          fingerprintPlot, SLOT(updatePlotRange(PlotRange)));
  connect(fingerprintOptions,
          SIGNAL(filterChanged(FingerprintFilterMode, bool, bool, bool, QString,
                               QString)),
          fingerprintPlot,
          SLOT(updateFilter(FingerprintFilterMode, bool, bool, bool, QString,
                            QString)));
  connect(fingerprintOptions, SIGNAL(saveFingerprint(QString)), fingerprintPlot,
          SLOT(saveFingerprint(QString)));
  connect(fingerprintOptions, SIGNAL(closeClicked()), this, SLOT(close()));

  // Fingerprint plot connections
  connect(fingerprintPlot, &FingerprintPlot::resetSurfaceFeatures, this,
          &FingerprintWindow::resetSurfaceFeatures);
  connect(fingerprintPlot, SIGNAL(surfaceFeatureChanged()), this,
          SIGNAL(surfaceFeatureChanged()));
  connect(fingerprintPlot, SIGNAL(surfaceAreaPercentageChanged(double)),
          fingerprintOptions, SLOT(updateSurfaceAreaProgressBar(double)));
}

FingerprintWindow::~FingerprintWindow() { delete fingerprintPlot; }

void FingerprintWindow::createFingerprintPlot() {
  fingerprintPlot = new FingerprintPlot(this);
  setCentralWidget(fingerprintPlot);
}

void FingerprintWindow::createOptionsDockWidget() {
  fingerprintOptions = new FingerprintOptions(this);

  optionsDockWidget = new QDockWidget(tr("Options"), this);
  optionsDockWidget->setWidget(fingerprintOptions);
  optionsDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
  addDockWidget(Qt::RightDockWidgetArea, optionsDockWidget);
}

void FingerprintWindow::show() {
  Mesh * mesh{nullptr};
  fingerprintPlot->setMesh(mesh);

  auto * structure = qobject_cast<ChemicalStructure *>(mesh->parent());
  if (structure != nullptr) {
    fingerprintOptions->setElementList(structure->uniqueElementSymbols());
  }
  fingerprintPlot->updateFingerprintPlot();
  QWidget::show();
}

void FingerprintWindow::setScene(Scene *scene) {
  m_scene = scene;
  setTitle(m_scene);
}

void FingerprintWindow::resetCrystal() { setScene(nullptr); }

void FingerprintWindow::resetSurfaceFeatures() {
  // TODO
  //m_scene->resetSurfaceFeatures();
  emit surfaceFeatureChanged();
}

void FingerprintWindow::close() {
  fingerprintOptions->resetOptions();
  resetSurfaceFeatures();
  hide();
}

void FingerprintWindow::closeEvent(QCloseEvent *event) {
  close();
  event->accept();
}

void FingerprintWindow::setTitle(Scene *scene) {
  if (scene == nullptr)
    return;
  QString name = scene->title();
  // TODO get proper surface description
  QString surfaceDescription = "Surface description";
  setWindowTitle(name + " [ " + surfaceDescription + " ]");
}

FingerprintBreakdown
FingerprintWindow::fingerprintBreakdown(QStringList elementSymbolList) {
  QMap<QString, QVector<double>> fingerprintBreakdown;

  foreach (QString insideElementSymbol, elementSymbolList) {
    auto filteredAreas =
        fingerprintPlot->filteredAreas(insideElementSymbol, elementSymbolList);
    fingerprintBreakdown[insideElementSymbol] = filteredAreas;
  }

  return fingerprintBreakdown;
}
