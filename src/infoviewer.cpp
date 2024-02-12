#include "infoviewer.h"

#include <QtDebug>

InfoViewer::InfoViewer(QWidget *parent) : QDialog(parent) {
  setupUi(this);
  init();
  initConnections();
}

void InfoViewer::init() {
  Qt::WindowFlags flags = windowFlags();
  setWindowFlags(flags | Qt::WindowStaysOnTopHint);
  setModal(false);
}

void InfoViewer::initConnections() {
  connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
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

QTextDocument *InfoViewer::document(InfoType infoType) {
  QTextDocument *doc;

  switch (infoType) {
  case GeneralCrystalInfo:
    doc = crystalTextEdit->document();
    break;
  case AtomCoordinateInfo:
    doc = atomsTextEdit->document();
    break;
  case CurrentSurfaceInfo:
    doc = surfaceTextEdit->document();
    break;
  case InteractionEnergyInfo:
    doc = energiesTextEdit->document();
    break;
  }
  return doc;
}

void InfoViewer::setDocument(QTextDocument *document, InfoType infoType) {
  switch (infoType) {
  case GeneralCrystalInfo:
    crystalTextEdit->setDocument(document);
    break;
  case AtomCoordinateInfo:
    atomsTextEdit->setDocument(document);
    break;
  case CurrentSurfaceInfo:
    surfaceTextEdit->setDocument(document);
    break;
  case InteractionEnergyInfo:
    energiesTextEdit->setDocument(document);
    break;
  }
}

void InfoViewer::setTab(InfoType infoType) {
  switch (infoType) {
  case GeneralCrystalInfo:
    tabWidget->setCurrentWidget(crystalTab);
    break;
  case AtomCoordinateInfo:
    tabWidget->setCurrentWidget(atomsTab);
    break;
  case CurrentSurfaceInfo:
    tabWidget->setCurrentWidget(surfaceTab);
    break;
  case InteractionEnergyInfo:
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
    result = GeneralCrystalInfo;
    break;
  case 1:
    result = AtomCoordinateInfo;
    break;
  case 2:
    result = CurrentSurfaceInfo;
    break;
  case 3:
    result = InteractionEnergyInfo;
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
    if (currentTab() == CurrentSurfaceInfo) {
      updateCurrentTab();
    }
  }
}
