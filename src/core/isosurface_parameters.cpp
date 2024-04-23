#include "isosurface_parameters.h"
#include "globalconfiguration.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

namespace isosurface {

QString kindToString(Kind kind) {
    switch(kind) {
	case Kind::Promolecule:
	    return "promolecule";
	case Kind::Hirshfeld:
	    return "hirshfeld";
	case Kind::Void:
	    return "void";
	case Kind::ESP:
	    return "esp";
	case Kind::ElectronDensity:
	    return "rho";
	case Kind::DeformationDensity:
	    return "def";
	default:
	    return "unknown";
    };
}

Kind stringToKind(const QString &s) {
    qDebug() << "stringToKind called with:" << s;
    if(s == "promolecule" || s == "Promolecule Density") return Kind::Promolecule;
    else if(s == "hirshfeld" || s == "Hirshfeld") return Kind::Hirshfeld;
    else if(s == "void" || s == "Void" || s == "Crystal Voids") return Kind::Void;
    else if(s == "esp") return Kind::ESP;
    else if(s == "rho") return Kind::ElectronDensity;
    else if(s == "def") return Kind::DeformationDensity;
    else return Kind::Unknown;
}


QMap<QString, SurfacePropertyDescription> loadPropertyDescriptions(const QJsonObject &json) {
    QMap<QString, SurfacePropertyDescription> properties;
    const QJsonObject &items = json["properties"].toObject();
    for (const QString &key : items.keys()) {
        QJsonObject obj = items[key].toObject();
        SurfacePropertyDescription property;
        property.cmap = obj["cmap"].toString();
        property.occName = obj["occName"].toString();
        property.displayName = obj["displayName"].toString();
        property.units = obj["units"].toString();
        property.needsWavefunction = obj["needsWavefunction"].toBool();
        property.needsIsovalue = obj["needsIsovalue"].toBool();
        property.needsOrbital = obj["needsOrbitalSelection"].toBool();
        property.description = obj["description"].toString();
        properties.insert(key, property);
    }
    return properties;
}

QMap<QString, SurfaceDescription> loadSurfaceDescriptions(const QJsonObject &json) {
    QMap<QString, SurfaceDescription> surfaces;
    const QJsonObject &items = json["surfaces"].toObject();
    for (const QString &key : items.keys()) {
        QJsonObject obj = items[key].toObject();
        SurfaceDescription surface;
        surface.displayName = obj["displayName"].toString();
        surface.occName = obj["occName"].toString();
        surface.defaultIsovalue = obj["defaultIsovalue"].toDouble();
        surface.needsIsovalue = obj["needsIsovalue"].toBool();
        surface.needsWavefunction = obj["needsWavefunction"].toBool();
        surface.needsOrbital = obj.contains("needsOrbital");
        surface.needsCluster = obj.contains("needsCluster");
        surface.periodic = obj.contains("periodic");
        surface.units = obj["units"].toString();
        surface.description = obj["description"].toString();
        QJsonArray reqProps = obj["requestableProperties"].toArray();
        for (const QJsonValue &val : reqProps) {
            surface.requestableProperties.append(val.toString());
        }
        surfaces.insert(key, surface);
    }
    return surfaces;
}

QMap<QString, double> loadResolutionLevels(const QJsonObject &json) {
    QMap<QString, double>  levels;
    const QJsonObject &items = json["resolutionLevels"].toObject();
    for (const QString &key : items.keys()) {
        levels.insert(key, items[key].toDouble());
    }
    return levels;
}



bool loadSurfaceDescriptionConfiguration(
    QMap<QString, SurfacePropertyDescription> &propertyDescriptions,
    QMap<QString, SurfaceDescription> &descriptions,
    QMap<QString, double> &resolutions) {

    QFile file(":/resources/surface_description.json");
    if (!file.open(QIODevice::ReadOnly)) {
	qWarning("Couldn't open config file.");
	return false;
    }
    QByteArray data = file.readAll();
    QJsonDocument doc(QJsonDocument::fromJson(data));

    propertyDescriptions = loadPropertyDescriptions(doc.object());
    descriptions = loadSurfaceDescriptions(doc.object());
    resolutions = loadResolutionLevels(doc.object());
    return true;
}


SurfaceDescription getSurfaceDescription(Kind kind) {
    QString s = kindToString(kind);
    const auto &descriptions = GlobalConfiguration::getInstance()->getSurfaceDescriptions();
    auto loc = descriptions.find(s);
    if(loc != descriptions.end()) return *loc;
    return {};
}

QString getDisplayName(QString s) {
    const auto &descriptions = GlobalConfiguration::getInstance()->getSurfaceDescriptions();
    auto loc = descriptions.find(s);
    if(loc != descriptions.end()) return (*loc).displayName;
    return s;
}

}
