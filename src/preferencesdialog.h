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
  void populateExecutablesFromPath(bool override=false);

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
  void screenGammaChanged();
  void materialChanged();
  void lightSettingsChanged();
  void textSettingsChanged();
  void targetFramerateChanged(int fps);

private slots:
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
  void setJmolColors(bool);
  void setScreenGamma(int);
  void setMaterialFactors();
  void setLightFixedToCamera(bool);
  void setShowLightPositions(bool);
  void setGLDepthTestEnabled(bool);
  void setUseImpostorRendering(bool);
  void setTargetFramerate(int);
  void setLightColors();
  void setLightIntensities(double);
  void setTextSliders(int);
  void restoreDefaultLightingSettings();
  void onTextFontFamilyChanged(const QFont &font);
  void onTextFontSizeChanged(int);

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

  QColor m_currentSelectionColor{Qt::yellow};
  QColor m_currentBackgroundColor{Qt::white};
  QColor m_currentNonePropertyColor{Qt::gray};
  QColor m_currentFaceHighlightColor{Qt::red};
  QColor m_currentTextLabelOutlineColor{Qt::white};
  QColor m_currentTextLabelColor{Qt::black};
};
