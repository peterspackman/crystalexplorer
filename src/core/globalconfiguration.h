#pragma once
#include "isosurface_parameters.h"
#include "colormap.h"
#include <QMutex>
#include <QMutexLocker>

class GlobalConfiguration {
private:
  static GlobalConfiguration *instance;
  static QMutex mutex;

  QMap<QString, isosurface::SurfacePropertyDescription>
      surfacePropertyDescriptions;
  QMap<QString, isosurface::SurfaceDescription> surfaceDescriptions;
  QMap<QString, double> surfaceResolutionLevels;
  QMap<QString, ColorMapDescription> colorMapDescriptions;

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

  const QMap<QString, isosurface::SurfacePropertyDescription> &
  getPropertyDescriptions() const;
  const QMap<QString, isosurface::SurfaceDescription> &
  getSurfaceDescriptions() const;
  const QMap<QString, double> &getSurfaceResolutionLevels() const;
  QString getColorMapNameForProperty(const QString &) const;

  const QMap<QString, ColorMapDescription> getColorMapDescriptions() const;
};
