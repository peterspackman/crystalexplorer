#include "isosurface_parameters.h"
#include "globalconfiguration.h"
#include <QDebug>
#include <QFile>
#include <nlohmann/json.hpp>

void to_json(nlohmann::json &j,
             const isosurface::SurfacePropertyDescription &f) {
  j = {{"cmap", f.cmap},
       {"occName", f.occName},
       {"displayName", f.displayName},
       {"units", f.needsWavefunction},
       {"needsOrbital", f.needsOrbital},
       {"description", f.description}};
}

void from_json(const nlohmann::json &j,
               isosurface::SurfacePropertyDescription &f) {
  j.at("cmap").get_to(f.cmap);
  j.at("occName").get_to(f.occName);
  j.at("displayName").get_to(f.displayName);
  j.at("description").get_to(f.description);

  if (j.contains("units")) {
    j.at("units").get_to(f.units);
  }
  if (j.contains("icon")) {
    j.at("icon").get_to(f.iconName);
  }
  if (j.contains("needsOrbital")) {
    j.at("needsOrbital").get_to(f.needsOrbital);
  }
  if (j.contains("needsWavefunction")) {
    j.at("needsWavefunction").get_to(f.needsWavefunction);
  }
}

void to_json(nlohmann::json &j, const isosurface::SurfaceDescription &f) {
  j = {{"displayName", f.displayName},
       {"occName", f.occName},
       {"defaultIsovalue", f.defaultIsovalue},
       {"needsIsovalue", f.needsIsovalue},
       {"needsWavefunction", f.needsWavefunction},
       {"needsOrbital", f.needsOrbital},
       {"needsCluster", f.needsCluster},
       {"periodic", f.periodic},
       {"units", f.units},
       {"requestableProperties", f.requestableProperties},
       {"computeNegativeIsovalue", f.computeNegativeIsovalue}};
}

void from_json(const nlohmann::json &j, isosurface::SurfaceDescription &s) {
  j.at("displayName").get_to(s.displayName);
  j.at("occName").get_to(s.occName);
  j.at("description").get_to(s.description);
  if (j.contains("needsIsovalue")) {
    j.at("needsIsovalue").get_to(s.needsIsovalue);
  }
  if (j.contains("icon")) {
    j.at("icon").get_to(s.iconName);
  }
  if (j.contains("defaultIsovalue")) {
    j.at("defaultIsovalue").get_to(s.defaultIsovalue);
  }
  if (j.contains("needsWavefunction")) {
    j.at("needsWavefunction").get_to(s.needsWavefunction);
  }
  if (j.contains("needsOrbital")) {
    j.at("needsOrbital").get_to(s.needsOrbital);
  }
  if (j.contains("needsCluster")) {
    j.at("needsCluster").get_to(s.needsCluster);
  }
  if (j.contains("periodic")) {
    j.at("periodic").get_to(s.periodic);
  }
  if (j.contains("units")) {
    j.at("units").get_to(s.units);
  }
  if (j.contains("requestableProperties")) {
    j.at("requestableProperties").get_to(s.requestableProperties);
  }
  if (j.contains("computeNegativeIsovalue")) {
    j.at("computeNegativeIsovalue").get_to(s.computeNegativeIsovalue);
  }
}

void to_json(nlohmann::json &j, const isosurface::Resolution &res) {
  j = isosurface::resolutionToString(res);
}

void from_json(const nlohmann::json &j, isosurface::Resolution &res) {
  res = isosurface::stringToResolution(
      QString::fromStdString(j.get<std::string>()));
}

void to_json(nlohmann::json &j, const isosurface::Kind &kind) {
  j = isosurface::kindToString(kind).toStdString();
}

void from_json(const nlohmann::json &j, isosurface::Kind &kind) {
  kind = isosurface::stringToKind(QString::fromStdString(j.get<std::string>()));
}

void to_json(nlohmann::json &j, const isosurface::OrbitalDetails &details) {
  j = {{"label", details.label.toStdString()},
       {"index", details.index},
       {"occupied", details.occupied}};
}

void from_json(const nlohmann::json &j, isosurface::OrbitalDetails &details) {
  details.label = QString::fromStdString(j.at("label").get<std::string>());
  details.index = j.at("index").get<int>();
  details.occupied = j.at("occupied").get<bool>();
}

namespace isosurface {

QString kindToString(Kind kind) {
  switch (kind) {
  case Kind::Promolecule:
    return "promolecule_density";
  case Kind::Hirshfeld:
    return "hirshfeld";
  case Kind::Void:
    return "void";
  case Kind::ESP:
    return "esp";
  case Kind::ElectronDensity:
    return "electron_density";
  case Kind::DeformationDensity:
    return "deformation_density";
  case Kind::Orbital:
    return "orbital";
  default:
    return "unknown";
  };
}

QString defaultPropertyForKind(Kind kind) {
  switch (kind) {
  case Kind::Promolecule:
    return "dnorm";
  case Kind::Hirshfeld:
    return "dnorm";
  case Kind::Void:
    return "None";
  case Kind::ESP:
    return "None";
  case Kind::ElectronDensity:
    return "dnorm";
  case Kind::DeformationDensity:
    return "None";
  case Kind::Orbital:
    return "Isovalue";
  default:
    return "unknown";
  };
}

Kind stringToKind(const QString &s) {
  qDebug() << "stringToKind called with:" << s;
  if (s == "promolecule" || s == "Promolecule Density" ||
      s == "promolecule_density")
    return Kind::Promolecule;
  else if (s == "hirshfeld" || s == "Hirshfeld")
    return Kind::Hirshfeld;
  else if (s == "void" || s == "Void" || s == "Crystal Voids" ||
           s == "crystal_void" || s == "Crystal Void")
    return Kind::Void;
  else if (s == "esp" || s == "electric_potential" ||
           s == "Electric Potential" || s == "Electrostatic Potential")
    return Kind::ESP;
  else if (s == "rho" || s == "electron_density" || s == "Electron Density")
    return Kind::ElectronDensity;
  else if (s == "def" || s == "deformation_density" ||
           s == "Deformation Density")
    return Kind::DeformationDensity;
  else if (s == "mo" || s == "orbital" || s == "Orbital")
    return Kind::Orbital;
  else
    return Kind::Unknown;
}

inline SurfacePropertyDescriptions
loadPropertyDescriptions(const nlohmann::json &json) {
  SurfacePropertyDescriptions properties;
  auto s = [](const std::string &str) { return QString::fromStdString(str); };
  qDebug() << "Load property descriptions";

  if (!json.contains("properties") || !json["properties"].is_object()) {
    qWarning() << "JSON does not contain a 'properties' object";
    return properties;
  }

  for (const auto &item : json["properties"].items()) {
    try {
      SurfacePropertyDescription spd;
      from_json(item.value(), spd);
      properties.descriptions.insert(spd.displayName, spd);
      // allow referring by the occName or displayName as well
      properties.displayNameLookup.insert(s(item.key()), spd.displayName);
      properties.displayNameLookup.insert(spd.occName, spd.displayName);
      properties.displayNameLookup.insert(spd.displayName, spd.displayName);
    } catch (nlohmann::json::exception &e) {
      qWarning() << "Failed to parse property" << s(item.key()) << ":"
                 << e.what();
    }
  }
  return properties;
}

inline SurfaceDescriptions loadSurfaceDescriptions(const nlohmann::json &json) {
  SurfaceDescriptions surfaces;

  if (!json.contains("surfaces") || !json["surfaces"].is_object()) {
    qWarning() << "JSON does not contain a 'surfaces' object";
    return surfaces;
  }
  qDebug() << "Load surface descriptions";
  auto s = [](const std::string &str) { return QString::fromStdString(str); };

  for (const auto &item : json["surfaces"].items()) {
    try {
      SurfaceDescription sd;
      from_json(item.value(), sd);
      surfaces.descriptions.insert(sd.displayName, sd);

      surfaces.displayNameLookup.insert(s(item.key()), sd.displayName);
      surfaces.displayNameLookup.insert(sd.occName, sd.displayName);
      surfaces.displayNameLookup.insert(sd.displayName, sd.displayName);
    } catch (nlohmann::json::exception &e) {
      qWarning() << "Failed to parse surface" << s(item.key()) << ":"
                 << e.what();
    }
  }
  return surfaces;
}

QMap<QString, double> loadResolutionLevels(const nlohmann::json &json) {
  QMap<QString, double> resolutions;
  auto s = [](const std::string &str) { return QString::fromStdString(str); };

  if (!json.contains("resolutionLevels") ||
      !json["resolutionLevels"].is_object()) {
    qWarning() << "JSON does not contain a 'resolutions' object";
    return resolutions;
  }

  for (const auto &item : json["resolutionLevels"].items()) {
    try {
      double value = item.value().get<double>();
      resolutions.insert(s(item.key()), value);
    } catch (nlohmann::json::exception &e) {
      qWarning() << "Failed to parse resolution" << s(item.key()) << ":"
                 << e.what();
    }
  }
  return resolutions;
}

bool loadSurfaceDescriptionConfiguration(
    SurfacePropertyDescriptions &propertyDescriptions,
    SurfaceDescriptions &descriptions, QMap<QString, double> &resolutions) {
  QFile file(":/resources/surface_description.json");
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning("Couldn't open config file.");
    return false;
  }
  QByteArray data = file.readAll();

  nlohmann::json doc;
  try {
    doc = nlohmann::json::parse(data.constData());
  } catch (nlohmann::json::parse_error &e) {
    qWarning() << "JSON parse error:" << e.what();
    return false;
  }

  propertyDescriptions = loadPropertyDescriptions(doc);
  descriptions = loadSurfaceDescriptions(doc);
  resolutions = loadResolutionLevels(doc);

  return true;
}

SurfaceDescription SurfaceDescriptions::get(const QString &s) const {
  auto loc = displayNameLookup.find(s);
  if (loc != displayNameLookup.end())
    return descriptions.value(*loc);
  return {};
}

SurfacePropertyDescription
SurfacePropertyDescriptions::get(const QString &s) const {
  auto loc = displayNameLookup.find(s);
  if (loc != displayNameLookup.end())
    return descriptions.value(*loc);
  return {};
}

SurfaceDescription getSurfaceDescription(Kind kind) {
  QString s = kindToString(kind);
  const auto &descriptions =
      GlobalConfiguration::getInstance()->getSurfaceDescriptions();
  return descriptions.get(s);
}

QString getSurfaceDisplayName(QString s) {
  const auto &descriptions =
      GlobalConfiguration::getInstance()->getSurfaceDescriptions();
  return descriptions.get(s).displayName;
}

QString getSurfacePropertyDisplayName(QString s) {
  const auto &descriptions =
      GlobalConfiguration::getInstance()->getPropertyDescriptions();
  return descriptions.get(s).displayName;
}

QString generateSurfaceName(const Parameters &parameters,
                            const QString &fragmentIdentifier) {
  SurfaceDescription desc = getSurfaceDescription(parameters.kind);
  QString surfaceType = desc.displayName;

  // Use the fragment identifier from parameters, or the override
  QString fragId = fragmentIdentifier.isEmpty() ? parameters.fragmentIdentifier
                                                : fragmentIdentifier;

  // Default fallback
  if (fragId.isEmpty()) {
    fragId = "Fragment";
  }

  // Build the name: "Kind FragmentID (resolution) [params]"
  QString result = QString("%1 %2 (%3)")
                       .arg(surfaceType)
                       .arg(fragId)
                       .arg(parameters.separation);

  // Add technical parameters in brackets
  QStringList params;
  if (desc.needsIsovalue && parameters.isovalue != desc.defaultIsovalue) {
    params << QString("iso=%1").arg(parameters.isovalue);
  }
  if (parameters.computeNegativeIsovalue) {
    params << "±";
  }

  if (!params.isEmpty()) {
    result += QString(" [%1]").arg(params.join(", "));
  }

  return result;
}

} // namespace isosurface
