#include "infoviewer.h"

#include <QtDebug>

InfoViewer::InfoViewer(QWidget *parent) : QDialog(parent) {
  setupUi(this);
  init();
  initConnections();
}

QTextDocument *InfoViewer::document(InfoType infoType) {
  QTextDocument *doc = nullptr;
  switch (infoType) {
  default:
    return nullptr;
  case InfoType::InteractionEnergy:
    doc = energiesTextEdit->document();
    break;
  }
  return doc;
}

void InfoViewer::setDocument(QTextDocument *document, InfoType infoType) {
  switch (infoType) {
  default:
    qWarning() << "Use setScene";
    break;
  case InfoType::InteractionEnergy:
    energiesTextEdit->setDocument(document);
    break;
  }
}

void InfoViewer::setScene(Scene *scene) {
  crystalInfoDocument->updateScene(scene);
  atomInfoDocument->updateScene(scene);
}

void InfoViewer::init() {
  Qt::WindowFlags flags = windowFlags();
  setWindowFlags(flags | Qt::WindowStaysOnTopHint);
  setModal(false);
}

void InfoViewer::initConnections() {
  connect(tabWidget, &QTabWidget::currentChanged, this,
          &InfoViewer::tabChanged);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &InfoViewer::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &InfoViewer::reject);
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
