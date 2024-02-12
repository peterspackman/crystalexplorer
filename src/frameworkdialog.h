#pragma once
#include <QAbstractButton>
#include <QDialog>

#include "frameworkdescription.h"

// This means the units displayed on the spinner
// are Á per MJ per mol
const double scaleRescale = 10000.0;

namespace Ui {
class FrameworkDialog;
}

class FrameworkDialog : public QDialog {
  Q_OBJECT

public:
  explicit FrameworkDialog(QWidget *parent = 0);
  ~FrameworkDialog();

  void setCurrentFramework(FrameworkType);
  void setEnergyTheories(const QVector<EnergyTheory> &);

public slots:
  void reject();
  void accept();

private slots:
  void optionsButtonClicked();
  void scaleSpinboxChanged(double);
  void cutoffSpinboxChanged(double);
  void energyTheoriesComboboxChanged(int);

signals:
  void cycleFrameworkRequested(bool);
  void frameworkDialogClosing();
  void frameworkDialogCutoffChanged();
  void frameworkDialogScaleChanged();
  void energyTheoryChanged(EnergyTheory);

private:
  void init();
  void initConnections();
  void cleanupForClosing();

  void setCurrentFrameworkLabel(QString, QColor);
  void enableScaleOptions(bool);

  void updateCutoffSpinboxFromSettings();
  void updateScaleSpinboxFromSettings();
  void updateOptions(bool);
  void updateOptionsVisibility(bool);
  void updateOptionsButtonText(bool);
  void updateEnergyTheories();

  Ui::FrameworkDialog *ui;

  bool m_showOptions{false};
  FrameworkType m_currentFramework;
  QVector<EnergyTheory> m_energyTheories;
};
