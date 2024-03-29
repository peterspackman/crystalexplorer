#pragma once
#include <QDialog>
#include <QStandardItemModel>

#include "periodictabledialog.h"
#include "ui_preferencesdialog.h"

const int GLOBAL_PERSPECTIVE_LEVEL = 30;

class PreferencesDialog : public QDialog, public Ui::PreferencesDialog {
  Q_OBJECT

public:
  PreferencesDialog(QWidget *parent = 0);

public slots:
  void show();
  void updateGlwindowBackgroundColor(QColor);

signals:
  void resetElementData();
  void redrawCrystalForPreferencesChange();
  void faceHighlightColorChanged();
  void setOpenglProjection(bool, float);
  void glwindowBackgroundColorChanged(QColor);
  void glDepthTestEnabledChanged(bool);
  void showElementElementCloseContactsToggled(bool);
  void redrawCloseContactsForPreferencesChange();
  void nonePropertyColorChanged();
  void selectionColorChanged();
  void energyColorSchemeChanged();
  void screenGammaChanged();
  void materialChanged();
  void lightSettingsChanged();
  void textSettingsChanged();

private slots:
  void getTontoPath();
  void getValueForExternalProgramSetting(QStandardItem *item);
  void handleExternalProgramSettingsDoubleClick(const QModelIndex &index);
  void restoreDefaultExternalProgramSetting();

  void accept();
  void editElements();
  void resetAllElements();
  void setFaceHighlightColor();
  void setTextLabelColor();
  void setTextLabelOutlineColor();
  void setEnergyFrameworkPositiveColor();
  void setNonePropertyColor();
  void setSelectionColor();
  void setBondThickness(int);
  void setContactLineThickness(int);
  void setViewOrthographic();
  void setViewPerspective();
  void updateSliderPerspective();
  void contextualGlwindowBackgroundColor();
  void restoreExpertSettings();
  void setInternalBasisset();
  void setEnergiesTableDecimalPlaces(int);
  void setEnergiesColorScheme(int);
  void setJmolColors(bool);
  void setScreenGamma(int);
  void setMaterialFactors();
  void setLightFixedToCamera(bool);
  void setShowLightPositions(bool);
  void setGLDepthTestEnabled(bool);
  void setLightColors();
  void setLightIntensities(double);
  void setTextSliders(int);
  void restoreDefaultLightingSettings();

private:
  void updateLightPositions();
  void init();
  void initConnections();
  void loadExternalProgramSettings();
  void updateExternalProgramSettings();
  void updateDialogFromSettings();
  void updateLightsFromSettings();
  void setButtonColor(QAbstractButton *, QColor);
  QColor getButtonColor(QAbstractButton *);
  void updateSettingsFromDialog();
  void setProjection(bool);
  void enablePerspectiveSlider(bool);
  bool _updateDialogFromSettingsDone;
  PeriodicTableDialog *m_periodicTableDialog{nullptr};
  QMap<QString, QString> m_lightColorKeys;
  QMap<QString, QString> m_lightIntensityKeys;
  QMap<QString, QString> m_textSliderKeys;
  QMap<QString, QStringList> m_externalProgramSettingsKeys;
  QStandardItemModel *m_externalProgramSettingsModel{nullptr};
};
