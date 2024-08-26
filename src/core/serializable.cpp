#include "serializable.h"

QJsonObject Serializable::serialize() const {
  QJsonObject json;
  const QMetaObject *metaObject = this->metaObject();

  for (int i = 0; i < metaObject->propertyCount(); ++i) {
    QMetaProperty property = metaObject->property(i);
    if (property.isStored()) {
      json[property.name()] = QJsonValue::fromVariant(property.read(this));
    }
  }

  QJsonArray childrenArray;
  for (const QObject *child : children()) {
    if (const Serializable *serializableChild =
            qobject_cast<const Serializable *>(child)) {
      childrenArray.append(serializableChild->serialize());
    }
  }

  if (!childrenArray.isEmpty()) {
    json["children"] = childrenArray;
  }

  json["class"] = metaObject->className();
  return json;
}

void Serializable::deserialize(const QJsonObject &json) {
  const QMetaObject *metaObject = this->metaObject();

  for (int i = 0; i < metaObject->propertyCount(); ++i) {
    QMetaProperty property = metaObject->property(i);
    if (property.isStored() && json.contains(property.name())) {
      property.write(this, json[property.name()].toVariant());
    }
  }

  if (json.contains("children")) {
    QJsonArray childrenArray = json["children"].toArray();
    for (const QJsonValue &childValue : childrenArray) {
      QJsonObject childObject = childValue.toObject();
      QString className = childObject["class"].toString();

      // You'd need a factory method to create objects based on class names
      Serializable *child = createObject(className);
      if (child) {
        child->setParent(this);
        child->deserialize(childObject);
      }
    }
  }
}

bool Serializable::saveToFile(const QString &filename) const {
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }

  QJsonDocument doc(serialize());
  file.write(doc.toJson());
  return true;
}

bool Serializable::loadFromFile(const QString &filename) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }

  QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
  deserialize(doc.object());
  return true;
}

QMap<QString, std::function<Serializable*()>> Serializable::objectFactory;
