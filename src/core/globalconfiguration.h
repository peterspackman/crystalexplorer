#pragma once
#include "colormap.h"
#include "isosurface_parameters.h"
#include <QMutex>
#include <QMutexLocker>

class GlobalConfiguration {
private:
  static GlobalConfiguration *instance;
  static QMutex mutex;

  isosurface::SurfacePropertyDescriptions surfacePropertyDescriptions;
  isosurface::SurfaceDescriptions surfaceDescriptions;
  QMap<QString, double> surfaceResolutionLevels;
  QMap<QString, ColorMapDescription> colorMapDescriptions;
  bool m_haveData{false};

protected:
  GlobalConfiguration() {} // Constructor must be protected or private
  ~GlobalConfiguration() {}

public:
  GlobalConfiguration(GlobalConfiguration &other) =
      delete; // Disable copy constructor
  void operator=(const GlobalConfiguration &) =
      delete; // Disable assignment operator

  static GlobalConfiguration *getInstance();

  bool load();

  const isosurface::SurfacePropertyDescriptions &
  getPropertyDescriptions() const;
  const isosurface::SurfaceDescriptions &getSurfaceDescriptions() const;
  const QMap<QString, double> &getSurfaceResolutionLevels() const;
  QString getColorMapNameForProperty(const QString &) const;

  const QMap<QString, ColorMapDescription> getColorMapDescriptions() const;
};
