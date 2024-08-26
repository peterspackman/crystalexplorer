#pragma once
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaProperty>
#include <QObject>

class Serializable : public QObject {
  Q_OBJECT

public:
  explicit Serializable(QObject* parent = nullptr) : QObject(parent) {}
  virtual QJsonObject serialize() const;
  virtual void deserialize(const QJsonObject &json);

  bool saveToFile(const QString &filename) const;
  bool loadFromFile(const QString &filename);

protected:
  static QMap<QString, std::function<Serializable *()>> objectFactory;

  static Serializable *createObject(const QString &className) {
    auto it = objectFactory.find(className);
    if (it != objectFactory.end()) {
      return it.value()();
    }
    return nullptr;
  }

public:
  template <typename T> static void registerClass(const QString &className) {
    objectFactory[className] = []() -> Serializable * { return new T(); };
  }
};
