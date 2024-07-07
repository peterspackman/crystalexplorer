#pragma once
#include <QDockWidget>
#include <QMainWindow>

#include "fingerprintoptions.h"
#include "fingerprintplot.h"
#include "scene.h"

typedef QMap<QString, QVector<double>> FingerprintBreakdown;

class FingerprintWindow : public QMainWindow {
  Q_OBJECT

public:
  FingerprintWindow(QWidget *parent = 0);
  ~FingerprintWindow();
  void updateFingerprint(int);
  FingerprintBreakdown fingerprintBreakdown(QStringList);

public slots:
  void show();
  void setScene(Scene *);
  void setMesh(Mesh *);
  void resetCrystal();

protected slots:
  void closeEvent(QCloseEvent *);
  void close();
  void resetSurfaceFeatures();

signals:
  void surfaceFeatureChanged();

private:
  void init();
  void initConnections();
  void createFingerprintPlot();
  void createOptionsDockWidget();
  void setTitle(Scene *);
  Scene *m_scene{nullptr};
  Mesh *m_mesh{nullptr};
  FingerprintPlot *fingerprintPlot;
  FingerprintOptions *fingerprintOptions;
  QDockWidget *optionsDockWidget;
};
