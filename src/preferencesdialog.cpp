#include <QColorDialog>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QtDebug>

#include "colormap.h"
#include "elementeditor.h"
#include "globals.h"
#include "preferencesdialog.h"
#include "settings.h"

#include "exefileutilities.h"

enum PreferencesRoles { PreferencesKeyRole = Qt::UserRole + 1 };

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent),
      m_externalProgramSettingsModel(new QStandardItemModel(this)) {
  setupUi(this);

  m_externalProgramSettingsModel->setHorizontalHeaderLabels(QStringList()
                                                            << "Program"
                                                            << "Setting"
                                                            << "Value");
  externalProgramPaths->setModel(m_externalProgramSettingsModel);

  QStringList programGroups{
      settings::keys::GAUSSIAN_GROUP, settings::keys::NWCHEM_GROUP,
      settings::keys::GAUSSIAN_GROUP, settings::keys::PSI4_GROUP,
      settings::keys::OCC_GROUP,      settings::keys::ORCA_GROUP,
      settings::keys::XTB_GROUP};
  for (const auto &group : std::as_const(programGroups)) {
    m_externalProgramSettingsKeys.insert(group,
                                         settings::settingsFromGroup(group));
  }
  populateExecutablesFromPath(false);

  m_lightColorKeys.insert(lightAmbientColour->objectName(),
                          settings::keys::LIGHT_AMBIENT);
  m_lightColorKeys.insert(light1SpecularColour->objectName(),
                          settings::keys::LIGHT_SPECULAR_1);
  m_lightColorKeys.insert(light2SpecularColour->objectName(),
                          settings::keys::LIGHT_SPECULAR_2);
  m_lightColorKeys.insert(light3SpecularColour->objectName(),
                          settings::keys::LIGHT_SPECULAR_3);
  m_lightColorKeys.insert(light4SpecularColour->objectName(),
                          settings::keys::LIGHT_SPECULAR_4);

  m_lightIntensityKeys.insert(lightAmbientSpinBox->objectName(),
                              settings::keys::LIGHT_AMBIENT_INTENSITY);
  m_lightIntensityKeys.insert(lightIntensity1SpinBox->objectName(),
                              settings::keys::LIGHT_INTENSITY_1);
  m_lightIntensityKeys.insert(lightIntensity2SpinBox->objectName(),
                              settings::keys::LIGHT_INTENSITY_2);
  m_lightIntensityKeys.insert(lightIntensity3SpinBox->objectName(),
                              settings::keys::LIGHT_INTENSITY_3);
  m_lightIntensityKeys.insert(lightIntensity4SpinBox->objectName(),
                              settings::keys::LIGHT_INTENSITY_4);

  m_lightIntensityKeys.insert(lightAttenuationRangeLowerSpinBox->objectName(),
                              settings::keys::LIGHT_ATTENUATION_MINIMUM);
  m_lightIntensityKeys.insert(lightAttenuationRangeUpperSpinBox->objectName(),
                              settings::keys::LIGHT_ATTENUATION_MAXIMUM);

  m_textSliderKeys.insert(textOutlineWidthSlider->objectName(),
                          settings::keys::TEXT_OUTLINE);
  m_textSliderKeys.insert(textBufferWidthSlider->objectName(),
                          settings::keys::TEXT_BUFFER);
  m_textSliderKeys.insert(textSmoothingWidthSlider->objectName(),
                          settings::keys::TEXT_SMOOTHING);
  init();
  initConnections();
}

void PreferencesDialog::init() {
  enablePerspectiveSlider(buttonPerspective->isChecked());

  configurationFilePath->setText(settings::filePath());
  tabWidget->setCurrentIndex(0);
  jmolColorCheckBox->setChecked(
      settings::readSetting(settings::keys::USE_JMOL_COLORS).toBool());
  gammaSlider->setValue(static_cast<int>(
      settings::readSetting(settings::keys::SCREEN_GAMMA).toFloat() * 100));
  metallicSpinBox->setValue(static_cast<double>(
      settings::readSetting(settings::keys::MATERIAL_METALLIC).toFloat()));
  roughnessSpinBox->setValue(static_cast<double>(
      settings::readSetting(settings::keys::MATERIAL_ROUGHNESS).toFloat()));
  lightCameraFixCheckBox->setChecked(
      settings::readSetting(settings::keys::LIGHT_TRACKS_CAMERA).toBool());
  showLightPositionsCheckBox->setChecked(
      settings::readSetting(settings::keys::SHOW_LIGHT_POSITIONS).toBool());

  ColorMapName currentScheme = colorMapFromString(
      settings::readSetting(settings::keys::ENERGY_COLOR_SCHEME).toString());
  int idx = 0;
  for (const auto &scheme : availableColorMaps()) {
    energyColorSchemeComboBox->addItem(colorMapToString(scheme));
    if (scheme == currentScheme)
      energyColorSchemeComboBox->setCurrentIndex(idx);
    idx++;
  }
  textFontComboBox->setCurrentFont(
      QFont(settings::readSetting(settings::keys::TEXT_FONT_FAMILY).toString(),
            settings::readSetting(settings::keys::TEXT_FONT_SIZE).toInt()));
}

void PreferencesDialog::initConnections() {
  // General preferences

  connect(editElementsButton, &QPushButton::clicked, this,
          &PreferencesDialog::editElements);
  connect(jmolColorCheckBox, &QCheckBox::toggled, this,
          &PreferencesDialog::setJmolColors);
  connect(resetAllElementsButton, &QPushButton::clicked, this,
          &PreferencesDialog::resetAllElements);

  // External program preferences

  externalProgramPaths->setEditTriggers(QAbstractItemView::SelectedClicked |
                                        QAbstractItemView::AnyKeyPressed);
  connect(restoreProgramSettingButton, &QPushButton::clicked, this,
          &PreferencesDialog::restoreDefaultExternalProgramSetting);
  connect(externalProgramPaths, &QTreeView::doubleClicked, this,
          &PreferencesDialog::handleExternalProgramSettingsDoubleClick);

  // Display preferences
  connect(backgroundColorButton, &QPushButton::clicked, this,
          &PreferencesDialog::contextualGlwindowBackgroundColor);
  connect(faceHighlightColorButton, &QPushButton::clicked, this,
          &PreferencesDialog::setFaceHighlightColor);
  connect(textColorButton, &QPushButton::clicked, this,
          &PreferencesDialog::setTextLabelColor);
  connect(textOutlineColorButton, &QPushButton::clicked, this,
          &PreferencesDialog::setTextLabelOutlineColor);
  connect(energyFrameworkPositiveColorButton, &QPushButton::clicked, this,
          &PreferencesDialog::setEnergyFrameworkPositiveColor);
  connect(nonePropertyColorButton, &QPushButton::clicked, this,
          &PreferencesDialog::setNonePropertyColor);
  connect(selectionColorButton, &QPushButton::clicked, this,
          &PreferencesDialog::setSelectionColor);

  connect(bondThicknessSlider, &QSlider::valueChanged, this,
          &PreferencesDialog::setBondThickness);
  connect(contactLineThicknessSlider, &QSlider::valueChanged, this,
          &PreferencesDialog::setContactLineThickness);

  connect(buttonPerspective, &QRadioButton::clicked, this,
          &PreferencesDialog::setViewPerspective);
  connect(buttonOrthographic, &QRadioButton::clicked, this,
          &PreferencesDialog::setViewOrthographic);
  connect(sliderPerspective, &QSlider::valueChanged, this,
          &PreferencesDialog::updateSliderPerspective);

  connect(gammaSlider, &QSlider::valueChanged, this,
          &PreferencesDialog::setScreenGamma);
  connect(materialComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &PreferencesDialog::setMaterialFactors);
  connect(metallicSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &PreferencesDialog::setMaterialFactors);
  connect(roughnessSpinBox,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &PreferencesDialog::setMaterialFactors);

  connect(lightCameraFixCheckBox, &QCheckBox::toggled, this,
          &PreferencesDialog::setLightFixedToCamera);
  connect(showLightPositionsCheckBox, &QCheckBox::toggled, this,
          &PreferencesDialog::setShowLightPositions);
  connect(resetLightingButton, &QPushButton::clicked, this,
          &PreferencesDialog::restoreDefaultLightingSettings);
  auto updateLightsLambda = [&](auto) { updateLightPositions(); };
  connect(light1XSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          updateLightsLambda);
  connect(light1YSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          updateLightsLambda);
  connect(light1ZSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          updateLightsLambda);
  connect(light2XSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          updateLightsLambda);
  connect(light2YSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          updateLightsLambda);
  connect(light2ZSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          updateLightsLambda);
  connect(light3XSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          updateLightsLambda);
  connect(light3YSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          updateLightsLambda);
  connect(light3ZSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          updateLightsLambda);
  connect(light4XSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          updateLightsLambda);
  connect(light4YSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          updateLightsLambda);
  connect(light4ZSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          updateLightsLambda);
  connect(lightAmbientColour, &QPushButton::clicked, this,
          &PreferencesDialog::setLightColors);
  connect(light1SpecularColour, &QPushButton::clicked, this,
          &PreferencesDialog::setLightColors);
  connect(light2SpecularColour, &QPushButton::clicked, this,
          &PreferencesDialog::setLightColors);
  connect(light3SpecularColour, &QPushButton::clicked, this,
          &PreferencesDialog::setLightColors);
  connect(light4SpecularColour, &QPushButton::clicked, this,
          &PreferencesDialog::setLightColors);
  connect(lightAmbientSpinBox,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &PreferencesDialog::setLightIntensities);
  connect(lightIntensity1SpinBox,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &PreferencesDialog::setLightIntensities);
  connect(lightIntensity2SpinBox,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &PreferencesDialog::setLightIntensities);
  connect(lightIntensity3SpinBox,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &PreferencesDialog::setLightIntensities);
  connect(lightIntensity4SpinBox,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &PreferencesDialog::setLightIntensities);
  connect(lightAttenuationRangeLowerSpinBox,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &PreferencesDialog::setLightIntensities);
  connect(lightAttenuationRangeUpperSpinBox,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &PreferencesDialog::setLightIntensities);

  connect(textOutlineWidthSlider, &QAbstractSlider::valueChanged, this,
          &PreferencesDialog::setTextSliders);
  connect(textFontSizeSlider, &QAbstractSlider::valueChanged, this,
          &PreferencesDialog::onTextFontSizeChanged);
  connect(textSmoothingWidthSlider, &QAbstractSlider::valueChanged, this,
          &PreferencesDialog::setTextSliders);
  connect(textBufferWidthSlider, &QAbstractSlider::valueChanged, this,
          &PreferencesDialog::setTextSliders);
  connect(textFontComboBox, &QFontComboBox::currentFontChanged, this,
          &PreferencesDialog::onTextFontFamilyChanged);

  // Advanced settings
  // There is no connections for widgets: deleteWorkingFilesCheckBox and
  // enableXHNormalisationCheckBox
  // When accept is called on the preferences dialog, the CrystalExplorer
  // QSettings are updated accordingly.

  connect(restoreExpertSettingsButton, &QPushButton::clicked, this,
          &PreferencesDialog::restoreExpertSettings);
  connect(energyPrecisionSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
          this, &PreferencesDialog::setEnergiesTableDecimalPlaces);
  connect(energyColorSchemeComboBox,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &PreferencesDialog::setEnergiesColorScheme);
  connect(glDepthTestEnabledCheckBox, QOverload<bool>::of(&QCheckBox::toggled),
          this, &PreferencesDialog::setGLDepthTestEnabled);

  // Dialog connections
  connect(buttonOk, &QPushButton::clicked, this, &PreferencesDialog::accept);
}

void PreferencesDialog::setViewOrthographic() { setProjection(false); }

void PreferencesDialog::setViewPerspective() { setProjection(true); }

void PreferencesDialog::setProjection(bool usePerspective) {
  enablePerspectiveSlider(usePerspective);
  settings::writeSetting(settings::keys::USE_PERSPECTIVE_FLAG, usePerspective);
  emit setOpenglProjection(usePerspective, sliderPerspective->value());
}

void PreferencesDialog::enablePerspectiveSlider(bool enable) {
  sliderPerspective->setEnabled(enable);
  perspectiveLabel1->setEnabled(enable);
  perspectiveLabel2->setEnabled(enable);
}

void PreferencesDialog::updateSliderPerspective() {
  // Note we don't bother saving the sliders value to the QSettings
  emit setOpenglProjection(true, sliderPerspective->value());
}

void PreferencesDialog::restoreDefaultExternalProgramSetting() {
  auto index = externalProgramPaths->currentIndex();
  if (!index.isValid())
    return;

  if (index.parent().isValid()) {
    // child item
    QString key =
        m_externalProgramSettingsModel->itemFromIndex(index.parent())->text() +
        "/" + m_externalProgramSettingsModel->itemFromIndex(index)->text();
    settings::restoreDefaultSetting(key);
    m_externalProgramSettingsModel->itemFromIndex(index)->setText(
        settings::readSetting(key).toString());
  }
}

void PreferencesDialog::handleExternalProgramSettingsDoubleClick(
    const QModelIndex &index) {
  if (!index.isValid())
    return;

  if (index.parent().isValid()) {
    getValueForExternalProgramSetting(
        m_externalProgramSettingsModel->itemFromIndex(index));
  }
}

void PreferencesDialog::getValueForExternalProgramSetting(QStandardItem *item) {
  if (item->parent() == nullptr)
    return;
  if (item->column() != 2)
    return;
  const QString &currentValue = item->text();
  QStandardItem *settingItem = item->parent()->child(item->row(), 1);
  if (settingItem == nullptr) {
    qDebug() << "Something very wrong in getValueForExternalProgramSetting: no "
                "setting item!";
    return;
  }
  const QString &setting = settingItem->text();
  qDebug() << "Setting is" << setting;
  if (setting == "executablePath") {
    QString path = QFileDialog::getOpenFileName(
        0, QString("Executable path for %1").arg(item->parent()->text()),
        currentValue);
    if (!path.isEmpty())
      item->setText(path);
  } else if (setting == "dataDirectory") {
    QString path = QFileDialog::getExistingDirectory(
        0, QString("Data path for %1").arg(item->parent()->text()),
        currentValue);
    if (!path.isEmpty())
      item->setText(path);
  } else {
    externalProgramPaths->edit(item->index());
  }
}

void PreferencesDialog::accept() {
  updateSettingsFromDialog();
  QDialog::accept();
}

void PreferencesDialog::show() {
  updateDialogFromSettings();
  QWidget::show();
}

void PreferencesDialog::updateLightsFromSettings() {
  blockSignals(true);
  lightPositionGroupBox->setHidden(
      settings::readSetting(settings::keys::LIGHT_TRACKS_CAMERA).toBool());

  if (lightPositionGroupBox->isVisible()) {
    QVector3D pos = settings::readSetting(settings::keys::LIGHT_POSITION_1)
                        .value<QVector3D>();
    light1XSpinBox->setValue(pos.x());
    light1YSpinBox->setValue(pos.y());
    light1ZSpinBox->setValue(pos.z());
    pos = settings::readSetting(settings::keys::LIGHT_POSITION_2)
              .value<QVector3D>();
    light2XSpinBox->setValue(pos.x());
    light2YSpinBox->setValue(pos.y());
    light2ZSpinBox->setValue(pos.z());
    pos = settings::readSetting(settings::keys::LIGHT_POSITION_3)
              .value<QVector3D>();
    light3XSpinBox->setValue(pos.x());
    light3YSpinBox->setValue(pos.y());
    light3ZSpinBox->setValue(pos.z());
    pos = settings::readSetting(settings::keys::LIGHT_POSITION_4)
              .value<QVector3D>();
    light4XSpinBox->setValue(pos.x());
    light4YSpinBox->setValue(pos.y());
    light4ZSpinBox->setValue(pos.z());
  }
  materialComboBox->setCurrentIndex(
      settings::readSetting(settings::keys::MATERIAL).toInt() - 1);
  setButtonColor(
      lightAmbientColour,
      settings::readSetting(settings::keys::LIGHT_AMBIENT).toString());
  setButtonColor(
      light1SpecularColour,
      settings::readSetting(settings::keys::LIGHT_SPECULAR_1).toString());
  setButtonColor(
      light2SpecularColour,
      settings::readSetting(settings::keys::LIGHT_SPECULAR_2).toString());
  setButtonColor(
      light3SpecularColour,
      settings::readSetting(settings::keys::LIGHT_SPECULAR_3).toString());
  setButtonColor(
      light4SpecularColour,
      settings::readSetting(settings::keys::LIGHT_SPECULAR_4).toString());
  lightAmbientSpinBox->setValue(
      settings::readSetting(settings::keys::LIGHT_AMBIENT_INTENSITY).toFloat());
  lightIntensity1SpinBox->setValue(
      settings::readSetting(settings::keys::LIGHT_INTENSITY_1).toFloat());
  lightIntensity2SpinBox->setValue(
      settings::readSetting(settings::keys::LIGHT_INTENSITY_2).toFloat());
  lightIntensity3SpinBox->setValue(
      settings::readSetting(settings::keys::LIGHT_INTENSITY_3).toFloat());
  lightIntensity4SpinBox->setValue(
      settings::readSetting(settings::keys::LIGHT_INTENSITY_4).toFloat());

  lightAttenuationRangeLowerSpinBox->setValue(
      settings::readSetting(settings::keys::LIGHT_ATTENUATION_MINIMUM)
          .toFloat());
  lightAttenuationRangeUpperSpinBox->setValue(
      settings::readSetting(settings::keys::LIGHT_ATTENUATION_MAXIMUM)
          .toFloat());

  lightCameraFixCheckBox->setChecked(
      settings::readSetting(settings::keys::LIGHT_TRACKS_CAMERA).toBool());
  blockSignals(false);
}

void PreferencesDialog::restoreDefaultLightingSettings() {
  // TODO shift this into Settings, functionality should be in there.
  settings::restoreDefaultSettings({
      settings::keys::LIGHT_TRACKS_CAMERA,
      settings::keys::LIGHT_AMBIENT,
      settings::keys::LIGHT_AMBIENT_INTENSITY,
      settings::keys::SHOW_LIGHT_POSITIONS,
      settings::keys::LIGHT_SPECULAR_1,
      settings::keys::LIGHT_SPECULAR_2,
      settings::keys::LIGHT_SPECULAR_3,
      settings::keys::LIGHT_SPECULAR_4,
      settings::keys::LIGHT_POSITION_1,
      settings::keys::LIGHT_POSITION_2,
      settings::keys::LIGHT_POSITION_3,
      settings::keys::LIGHT_POSITION_4,
      settings::keys::LIGHT_INTENSITY_1,
      settings::keys::LIGHT_INTENSITY_2,
      settings::keys::LIGHT_INTENSITY_3,
      settings::keys::LIGHT_INTENSITY_4,
      settings::keys::LIGHT_ATTENUATION_MINIMUM,
      settings::keys::LIGHT_ATTENUATION_MAXIMUM,
  });

  updateLightsFromSettings();
  emit lightSettingsChanged();
}

void PreferencesDialog::loadExternalProgramSettings() {
  QStringList wavefunctionSources = {"OCC"};

  m_externalProgramSettingsModel->clear();
  m_externalProgramSettingsModel->setColumnCount(3);
  m_externalProgramSettingsModel->setHorizontalHeaderLabels(
      {"Program", "Setting", "Value"});
  for (auto kv = m_externalProgramSettingsKeys.constKeyValueBegin();
       kv != m_externalProgramSettingsKeys.constKeyValueEnd(); kv++) {
    QStandardItem *programItem = new QStandardItem(kv->first);
    programItem->setFlags(programItem->flags() & ~Qt::ItemIsEditable);
    m_externalProgramSettingsModel->appendRow(programItem);
    for (const auto &setting : std::as_const(kv->second)) {
      QString fullKey = kv->first + "/" + setting;
      QString currentValue = settings::readSetting(fullKey).toString();
      if (setting == "executablePath" && !currentValue.isEmpty()) {
        wavefunctionSources.append(kv->first);
      }
      QStandardItem *blankItem = new QStandardItem("");
      blankItem->setFlags(blankItem->flags() & ~Qt::ItemIsEditable);
      blankItem->setData(fullKey, PreferencesRoles::PreferencesKeyRole);
      QStandardItem *settingItem = new QStandardItem(setting);
      settingItem->setFlags(settingItem->flags() & ~Qt::ItemIsEditable);
      settingItem->setData(fullKey, PreferencesRoles::PreferencesKeyRole);
      QStandardItem *valueItem = new QStandardItem(currentValue);
      valueItem->setData(QVariant(fullKey),
                         PreferencesRoles::PreferencesKeyRole);
      programItem->appendRow({blankItem, settingItem, valueItem});
    }
  }

  preferredWavefunctionSourceComboBox->clear();
  preferredWavefunctionSourceComboBox->addItems(wavefunctionSources);
  QString preferredWavefunctionSource =
      settings::readSetting(settings::keys::PREFERRED_WAVEFUNCTION_SOURCE)
          .toString();
  int preferredIndex = wavefunctionSources.indexOf(preferredWavefunctionSource);
  if (preferredIndex < 0)
    preferredIndex = 0;
  preferredWavefunctionSourceComboBox->setCurrentIndex(preferredIndex);
}

void PreferencesDialog::updateExternalProgramSettings() {
  const int PROGRAM_COLUMN = 0;
  // const int SETTING_COLUMN = 1;
  const int VALUE_COLUMN = 2;

  int rowCount = m_externalProgramSettingsModel->rowCount();
  for (int i = 0; i < rowCount; i++) {
    auto *programItem = m_externalProgramSettingsModel->item(i, PROGRAM_COLUMN);
    if (!programItem)
      continue;
    if (programItem->hasChildren()) {
      for (int j = 0; j < programItem->rowCount(); j++) {
        auto *valueItem = programItem->child(j, VALUE_COLUMN);
        if (!valueItem)
          continue;
        QString key =
            valueItem->data(PreferencesRoles::PreferencesKeyRole).toString();
        settings::writeSetting(key, valueItem->text());
      }
    }
  }
}

void PreferencesDialog::updateDialogFromSettings() {
  _updateDialogFromSettingsDone = false;
  // General Preferences
  autoloadLastFileCheckBox->setChecked(
      settings::readSetting(settings::keys::AUTOLOAD_LAST_FILE).toBool());

  loadExternalProgramSettings();

  // Display Preferences
  m_currentBackgroundColor = QColor(
      settings::readSetting(settings::keys::BACKGROUND_COLOR).toString());
  setButtonColor(backgroundColorButton, m_currentBackgroundColor);

  m_currentTextLabelColor =
      QColor(settings::readSetting(settings::keys::TEXT_COLOR).toString());
  setButtonColor(textColorButton, m_currentTextLabelColor);

  m_currentTextLabelOutlineColor = QColor(
      settings::readSetting(settings::keys::TEXT_OUTLINE_COLOR).toString());
  setButtonColor(textOutlineColorButton, m_currentTextLabelOutlineColor);

  m_currentFaceHighlightColor = QColor(
      settings::readSetting(settings::keys::FACE_HIGHLIGHT_COLOR).toString());
  setButtonColor(faceHighlightColorButton, m_currentFaceHighlightColor);

  m_currentNonePropertyColor = QColor(
      settings::readSetting(settings::keys::NONE_PROPERTY_COLOR).toString());
  setButtonColor(nonePropertyColorButton, m_currentNonePropertyColor);

  setButtonColor(
      energyFrameworkPositiveColorButton,
      settings::readSetting(settings::keys::ENERGY_FRAMEWORK_POSITIVE_COLOR)
          .toString());
  m_currentSelectionColor =
      QColor(settings::readSetting(settings::keys::SELECTION_COLOR).toString());
  setButtonColor(selectionColorButton, m_currentSelectionColor);

  textOutlineWidthSlider->setValue(static_cast<int>(
      settings::readSetting(settings::keys::TEXT_OUTLINE).toFloat() * 100.0f));
  textFontSizeSlider->setValue(static_cast<int>(
      settings::readSetting(settings::keys::TEXT_FONT_SIZE).toInt()));
  textSmoothingWidthSlider->setValue(static_cast<int>(
      settings::readSetting(settings::keys::TEXT_SMOOTHING).toFloat() *
      100.0f));
  textBufferWidthSlider->setValue(static_cast<int>(
      settings::readSetting(settings::keys::TEXT_BUFFER).toFloat() * 100.0f));
  glDepthTestEnabledCheckBox->setChecked(
      settings::readSetting(settings::keys::ENABLE_DEPTH_TEST).toBool());

  updateLightsFromSettings();
  bondThicknessSlider->setValue(
      settings::readSetting(settings::keys::BOND_THICKNESS).toInt());

  contactLineThicknessSlider->setValue(
      settings::readSetting(settings::keys::CONTACT_LINE_THICKNESS).toInt());

  buttonPerspective->setChecked(
      settings::readSetting(settings::keys::USE_PERSPECTIVE_FLAG).toBool());
  sliderPerspective->setValue(GLOBAL_PERSPECTIVE_LEVEL);

  // Advanced Preferences
  enableXHNormalisationCheckBox->setChecked(
      settings::readSetting(settings::keys::XH_NORMALIZATION).toBool());
  deleteWorkingFilesCheckBox->setChecked(
      settings::readSetting(settings::keys::DELETE_WORKING_FILES).toBool());
  writeGaussianCPFilesCheckBox->setChecked(
      settings::readSetting(settings::keys::WRITE_GAUSSIAN_CP_FILES).toBool());
  energyPrecisionSpinBox->setValue(
      settings::readSetting(settings::keys::ENERGY_TABLE_PRECISION).toInt());

  _updateDialogFromSettingsDone = true;
}

void PreferencesDialog::setButtonColor(QAbstractButton *colorButton,
                                       QColor color) {
  QPixmap pixmap = QPixmap(colorButton->iconSize());
  pixmap.fill(color);
  colorButton->setIcon(QIcon(pixmap));
}

QColor PreferencesDialog::getButtonColor(QAbstractButton *colorButton) {
  return colorButton->icon().pixmap(1, 1).toImage().pixel(0, 0);
}

void PreferencesDialog::updateSettingsFromDialog() {
  settings::writeSettings(
      {// General Preferences
       {settings::keys::AUTOLOAD_LAST_FILE,
        autoloadLastFileCheckBox->isChecked()},
       // Graphics
       {settings::keys::BACKGROUND_COLOR,
        getButtonColor(backgroundColorButton).name()},
       {settings::keys::FACE_HIGHLIGHT_COLOR,
        getButtonColor(faceHighlightColorButton).name()},
       {settings::keys::SELECTION_COLOR,
        getButtonColor(selectionColorButton).name()},
       {settings::keys::BOND_THICKNESS, bondThicknessSlider->value()},
       {settings::keys::CONTACT_LINE_THICKNESS,
        contactLineThicknessSlider->value()},
       // Advanced Preferences
       {settings::keys::DELETE_WORKING_FILES,
        deleteWorkingFilesCheckBox->isChecked()},
       {settings::keys::XH_NORMALIZATION,
        enableXHNormalisationCheckBox->isChecked()},
       {settings::keys::WRITE_GAUSSIAN_CP_FILES,
        writeGaussianCPFilesCheckBox->isChecked()},
       {settings::keys::PREFERRED_WAVEFUNCTION_SOURCE,
        preferredWavefunctionSourceComboBox->currentText()}});
  updateExternalProgramSettings();
}

void PreferencesDialog::contextualGlwindowBackgroundColor() {
  QColor color = QColorDialog::getColor(m_currentBackgroundColor, this);
  if (color.isValid()) {
    updateGlwindowBackgroundColor(color);
    emit glwindowBackgroundColorChanged(color);
  }
}

void PreferencesDialog::updateGlwindowBackgroundColor(QColor color) {
  m_currentBackgroundColor = color;
  setButtonColor(backgroundColorButton, color);
  settings::writeSetting(settings::keys::BACKGROUND_COLOR, color.name());
}

void PreferencesDialog::setFaceHighlightColor() {
  QColor color = QColorDialog::getColor(m_currentFaceHighlightColor, this);
  if (color.isValid()) {
    m_currentFaceHighlightColor = color;
    setButtonColor(faceHighlightColorButton, color);
    settings::writeSetting(settings::keys::FACE_HIGHLIGHT_COLOR, color.name());
    emit faceHighlightColorChanged();
  }
}

void PreferencesDialog::setTextLabelColor() {
  QColor color =
      QColorDialog::getColor(m_currentTextLabelColor, this, "Text label color");
  if (color.isValid()) {
    m_currentTextLabelColor = color;
    setButtonColor(textColorButton, color);
    settings::writeSetting(settings::keys::TEXT_COLOR, color.name());
    emit textSettingsChanged();
  }
}

void PreferencesDialog::setTextLabelOutlineColor() {
  QColor color = QColorDialog::getColor(m_currentTextLabelOutlineColor, this,
                                        "Text label outline color");
  if (color.isValid()) {
    m_currentTextLabelOutlineColor = color;
    setButtonColor(textOutlineColorButton, color);
    settings::writeSetting(settings::keys::TEXT_OUTLINE_COLOR, color.name());
    emit textSettingsChanged();
  }
}

void PreferencesDialog::setNonePropertyColor() {
  QColor color = QColorDialog::getColor(m_currentNonePropertyColor, this);
  if (color.isValid()) {
    m_currentNonePropertyColor = color;
    setButtonColor(nonePropertyColorButton, color);
    settings::writeSetting(settings::keys::NONE_PROPERTY_COLOR, color.name());
    emit nonePropertyColorChanged();
  }
}

void PreferencesDialog::setSelectionColor() {
  QColor color = QColorDialog::getColor(m_currentSelectionColor, this);
  if (color.isValid()) {
    m_currentSelectionColor = color;
    setButtonColor(selectionColorButton, color);
    settings::writeSetting(settings::keys::SELECTION_COLOR, color.name());
    emit selectionColorChanged();
  }
}

void PreferencesDialog::setBondThickness(int value) {
  settings::writeSetting(settings::keys::BOND_THICKNESS, value);
  if (_updateDialogFromSettingsDone) {
    emit redrawCrystalForPreferencesChange();
  }
}

void PreferencesDialog::setEnergiesTableDecimalPlaces(int value) {
  settings::writeSetting(settings::keys::ENERGY_TABLE_PRECISION, value);
}

void PreferencesDialog::setEnergiesColorScheme(int index) {
  settings::writeSetting(settings::keys::ENERGY_COLOR_SCHEME,
                         energyColorSchemeComboBox->itemText(index));
}

void PreferencesDialog::setScreenGamma(int value) {
  settings::writeSetting(settings::keys::SCREEN_GAMMA, value / 100.0f);
  emit screenGammaChanged();
}

void PreferencesDialog::setLightFixedToCamera(bool value) {
  settings::writeSetting(settings::keys::LIGHT_TRACKS_CAMERA, value);
  updateLightsFromSettings();
  emit lightSettingsChanged();
}

void PreferencesDialog::setShowLightPositions(bool value) {
  settings::writeSetting(settings::keys::SHOW_LIGHT_POSITIONS, value);
  updateLightsFromSettings();
  emit lightSettingsChanged();
}

void PreferencesDialog::updateLightPositions() {
  settings::writeSetting(settings::keys::LIGHT_POSITION_1,
                         QVector3D(light1XSpinBox->value(),
                                   light1YSpinBox->value(),
                                   light1ZSpinBox->value()));
  settings::writeSetting(settings::keys::LIGHT_POSITION_2,
                         QVector3D(light2XSpinBox->value(),
                                   light2YSpinBox->value(),
                                   light2ZSpinBox->value()));
  settings::writeSetting(settings::keys::LIGHT_POSITION_3,
                         QVector3D(light3XSpinBox->value(),
                                   light3YSpinBox->value(),
                                   light3ZSpinBox->value()));
  settings::writeSetting(settings::keys::LIGHT_POSITION_4,
                         QVector3D(light4XSpinBox->value(),
                                   light4YSpinBox->value(),
                                   light4ZSpinBox->value()));
  updateLightsFromSettings();
  emit lightSettingsChanged();
}

void PreferencesDialog::setLightColors() {
  QPushButton *sender = qobject_cast<QPushButton *>(QObject::sender());
  if (sender == nullptr)
    return;
  const QString &senderName = sender->objectName();
  const auto val = m_lightColorKeys.constFind(senderName);
  if (val == m_lightColorKeys.constEnd())
    return;

  QColor currentColor(settings::readSetting(val.value()).toString());
  QColor newColor = QColorDialog::getColor(currentColor, this);
  if (newColor.isValid()) {
    settings::writeSetting(val.value(), newColor.name());
    setButtonColor(sender, newColor);
    emit lightSettingsChanged();
  }
}

void PreferencesDialog::setLightIntensities(double value) {
  QDoubleSpinBox *sender = qobject_cast<QDoubleSpinBox *>(QObject::sender());
  if (sender == nullptr)
    return;
  const QString &senderName = sender->objectName();
  const auto val = m_lightIntensityKeys.constFind(senderName);
  if (val == m_lightIntensityKeys.constEnd())
    return;

  settings::writeSetting(val.value(), value);
  emit lightSettingsChanged();
}

void PreferencesDialog::setTextSliders(int value) {
  QAbstractSlider *sender = qobject_cast<QAbstractSlider *>(QObject::sender());
  if (sender == nullptr)
    return;
  const QString &senderName = sender->objectName();
  const auto val = m_textSliderKeys.constFind(senderName);
  if (val == m_textSliderKeys.constEnd())
    return;
  settings::writeSetting(val.value(), value / 100.0f);
  emit textSettingsChanged();
}

void PreferencesDialog::setMaterialFactors() {
  settings::writeSetting(settings::keys::MATERIAL_METALLIC,
                         metallicSpinBox->value());
  settings::writeSetting(settings::keys::MATERIAL_ROUGHNESS,
                         roughnessSpinBox->value());
  settings::writeSetting(settings::keys::MATERIAL,
                         materialComboBox->currentIndex() + 1);
  emit materialChanged();
}

void PreferencesDialog::setContactLineThickness(int value) {
  settings::writeSetting(settings::keys::CONTACT_LINE_THICKNESS, value);
  if (_updateDialogFromSettingsDone) {
    emit redrawCloseContactsForPreferencesChange();
  }
}

void PreferencesDialog::editElements() {
  if (m_periodicTableDialog == nullptr) {
    m_periodicTableDialog = new PeriodicTableDialog(this);
    connect(m_periodicTableDialog, &PeriodicTableDialog::elementChanged, this,
            &PreferencesDialog::redrawCrystalForPreferencesChange);
    connect(this, &PreferencesDialog::resetElementData, m_periodicTableDialog,
            &PeriodicTableDialog::resetElements);
  }
  m_periodicTableDialog->show();
}

void PreferencesDialog::setGLDepthTestEnabled(bool value) {
  settings::writeSetting(settings::keys::ENABLE_DEPTH_TEST, value);
  emit glDepthTestEnabledChanged(value);
}

void PreferencesDialog::setJmolColors(bool value) {
  settings::writeSetting(settings::keys::USE_JMOL_COLORS, value);
}

void PreferencesDialog::resetAllElements() {
  QString title = "Reset All Elements?";
  QString msg = "You are about to reset the data (colors, radii etc.) for all "
                "elements.\n\nAll previous changes will be lost.\n\nDo you "
                "want to continue?";
  if (QMessageBox::warning(this, title, msg,
                           QMessageBox::Cancel | QMessageBox::Ok) ==
      QMessageBox::Ok) {
    emit resetElementData();
    emit redrawCrystalForPreferencesChange();
  }
}

void PreferencesDialog::restoreExpertSettings() {

  settings::restoreDefaultSettings(
      {settings::keys::DELETE_WORKING_FILES, settings::keys::XH_NORMALIZATION});
  deleteWorkingFilesCheckBox->setChecked(
      settings::readSetting(settings::keys::DELETE_WORKING_FILES).toBool());
  enableXHNormalisationCheckBox->setChecked(
      settings::readSetting(settings::keys::XH_NORMALIZATION).toBool());
}

void PreferencesDialog::setEnergyFrameworkPositiveColor() {
  QColor old =
      settings::readSetting(settings::keys::ENERGY_FRAMEWORK_POSITIVE_COLOR)
          .value<QColor>();
  QColor color = QColorDialog::getColor(old, this);
  if (color.isValid()) {
    settings::writeSetting(settings::keys::ENERGY_FRAMEWORK_POSITIVE_COLOR,
                           color);
    setButtonColor(energyFrameworkPositiveColorButton, color);
    emit redrawCrystalForPreferencesChange();
  }
}

void PreferencesDialog::populateExecutablesFromPath(bool override) {

  for (auto kv = m_externalProgramSettingsKeys.constKeyValueBegin();
       kv != m_externalProgramSettingsKeys.constKeyValueEnd(); kv++) {
    QString group = kv->first;
    QString currentSetting =
        settings::readSetting(group + "/executablePath").toString();
    if (!(override || currentSetting.isEmpty()))
      continue;
    auto availableSettings = kv->second;
    qDebug() << "Populate empty executables for" << group;
    QStringList names;
    if (availableSettings.contains("executableNames")) {
      names = settings::readSetting(group + "/executableNames").toStringList();
    } else {
      names.push_back(group);
    }
    for (const auto &name : names) {
      auto result = exe::findProgramInPath(name);
      if (!result.isEmpty()) {
        settings::writeSetting(group + "/executablePath", result);
        break;
      }
    }
  }
}

void PreferencesDialog::onTextFontFamilyChanged(const QFont &font) {
  settings::writeSetting(settings::keys::TEXT_FONT_FAMILY, font.family());
  emit textSettingsChanged();
}

void PreferencesDialog::onTextFontSizeChanged(int size) {
  settings::writeSetting(settings::keys::TEXT_FONT_SIZE, size);
  emit textSettingsChanged();
}
