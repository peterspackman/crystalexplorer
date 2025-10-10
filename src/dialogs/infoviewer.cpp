#include "infoviewer.h"
#include "colormap.h"

#include <QtDebug>

InfoViewer::InfoViewer(QWidget *parent) : QDialog(parent) {
  setupUi(this);
  init();
  initConnections();
}

void InfoViewer::setScene(Scene *scene) {
  crystalInfoDocument->updateScene(scene);
  atomInfoDocument->updateScene(scene);
  interactionsInfoDocument->updateScene(scene);
  surfaceInfoDocument->updateScene(scene);
  elasticTensorInfoDocument->updateScene(scene);
}

void InfoViewer::init() {
  Qt::WindowFlags flags = windowFlags();
  setWindowFlags(flags | Qt::WindowStaysOnTopHint);
  setModal(false);
  
  // Set up color maps
  auto colorMaps = availableColorMaps();
  energyColorComboBox->clear();
  energyColorComboBox->insertItems(0, colorMaps);
  QString currentColorMap =
      settings::readSetting(settings::keys::ENERGY_COLOR_SCHEME).toString();
  energyColorComboBox->setCurrentIndex(colorMaps.indexOf(currentColorMap));
}

void InfoViewer::initConnections() {
  connect(tabWidget, &QTabWidget::currentChanged, this,
          &InfoViewer::tabChanged);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &InfoViewer::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &InfoViewer::reject);
  connect(energyPrecisionSpinBox, &QSpinBox::valueChanged, this,
          &InfoViewer::updateInteractionDisplaySettings);
  connect(energyColorComboBox, &QComboBox::currentIndexChanged, this,
          &InfoViewer::updateEnergyColorSettings);
  connect(distancePrecisionSpinBox, &QSpinBox::valueChanged, this,
          &InfoViewer::updateInteractionDisplaySettings);

  // Forward elastic tensor request signal
  connect(interactionsInfoDocument, &InteractionInfoDocument::elasticTensorRequested,
          this, &InfoViewer::elasticTensorRequested);
}

void InfoViewer::accept() {
  emit infoViewerClosed();
  QDialog::accept();
}

void InfoViewer::reject() {
  emit infoViewerClosed();
  QDialog::reject();
}

void InfoViewer::show() {
  tabChanged(tabWidget->currentIndex()); // force refresh of information when
                                         // showing InfoViewer
  QDialog::show();
}

void InfoViewer::tabChanged(int tabIndex) {
  Q_UNUSED(tabIndex);
  emit tabChangedTo(currentTab());
}

void InfoViewer::setTab(InfoType infoType) {
  switch (infoType) {
  case InfoType::Crystal:
    tabWidget->setCurrentWidget(crystalTab);
    break;
  case InfoType::Atoms:
    tabWidget->setCurrentWidget(atomsTab);
    break;
  case InfoType::Surface:
    tabWidget->setCurrentWidget(surfaceTab);
    break;
  case InfoType::InteractionEnergy:
    tabWidget->setCurrentWidget(energiesTab);
    break;
  case InfoType::ElasticTensor:
    tabWidget->setCurrentWidget(elasticTensorTab);
    break;
  }
}

void InfoViewer::updateCurrentTab() {
  emit tabChanged(tabWidget->currentIndex());
}

InfoType InfoViewer::currentTab() {
  InfoType result;
  switch (tabWidget->currentIndex()) {
  case 0:
    result = InfoType::Crystal;
    break;
  case 1:
    result = InfoType::Atoms;
    break;
  case 2:
    result = InfoType::Surface;
    break;
  case 3:
    result = InfoType::InteractionEnergy;
    break;
  case 4:
    result = InfoType::ElasticTensor;
    break;
  }
  return result;
}

void InfoViewer::updateInfoViewerForCrystalChange() {
  if (isVisible()) {
    updateCurrentTab();
  }
}

void InfoViewer::updateInfoViewerForSurfaceChange() {
  if (isVisible()) {
    if (currentTab() == InfoType::Surface) {
      updateCurrentTab();
    }
  }
}

void InfoViewer::updateInteractionDisplaySettings() {
  InteractionInfoSettings settings;
  settings.distancePrecision = distancePrecisionSpinBox->value();
  settings.energyPrecision = energyPrecisionSpinBox->value();
  interactionsInfoDocument->updateSettings(settings);
}

void InfoViewer::updateEnergyColorSettings() {
  auto colorScheme = energyColorComboBox->currentText();
  settings::writeSetting(settings::keys::ENERGY_COLOR_SCHEME, colorScheme);
  interactionsInfoDocument->forceUpdate();
  emit energyColorSchemeChanged();
}

void InfoViewer::enableExperimentalFeatures(bool enable) {
  interactionsInfoDocument->enableExperimentalFeatures(enable);
}
