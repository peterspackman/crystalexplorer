#include "globalconfiguration.h"

// Initialize static membes
GlobalConfiguration *GlobalConfiguration::instance = nullptr;
QMutex GlobalConfiguration::mutex;

GlobalConfiguration *GlobalConfiguration::getInstance() {
  QMutexLocker locker(&mutex);
  if (!instance) {
    instance = new GlobalConfiguration();
  }
  return instance;
}

bool GlobalConfiguration::load() {
  bool surfaceSuccess = isosurface::loadSurfaceDescriptionConfiguration(
      surfacePropertyDescriptions, surfaceDescriptions,
      surfaceResolutionLevels);

  if (!surfaceSuccess) {
    qWarning() << "Unable to load surface descriptions from file";
  }
  bool colorMapSuccess = loadColorMapConfiguration(colorMapDescriptions);
  if (!colorMapSuccess) {
    qWarning() << "Unable to load surface descriptions from file";
  }
  return surfaceSuccess && colorMapSuccess;
}

const QMap<QString, isosurface::SurfacePropertyDescription> &
GlobalConfiguration::getPropertyDescriptions() const {
  return surfacePropertyDescriptions;
}

const QMap<QString, isosurface::SurfaceDescription> &
GlobalConfiguration::getSurfaceDescriptions() const {
  return surfaceDescriptions;
}

const QMap<QString, double> &
GlobalConfiguration::getSurfaceResolutionLevels() const {
  return surfaceResolutionLevels;
}


const QMap<QString, ColorMapDescription> GlobalConfiguration::getColorMapDescriptions() const {
  return colorMapDescriptions;
}

QString GlobalConfiguration::getColorMapNameForProperty(
    const QString &propertyName) const {
  auto it = surfacePropertyDescriptions.find(propertyName);
  if (it != surfacePropertyDescriptions.end()) {
    return it->cmap;
  }
  return "Viridis";
}
