#include "publication_reference.h"
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

bool ReferenceManager::loadFromResource(const QString &resourcePath) {
  QFile file(resourcePath);
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Failed to open references resource:" << resourcePath;
    return false;
  }

  QByteArray data = file.readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);

  if (!doc.isObject()) {
    qWarning() << "Invalid JSON format in references file";
    return false;
  }

  QJsonObject root = doc.object();
  QJsonArray references = root["references"].toArray();

  m_references.clear();
  for (const auto &value : references) {
    if (value.isObject()) {
      PublicationReference ref =
          PublicationReference::fromJson(value.toObject());
      m_references[ref.key] = ref;
    }
  }

  qDebug() << "Loaded" << m_references.size() << "references";
  return true;
}

bool ReferenceManager::loadFromFile(const QString &filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Failed to open references file:" << filePath;
    return false;
  }

  QByteArray data = file.readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);

  if (!doc.isObject()) {
    qWarning() << "Invalid JSON format in references file";
    return false;
  }

  QJsonObject root = doc.object();
  QJsonArray references = root["references"].toArray();

  m_references.clear();
  for (const auto &value : references) {
    if (value.isObject()) {
      PublicationReference ref =
          PublicationReference::fromJson(value.toObject());
      m_references[ref.key] = ref;
    }
  }

  qDebug() << "Loaded" << m_references.size() << "references from file";
  return true;
}

QStringList
ReferenceManager::getCitationsForMethod(const QString &method) const {
  QStringList citations;

  // Always cite CrystalExplorer
  if (hasReference("Spackman2021")) {
    citations << "Spackman2021";
  }

  // Method-specific citations
  if (method.contains("CE-1p", Qt::CaseInsensitive)) {
    if (hasReference("Spackman2023b")) {
      citations << "Spackman2023b";
    }
  } else if (method.contains("CE-HF", Qt::CaseInsensitive)) {
    if (hasReference("Mackenzie2017")) {
      citations << "Mackenzie2017";
    }
  } else if (method.contains("GFN2-xTB", Qt::CaseInsensitive)) {
    if (hasReference("Bannwarth2019")) {
      citations << "Bannwarth2019";
    }
  } else if (method.contains("GFN-xTB", Qt::CaseInsensitive) &&
             !method.contains("GFN2")) {
    if (hasReference("Grimme2017")) {
      citations << "Grimme2017";
    }
  } else if (method.contains("GFN-FF", Qt::CaseInsensitive)) {
    if (hasReference("Spicher2020")) {
      citations << "Spicher2020";
    }
  }

  return citations;
}
