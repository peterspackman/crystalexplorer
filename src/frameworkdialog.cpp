#include "frameworkdialog.h"
#include "settings.h"
#include "ui_frameworkdialog.h"

#include <QDebug>

FrameworkDialog::FrameworkDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::FrameworkDialog) {
  ui->setupUi(this);
  init();
  initConnections();
}

FrameworkDialog::~FrameworkDialog() { delete ui; }

void FrameworkDialog::init() {
  Qt::WindowFlags flags = windowFlags();
  setWindowFlags(flags | Qt::WindowStaysOnTopHint);
  m_showOptions = false;
  updateOptions(m_showOptions);
}

void FrameworkDialog::initConnections() {
  connect(ui->prevButton, QOverload<bool>::of(&QPushButton::clicked), this,
          [this](bool) { emit cycleFrameworkRequested(true); });
  connect(ui->nextButton, QOverload<bool>::of(&QPushButton::clicked), this,
          [this](bool) { emit cycleFrameworkRequested(false); });
  connect(ui->energyTheoriesCombobox,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &FrameworkDialog::energyTheoriesComboboxChanged);
  connect(ui->scaleSpinbox, SIGNAL(valueChanged(double)), this,
          SLOT(scaleSpinboxChanged(double)));
  connect(ui->cutoffSpinbox, SIGNAL(valueChanged(double)), this,
          SLOT(cutoffSpinboxChanged(double)));

  connect(ui->optionsButton, SIGNAL(clicked()), this,
          SLOT(optionsButtonClicked()));
  connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(accept()));
}

void FrameworkDialog::setCurrentFramework(FrameworkType frameworkType) {
  m_currentFramework = frameworkType;
  auto detailedDescriptions = getDetailedDescriptions();
  auto frameworkColors = getFrameworkColors();
  auto showScaleOptionsFlags = getShowScaleOptionsFlags();
  setCurrentFrameworkLabel(detailedDescriptions[m_currentFramework],
                           frameworkColors[m_currentFramework]);
  enableScaleOptions(showScaleOptionsFlags[m_currentFramework]);
  updateCutoffSpinboxFromSettings();
  updateScaleSpinboxFromSettings();
}

void FrameworkDialog::setCurrentFrameworkLabel(QString text, QColor color) {
  QLabel *label = ui->currentFrameworkLabel;

  label->setText(text);

  QPalette palette = label->palette();
  palette.setColor(label->foregroundRole(), color.darker());
  label->setPalette(palette);

  QFont font = label->font();
  font.setPointSize(14);
  font.setBold(true);
  label->setFont(font);
}

void FrameworkDialog::enableScaleOptions(bool enable) {
  ui->scaleSpinbox->setEnabled(enable);
}

void FrameworkDialog::updateScaleSpinboxFromSettings() {
  float scale =
      settings::readSetting(settings::keys::ENERGY_FRAMEWORK_SCALE).toFloat();
  ui->scaleSpinbox->setValue(scale * scaleRescale);
}

void FrameworkDialog::updateCutoffSpinboxFromSettings() {
  auto cutoffSettingsKeys = getCutoffSettingsKeys();
  float cutoff =
      settings::readSetting(cutoffSettingsKeys[m_currentFramework]).toFloat();

  ui->cutoffSpinbox->setValue(cutoff);
}

void FrameworkDialog::reject() {
  cleanupForClosing();
  QDialog::reject();
}

void FrameworkDialog::accept() {
  cleanupForClosing();
  QDialog::accept();
}

void FrameworkDialog::cleanupForClosing() { emit frameworkDialogClosing(); }

void FrameworkDialog::scaleSpinboxChanged(double value) {
  settings::writeSetting(settings::keys::ENERGY_FRAMEWORK_SCALE,
                         value / scaleRescale);
  emit frameworkDialogScaleChanged();
}

void FrameworkDialog::cutoffSpinboxChanged(double value) {
  auto cutoffSettingsKeys = getCutoffSettingsKeys();
  settings::writeSetting(cutoffSettingsKeys[m_currentFramework], value);

  emit frameworkDialogCutoffChanged();
}

void FrameworkDialog::optionsButtonClicked() {
  m_showOptions = !m_showOptions; // _toggle showOptions
  updateOptions(m_showOptions);
}

void FrameworkDialog::updateOptionsVisibility(bool showOptions) {
  ui->optionsGroupbox->setVisible(showOptions);
}

void FrameworkDialog::updateOptionsButtonText(bool showOptions) {
  QString text = showOptions ? "Hide Options" : "Show Options";
  ui->optionsButton->setText(text);
}

void FrameworkDialog::updateOptions(bool showOptions) {
  updateOptionsVisibility(showOptions);
  updateOptionsButtonText(showOptions);
  adjustSize();
}

void FrameworkDialog::setEnergyTheories(
    const QVector<EnergyTheory> &energyTheories) {
  m_energyTheories = energyTheories;
  updateEnergyTheories();
}

void FrameworkDialog::updateEnergyTheories() {
  QStringList items;

  for (const auto &theory : m_energyTheories) {
    items << Wavefunction::levelOfTheoryString(theory.first, theory.second);
  }

  ui->energyTheoriesCombobox->clear();
  ui->energyTheoriesCombobox->addItems(items);
}

void FrameworkDialog::energyTheoriesComboboxChanged(int index) {
  // When clearing the combobox in updateEnergyTheories, this method gets calls
  // with an index of -1
  if (index == -1) {
    return;
  }

  EnergyTheory theory = m_energyTheories[index];
  emit energyTheoryChanged(theory);
}
