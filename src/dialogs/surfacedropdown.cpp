#include "surfacedropdown.h"

SurfaceTypeDropdown::SurfaceTypeDropdown(QWidget *parent) : QComboBox(parent) {
  connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &SurfaceTypeDropdown::onCurrentIndexChanged);
}

QString SurfaceTypeDropdown::current() const {
  return currentData().toString();
}

void SurfaceTypeDropdown::setCurrent(QString val) {

  int index = findData(val);
  if (index != -1) {
    setCurrentIndex(index);
  } else {
    setCurrentIndex(-1);
  }
}

isosurface::SurfaceDescription
SurfaceTypeDropdown::currentSurfaceDescription() const {
  return m_surfaceDescriptions.value(current());
}

void SurfaceTypeDropdown::setDescriptions(
    const SurfaceDescriptionMap &surfaceDescriptions) {
  m_surfaceDescriptions = surfaceDescriptions;

  clear();
  for (auto it = m_surfaceDescriptions.constBegin();
       it != m_surfaceDescriptions.constEnd(); ++it) {
    addItem(it.value().displayName, it.key());
  }
}

void SurfaceTypeDropdown::onCurrentIndexChanged(int index) {
  QString key = itemData(index).toString();
  emit selectionChanged(key);
  emit descriptionChanged(itemText(index));
}

SurfacePropertyTypeDropdown::SurfacePropertyTypeDropdown(QWidget *parent)
    : QComboBox(parent) {
  connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &SurfacePropertyTypeDropdown::onCurrentIndexChanged);
}

QString SurfacePropertyTypeDropdown::current() const {
  return currentData().toString();
}

isosurface::SurfacePropertyDescription
SurfacePropertyTypeDropdown::currentSurfacePropertyDescription() const {
  return m_surfacePropertyDescriptions.value(current());
}

void SurfacePropertyTypeDropdown::setDescriptions(
    const SurfaceDescriptionMap &surfaceDescriptions,
    const SurfacePropertyDescriptionMap &surfacePropertyDescriptions) {
  m_surfaceDescriptions = surfaceDescriptions;
  m_surfacePropertyDescriptions = surfacePropertyDescriptions;
}

void SurfacePropertyTypeDropdown::onCurrentIndexChanged(int index) {
  emit selectionChanged(itemData(index).toString());
}

void SurfacePropertyTypeDropdown::onSurfaceSelectionChanged(
    QString selectedSurfaceKey) {
  clear();

  isosurface::SurfaceDescription selectedSurface =
      getSurfaceDescription(selectedSurfaceKey);

  addItem("None", "None");
  for (const QString &property : selectedSurface.requestableProperties) {
    isosurface::SurfacePropertyDescription propertyDescription =
        getSurfacePropertyDescription(property);
    addItem(propertyDescription.displayName, property);
  }
}

isosurface::SurfaceDescription
SurfacePropertyTypeDropdown::getSurfaceDescription(
    const QString &surfaceKey) const {
  return m_surfaceDescriptions.value(surfaceKey);
}

isosurface::SurfacePropertyDescription
SurfacePropertyTypeDropdown::getSurfacePropertyDescription(
    const QString &propertyKey) const {
  return m_surfacePropertyDescriptions.value(propertyKey);
}

/// Resolution Combo box

ResolutionDropdown::ResolutionDropdown(QWidget *parent) : QComboBox(parent) {
  populateDropdown();
  connect(this, QOverload<int>::of(&ResolutionDropdown::currentIndexChanged),
          this, &ResolutionDropdown::onCurrentIndexChanged);
}

isosurface::Resolution ResolutionDropdown::currentLevel() const {
  return m_resolutionLevel;
}

float ResolutionDropdown::currentResolutionValue() const {
  return isosurface::resolutionValue(m_resolutionLevel);
}

void ResolutionDropdown::onCurrentIndexChanged(int index) {
  m_resolutionLevel =
      static_cast<isosurface::Resolution>(currentData().value<int>());
}

void ResolutionDropdown::populateDropdown() {
  blockSignals(true);
  clear();
  const std::vector<isosurface::Resolution> levels{
      isosurface::Resolution::VeryLow,  isosurface::Resolution::Low,
      isosurface::Resolution::Medium,   isosurface::Resolution::High,
      isosurface::Resolution::VeryHigh, isosurface::Resolution::Absurd,
  };
  for (const auto &l : levels) {
    addItem(isosurface::resolutionToString(l), static_cast<int>(l));
  }
  blockSignals(false);
}
