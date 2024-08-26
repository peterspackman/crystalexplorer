#pragma once
#include "ui_infoviewer.h"
#include <QDialog>

enum InfoType {
  GeneralCrystalInfo,
  AtomCoordinateInfo,
  InteractionEnergyInfo,
  CurrentSurfaceInfo
};

class InfoViewer : public QDialog, public Ui::InfoViewer {
  Q_OBJECT

public:
  InfoViewer(QWidget *);
  void show();
  QTextDocument *document(InfoType);
  void setDocument(QTextDocument *, InfoType);
  void setTab(InfoType);
  void updateCurrentTab();
  InfoType currentTab();

public slots:
  void updateInfoViewerForCrystalChange();
  void updateInfoViewerForSurfaceChange();

signals:
  void infoViewerClosed();
  void tabChangedTo(InfoType);

private slots:
  void tabChanged(int index);
  void accept();
  void reject();

private:
  void init();
  void initConnections();
};
