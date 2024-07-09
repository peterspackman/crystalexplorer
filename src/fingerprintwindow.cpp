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
  connect(fingerprintOptions, &FingerprintOptions::filterChanged,
          fingerprintPlot, &FingerprintPlot::updateFilter);
  connect(fingerprintOptions, &FingerprintOptions::saveFingerprint,
          fingerprintPlot, &FingerprintPlot::saveFingerprint);
  connect(fingerprintOptions, &FingerprintOptions::closeClicked,
          this, &FingerprintWindow::close);

  // Fingerprint plot connections
  connect(fingerprintPlot, &FingerprintPlot::surfaceFeatureChanged, this,
          &FingerprintWindow::surfaceFeatureChanged);
  connect(fingerprintPlot, &FingerprintPlot::surfaceAreaPercentageChanged,
          fingerprintOptions, &FingerprintOptions::updateSurfaceAreaProgressBar);
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
  fingerprintPlot->setMesh(m_mesh);

  auto *structure = qobject_cast<ChemicalStructure *>(m_mesh->parent());
  if (structure != nullptr) {
    fingerprintOptions->setElementList(structure->uniqueElementSymbols());
  }
  fingerprintPlot->updateFingerprintPlot();
  QWidget::show();
}

void FingerprintWindow::setMesh(Mesh * mesh) {
  m_mesh = mesh;
}

void FingerprintWindow::setScene(Scene *scene) {
  m_scene = scene;
  setTitle(m_scene);
}

void FingerprintWindow::resetCrystal() { setScene(nullptr); }

void FingerprintWindow::resetSurfaceFeatures() {
  // TODO
  // m_scene->resetSurfaceFeatures();
  emit surfaceFeatureChanged();
}

void FingerprintWindow::close() {
  fingerprintOptions->resetOptions();
  fingerprintPlot->resetSurfaceFeatures();
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
