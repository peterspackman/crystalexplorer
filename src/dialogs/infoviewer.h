#pragma once
#include "atominfodocument.h"
#include "ui_infoviewer.h"
#include <QDialog>

enum class InfoType {
  Crystal,
  Atoms,
  Surface,
  InteractionEnergy,
  ElasticTensor,
};

class InfoViewer : public QDialog, public Ui::InfoViewer {
  Q_OBJECT
public:
  InfoViewer(QWidget *parent = nullptr);
  void show();
  void setScene(Scene *);
  void setTab(InfoType);
  void updateCurrentTab();
  InfoType currentTab();

public slots:
  void updateInfoViewerForCrystalChange();
  void updateInfoViewerForSurfaceChange();
  void updateInteractionDisplaySettings();
  void updateEnergyColorSettings();

signals:
  void infoViewerClosed();
  void energyColorSchemeChanged();
  void tabChangedTo(InfoType);

private slots:
  void tabChanged(int index);
  void accept() override;
  void reject() override;

private:
  void init();
  void initConnections();
  AtomInfoDocument *m_atomInfoDocument{nullptr};
};
