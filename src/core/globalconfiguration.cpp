#include "globalconfiguration.h"

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
  if (m_haveData)
    return m_haveData;
  qDebug() << "Loading surface descriptions";
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
  m_haveData = surfaceSuccess && colorMapSuccess;
  return m_haveData;
}

const isosurface::SurfacePropertyDescriptions &
GlobalConfiguration::getPropertyDescriptions() const {
  return surfacePropertyDescriptions;
}

const isosurface::SurfaceDescriptions &
GlobalConfiguration::getSurfaceDescriptions() const {
  return surfaceDescriptions;
}

const QMap<QString, double> &
GlobalConfiguration::getSurfaceResolutionLevels() const {
  return surfaceResolutionLevels;
}

const QMap<QString, ColorMapDescription>
GlobalConfiguration::getColorMapDescriptions() const {
  return colorMapDescriptions;
}

QString GlobalConfiguration::getColorMapNameForProperty(
    const QString &propertyName) const {
  return surfacePropertyDescriptions.get(propertyName).cmap;
}
