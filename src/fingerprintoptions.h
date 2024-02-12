#pragma once
#include <QStringList>
#include <QToolButton>
#include <QWidget>

#include "fingerprintplot.h" // FingerprintFilterMode, PlotRange enums
#include "ui_fingerprintoptions.h"

const QString NONE_ELEMENT_LABEL = "All";

const FingerprintFilterMode defaultFilter = noFilter;

class FingerprintOptions : public QWidget, public Ui::FingerprintOptions {
  Q_OBJECT

public:
  FingerprintOptions(QWidget *parent = 0);
  void setElementList(QStringList);
  void resetOptions();

signals:
  void plotTypeChanged(PlotType);
  void plotRangeChanged(PlotRange);
  void filterChanged(FingerprintFilterMode, bool, bool, bool, QString, QString);
  void saveFingerprint(QString);
  void closeClicked();

private slots:
  void updatePlotType(int);
  void updatePlotRange(int);
  void updateFilterMode();
  void updateFilterSettings();
  void getFilenameAndSaveFingerprint();
  void updateSurfaceAreaProgressBar(double);
  void updateVisibilityOfFilterWidgets(int);

private:
  void init();
  QStringList filterOptions();
  void initConnections();
  void enableSignalsForWidgets(bool);
  void resetFilter();
  void updateVisibilityOfFilterWidgets(FingerprintFilterMode);
  void setVisibleElementFilteringWidgets(bool);
  void setVisibleSelectionFilteringWidgets(bool);
  void setVisibleCommonFilteringWidgets(bool);
  QColor getButtonColor(QToolButton *);
  void setButtonColor(QToolButton *, QColor);
  FingerprintFilterMode getFilterMode();
  void resetElementFilterOptions();
};
