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
  else if (s == "esp" || s == "electric_potential")
    return Kind::ESP;
  else if (s == "rho" || s == "electron_density")
    return Kind::ElectronDensity;
  else if (s == "def" || s == "deformation_density" ||
           s == "Deformation Density")
    return Kind::DeformationDensity;
  else
    return Kind::Unknown;
}

QMap<QString, SurfacePropertyDescription>
loadPropertyDescriptions(const nlohmann::json &json) {
  QMap<QString, SurfacePropertyDescription> properties;
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
      properties.insert(s(item.key()), spd);
    } catch (nlohmann::json::exception &e) {
      qWarning() << "Failed to parse property" << s(item.key()) << ":"
                 << e.what();
    }
  }
  return properties;
}

QMap<QString, SurfaceDescription>
loadSurfaceDescriptions(const nlohmann::json &json) {
  QMap<QString, SurfaceDescription> surfaces;

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
      surfaces.insert(s(item.key()), sd);
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
    QMap<QString, SurfacePropertyDescription> &propertyDescriptions,
    QMap<QString, SurfaceDescription> &descriptions,
    QMap<QString, double> &resolutions) {
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

SurfaceDescription getSurfaceDescription(Kind kind) {
  QString s = kindToString(kind);
  const auto &descriptions =
      GlobalConfiguration::getInstance()->getSurfaceDescriptions();
  auto loc = descriptions.find(s);
  if (loc != descriptions.end())
    return *loc;
  return {};
}

QString getDisplayName(QString s) {
  const auto &descriptions =
      GlobalConfiguration::getInstance()->getSurfaceDescriptions();
  auto loc = descriptions.find(s);
  if (loc != descriptions.end())
    return (*loc).displayName;
  return s;
}

} // namespace isosurface
