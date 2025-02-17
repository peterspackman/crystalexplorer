#include "fingerprintoptions.h"
#include "settings.h"
#include <QFileDialog>
#include <QMessageBox>

#include "math.h"
#include <QDebug>

FingerprintOptions::FingerprintOptions(QWidget *parent) : QWidget(parent) {
  setupUi(this);
  init();
  initConnections();
}

void FingerprintOptions::init() {
  surfaceAreaProgressBar->setFormat(""); // prevent default percentage label on
                                         // windows/linux (since we display our
                                         // own)
  plotRangeComboBox->addItems(plotRangeLabels());
  filterComboBox->addItems(filterOptions());
  resetOptions();
}

void FingerprintOptions::initConnections() {
  // plot type/range
  connect(plotRangeComboBox, &QComboBox::currentIndexChanged, this,
          &FingerprintOptions::updatePlotRange);
  // filter type
  connect(filterComboBox, &QComboBox::currentIndexChanged,
          [this](int val) { this->updateVisibilityOfFilterWidgets(val); });
  connect(filterComboBox, &QComboBox::currentIndexChanged, this,
          &FingerprintOptions::updateFilterSettings);
  // filter options
  connect(inElementComboBox, &QComboBox::currentIndexChanged, this,
          &FingerprintOptions::updateFilterSettings);
  connect(outElementComboBox, &QComboBox::currentIndexChanged, this,
          &FingerprintOptions::updateFilterSettings);
  connect(incRecipContactsCheckBox, &QCheckBox::toggled, this,
          &FingerprintOptions::updateFilterSettings);

  connect(filterMaximumSpinBox, &QDoubleSpinBox::valueChanged, this,
          &FingerprintOptions::updateFilterSettings);
  connect(filterMinimumSpinBox, &QDoubleSpinBox::valueChanged, this,
          &FingerprintOptions::updateFilterSettings);

  // Save as button
  connect(saveAsPushButton, &QPushButton::clicked, this,
          &FingerprintOptions::getFilenameAndSaveFingerprint);
  connect(closeButton, &QPushButton::clicked, this,
          &FingerprintOptions::closeClicked);
}

QStringList FingerprintOptions::filterOptions() {
  QStringList labels;
  for (const auto &mode : requestableFilters) {
    labels.push_back(fingerprintFilterLabels[static_cast<int>(mode)]);
  }
  return labels;
}

QStringList FingerprintOptions::plotRangeLabels() {

  const auto ranges = {
      FingerprintPlotRange::Standard,
      FingerprintPlotRange::Translated,
      FingerprintPlotRange::Expanded,
  };
  QStringList labels;
  for (const auto &plotRange : ranges) {
    labels.push_back(plotRangeSettings(plotRange).label);
  }
  return labels;
}

void FingerprintOptions::setButtonColor(QToolButton *colorButton,
                                        QColor color) {
  QPixmap pixmap = QPixmap(colorButton->iconSize());
  pixmap.fill(color);
  colorButton->setIcon(QIcon(pixmap));
}

QColor FingerprintOptions::getButtonColor(QToolButton *colorButton) {
  return colorButton->icon().pixmap(1, 1).toImage().pixel(0, 0);
}

void FingerprintOptions::resetOptions() {
  plotRangeComboBox->setCurrentIndex(0); // standard range
  filterComboBox->setCurrentIndex(0);    // no filter
  updateFilterMode();
}

void FingerprintOptions::resetElementFilterOptions() {
  inElementComboBox->setCurrentIndex(0);
  outElementComboBox->setCurrentIndex(0);
  incRecipContactsCheckBox->setChecked(false);
}

void FingerprintOptions::resetFilter() {
  filterComboBox->setCurrentIndex(0);

  resetElementFilterOptions();
  surfaceAreaLabel->setText("100 %");
  surfaceAreaProgressBar->setValue(100);
}

void FingerprintOptions::updateFilterMode() {
  FingerprintFilterMode filterMode = getFilterMode();
  updateVisibilityOfFilterWidgets(filterMode);
  if (filterMode == FingerprintFilterMode::None) {
    resetFilter();
  }
  updateFilterSettings();
}

void FingerprintOptions::updateVisibilityOfFilterWidgets(int currentIndex) {
  Q_ASSERT(currentIndex >= 0);
  Q_ASSERT(currentIndex < requestableFilters.size());
  updateVisibilityOfFilterWidgets(requestableFilters[currentIndex]);
}

void FingerprintOptions::updateVisibilityOfFilterWidgets(
    FingerprintFilterMode filterMode) {
  setVisibleElementFilteringWidgets(false);
  setVisibleSelectionFilteringWidgets(false);
  setVisibleCommonFilteringWidgets(false);
  setVisibleRangeFilteringWidgets(false);

  switch (filterMode) {
  case FingerprintFilterMode::None:
    // handled above
    break;
  case FingerprintFilterMode::Element:
    setVisibleElementFilteringWidgets(true);
    setVisibleCommonFilteringWidgets(true);
    break;
  case FingerprintFilterMode::De:
    setVisibleRangeFilteringWidgets(true);
    setVisibleCommonFilteringWidgets(true);
    break;
  case FingerprintFilterMode::Di:
    setVisibleRangeFilteringWidgets(true);
    setVisibleCommonFilteringWidgets(true);
    break;
  }
}

void FingerprintOptions::setVisibleSelectionFilteringWidgets(bool visible) {
  selectionFilterBox->setVisible(visible);
}

void FingerprintOptions::setVisibleElementFilteringWidgets(bool visible) {
  elementFilterOptionsBox->setVisible(visible);
}

void FingerprintOptions::setVisibleRangeFilteringWidgets(bool visible) {
  valueRangeGroupBox->setVisible(visible);
}

void FingerprintOptions::setVisibleCommonFilteringWidgets(bool visible) {
  filterResultsBox->setVisible(visible);
}

FingerprintFilterMode FingerprintOptions::getFilterMode() {
  int currentIndex = filterComboBox->currentIndex();
  return requestableFilters[currentIndex];
}

void FingerprintOptions::setElementList(QStringList elementSymbols) {

  inElementComboBox->blockSignals(true);
  outElementComboBox->blockSignals(true);

  inElementComboBox->clear();
  outElementComboBox->clear();

  elementSymbols.prepend(NONE_ELEMENT_LABEL);

  inElementComboBox->addItems(elementSymbols);
  outElementComboBox->addItems(elementSymbols);

  inElementComboBox->blockSignals(false);
  outElementComboBox->blockSignals(false);
  updateFilterSettings();
}

void FingerprintOptions::updatePlotRange(int index) {
  emit plotRangeChanged(static_cast<FingerprintPlotRange>(index));
}

void FingerprintOptions::updateFilterSettings() {
  FingerprintFilterOptions opts;
  opts.filterMode = getFilterMode();

  opts.insideFilterElementSymbol = inElementComboBox->currentText();
  opts.outsideFilterElementSymbol = outElementComboBox->currentText();

  opts.filterInsideElement =
      !(opts.insideFilterElementSymbol == NONE_ELEMENT_LABEL);
  opts.filterOutsideElement =
      !(opts.outsideFilterElementSymbol == NONE_ELEMENT_LABEL);

  // disable reciprocal contacts checkbox when None is selected in either
  // inElementComboBox or outElementComboBox
  incRecipContactsCheckBox->setEnabled(opts.filterInsideElement &&
                                       opts.filterOutsideElement);
  if (!incRecipContactsCheckBox->isEnabled()) {
    incRecipContactsCheckBox->setChecked(false);
  }

  opts.includeReciprocalContacts = incRecipContactsCheckBox->isChecked();
  opts.filterLower = filterMinimumSpinBox->value();
  opts.filterUpper = filterMaximumSpinBox->value();
  emit filterChanged(opts);
}

void FingerprintOptions::getFilenameAndSaveFingerprint() {
  QString filename;
  // if (settings::readSetting(settings::keys::ALLOW_CSV_FINGERPRINT_EXPORT)
  //		filename = QFileDialog::getSaveFileName(nullptr, tr("Save
  //  Fingerprint"), "untitled.eps", tr("Postscript (*.eps);;Comma Separated
  //  Values (*.csv);;Scalable Vector Graphics (*.svg)"));
  //	else
  //		filename = QFileDialog::getSaveFileName(nullptr, tr("Save
  //  Fingerprint"), "untitled.eps", tr("Postscript (*.eps);;Scalable Vector
  //  Graphics (*.svg)"));
  if (settings::readSetting(settings::keys::ALLOW_CSV_FINGERPRINT_EXPORT)
          .toBool()) {
    filename = QFileDialog::getSaveFileName(
        nullptr, tr("Save Fingerprint"), "untitled.eps",
        tr("Encapsulated Postscript (*.eps);; Portable Network Graphics "
           "(*.png);; Comma Separated Values (*.csv)"));
  } else {
    filename = QFileDialog::getSaveFileName(
        nullptr, tr("Save Fingerprint"), "untitled.eps",
        tr("Encapsulated Postscript (*.eps);; Portable Network Graphics "
           "(*.png)"));
  }
  if (!filename.isEmpty()) {
    emit saveFingerprint(filename);
  }
}

void FingerprintOptions::updateSurfaceAreaProgressBar(double percentage) {
  surfaceAreaProgressBar->setValue((int)ceil(percentage - 0.5));
  surfaceAreaLabel->setText(QString::number(percentage, 'f', 1) + " %");
}
