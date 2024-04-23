#include "globalconfiguration.h"

// Initialize static membes
GlobalConfiguration* GlobalConfiguration::instance = nullptr;
QMutex GlobalConfiguration::mutex;


GlobalConfiguration* GlobalConfiguration::getInstance() {
    QMutexLocker locker(&mutex);
    if (!instance) {
	instance = new GlobalConfiguration();
    }
    return instance;
}


bool GlobalConfiguration::load() {
    bool success = true;
    success = isosurface::loadSurfaceDescriptionConfiguration(
	surfacePropertyDescriptions,
	surfaceDescriptions,
	surfaceResolutionLevels
    );
    if(!success)  {
	qWarning() << "Unable to load surface descriptions from file";
	return false;
    }
    return success;
}

const QMap<QString, isosurface::SurfacePropertyDescription>& 
GlobalConfiguration::getPropertyDescriptions() const { 
    return surfacePropertyDescriptions;
}

const QMap<QString, isosurface::SurfaceDescription>& 
GlobalConfiguration::getSurfaceDescriptions() const {
    return surfaceDescriptions;
}

const QMap<QString, double>& GlobalConfiguration::getSurfaceResolutionLevels() const {
    return surfaceResolutionLevels;
}


