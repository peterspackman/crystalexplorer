#include "surfacedropdown.h"

/// Surface
SurfaceTypeDropdown::SurfaceTypeDropdown(QWidget *parent) : QComboBox(parent) {
  connect(this, QOverload<int>::of(&SurfaceTypeDropdown::currentIndexChanged), this, &SurfaceTypeDropdown::onCurrentIndexChanged);
  populateDropdown();
}

IsosurfaceDetails::Attributes
SurfaceTypeDropdown::currentSurfaceAttributes() const {
  return IsosurfaceDetails::getAttributes(m_selectedType);
}

void SurfaceTypeDropdown::onCurrentIndexChanged(int index) {
  m_selectedType =
      static_cast<IsosurfaceDetails::Type>(currentData().value<int>());
  qDebug() << "Emitting surface type changed" << static_cast<int>(m_selectedType);
  emit surfaceTypeChanged(m_selectedType);
}

void SurfaceTypeDropdown::populateDropdown() {
  const auto &types = IsosurfaceDetails::getAvailableTypes();
  for (auto kv = types.constKeyValueBegin(); kv != types.constKeyValueEnd();
       kv++) {
    addItem(kv->second.label, static_cast<int>(kv->first));
    if(kv->first == IsosurfaceDetails::defaultType()) {
        qDebug() << "Setting current index to " << count() - 1;
        setCurrentIndex(count() - 1);
    }
  }
}


/// Properties

SurfacePropertyTypeDropdown::SurfacePropertyTypeDropdown(QWidget *parent)
    : QComboBox(parent) {
    connect(this, QOverload<int>::of(&SurfacePropertyTypeDropdown::currentIndexChanged), this, &SurfacePropertyTypeDropdown::onCurrentIndexChanged);
}

void SurfacePropertyTypeDropdown::onCurrentIndexChanged(int index) {
  m_selectedType =
      static_cast<IsosurfacePropertyDetails::Type>(currentData().value<int>());
}

void SurfacePropertyTypeDropdown::onSurfaceTypeChanged(
    IsosurfaceDetails::Type selectedSurfaceType) {
  clear(); // Clear current items
  auto properties =
      IsosurfaceDetails::getRequestableProperties(selectedSurfaceType);

  qDebug() << "Surface type changed" << static_cast<int>(selectedSurfaceType) << "has " <<  properties.size();
  for (const auto &property : std::as_const(properties)) {
    const auto &attributes = IsosurfacePropertyDetails::getAttributes(property);
    addItem(attributes.name, static_cast<int>(property));
  }
}

// Function to get the currently selected IsosurfacePropertyDetails::Attributes
IsosurfacePropertyDetails::Attributes
SurfacePropertyTypeDropdown::currentSurfacePropertyAttributes() const {
  auto prop =
      static_cast<IsosurfacePropertyDetails::Type>(currentData().value<int>());
  return IsosurfacePropertyDetails::getAttributes(prop);
}



/// Resolution Combo box

ResolutionDropdown::ResolutionDropdown(QWidget *parent) : QComboBox(parent) {
  populateDropdown();
  connect(this, QOverload<int>::of(&ResolutionDropdown::currentIndexChanged), this, &ResolutionDropdown::onCurrentIndexChanged);
}


ResolutionDetails::Level ResolutionDropdown::currentLevel() const {
    return m_level;
}

float ResolutionDropdown::currentResolutionValue() const {
    return ResolutionDetails::value(m_level);
}

void ResolutionDropdown::onCurrentIndexChanged(int index) {
    m_level = static_cast<ResolutionDetails::Level>(currentData().value<int>());
}


void ResolutionDropdown::populateDropdown() {
    blockSignals(true);
    clear();
    for(const auto &l: ResolutionDetails::getLevels()) {
        addItem(ResolutionDetails::name(l), static_cast<int>(l));
    }
    blockSignals(false);
}
