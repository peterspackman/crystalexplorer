#pragma once
#include "json.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>

struct PublicationReference {
  QString key; // BibTeX key e.g. "Spackman2021"
  QString title;
  QStringList authors; // List of author names
  QString journal;
  QString year;
  QString volume;
  QString issue;
  QString pages;
  QString doi;
  QString url;
  QString type; // article, book, etc.

  // Generate formatted citation text
  QString formatCitation() const {
    QString citation;

    // Authors (abbreviated if more than 3)
    if (!authors.isEmpty()) {
      if (authors.size() <= 3) {
        citation = authors.join(", ");
      } else {
        citation = QString("%1 et al.").arg(authors.first());
      }
      citation += " ";
    }

    // Year
    if (!year.isEmpty()) {
      citation += QString("(%1) ").arg(year);
    }

    // Title
    if (!title.isEmpty()) {
      citation += QString("%1. ").arg(title);
    }

    // Journal
    if (!journal.isEmpty()) {
      citation += QString("<i>%1</i>").arg(journal);
      if (!volume.isEmpty()) {
        citation += QString(" <b>%1</b>").arg(volume);
      }
      if (!issue.isEmpty()) {
        citation += QString("(%1)").arg(issue);
      }
      if (!pages.isEmpty()) {
        citation += QString(", %1").arg(pages);
      }
      citation += ".";
    }

    return citation;
  }

  // Generate short citation (e.g. "Spackman et al. (2021)")
  QString formatShortCitation() const {
    QString citation;
    if (!authors.isEmpty()) {
      if (authors.size() == 1) {
        // Extract last name from first author
        QString firstName = authors.first();
        QStringList parts = firstName.split(",");
        citation = parts.first().trimmed();
      } else if (authors.size() == 2) {
        QStringList lastNames;
        for (const auto &author : authors) {
          QStringList parts = author.split(",");
          lastNames << parts.first().trimmed();
        }
        citation = lastNames.join(" & ");
      } else {
        // More than 2 authors
        QStringList parts = authors.first().split(",");
        citation = QString("%1 et al.").arg(parts.first().trimmed());
      }
    }

    if (!year.isEmpty()) {
      citation += QString(" (%1)").arg(year);
    }

    return citation;
  }

  // Convert to/from JSON
  QJsonObject toJson() const {
    QJsonObject obj;
    obj["key"] = key;
    obj["title"] = title;
    obj["journal"] = journal;
    obj["year"] = year;
    obj["volume"] = volume;
    obj["issue"] = issue;
    obj["pages"] = pages;
    obj["doi"] = doi;
    obj["url"] = url;
    obj["type"] = type;

    QJsonArray authorsArray;
    for (const auto &author : authors) {
      authorsArray.append(author);
    }
    obj["authors"] = authorsArray;

    return obj;
  }

  static PublicationReference fromJson(const QJsonObject &obj) {
    PublicationReference ref;
    ref.key = obj["key"].toString();
    ref.title = obj["title"].toString();
    ref.journal = obj["journal"].toString();
    ref.year = obj["year"].toString();
    ref.volume = obj["volume"].toString();
    ref.issue = obj["issue"].toString();
    ref.pages = obj["pages"].toString();
    ref.doi = obj["doi"].toString();
    ref.url = obj["url"].toString();
    ref.type = obj["type"].toString();

    QJsonArray authorsArray = obj["authors"].toArray();
    for (const auto &value : authorsArray) {
      ref.authors.append(value.toString());
    }

    return ref;
  }
};

// Manager class for loading and accessing references
class ReferenceManager {
public:
  static ReferenceManager &instance() {
    static ReferenceManager instance;
    return instance;
  }

  bool
  loadFromResource(const QString &resourcePath = ":/resources/references.json");
  bool loadFromFile(const QString &filePath);

  PublicationReference getReference(const QString &key) const {
    return m_references.value(key);
  }

  bool hasReference(const QString &key) const {
    return m_references.contains(key);
  }

  QList<QString> getAllKeys() const { return m_references.keys(); }

  // Get citations for different methods
  QStringList getCitationsForMethod(const QString &method) const;

private:
  ReferenceManager() = default;
  QMap<QString, PublicationReference> m_references;
};
