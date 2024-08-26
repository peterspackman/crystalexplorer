#include <QMutex>
#include <QMutexLocker>
#include "isosurface_parameters.h"

class GlobalConfiguration {
private:
    static GlobalConfiguration * instance;
    static QMutex mutex;

    QMap<QString, isosurface::SurfacePropertyDescription> surfacePropertyDescriptions;
    QMap<QString, isosurface::SurfaceDescription> surfaceDescriptions;
    QMap<QString, double> surfaceResolutionLevels;

protected:
    GlobalConfiguration() {} // Constructor must be protected or private
    ~GlobalConfiguration() {}

public:
    GlobalConfiguration(GlobalConfiguration &other) = delete; // Disable copy constructor
    void operator=(const GlobalConfiguration&) = delete; // Disable assignment operator

    static GlobalConfiguration* getInstance();

    bool load();

    const QMap<QString, isosurface::SurfacePropertyDescription>& getPropertyDescriptions() const;
    const QMap<QString, isosurface::SurfaceDescription>& getSurfaceDescriptions() const;
    const QMap<QString, double> & getSurfaceResolutionLevels() const;
};
