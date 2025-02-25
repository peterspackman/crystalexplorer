#pragma once
#include <QColor>
#include <QStringList>
#include <QByteArray>
#include <nlohmann/json.hpp>

inline void to_json(nlohmann::json &j, const QString &s) {
  j = s.toStdString();
}
inline void from_json(const nlohmann::json &j, QString &s) {
  s = QString::fromStdString(j.get<std::string>());
}

inline void to_json(nlohmann::json &j, const QStringList &s) {
  std::vector<QString> v(s.begin(), s.end());
  j = v;
}

inline void from_json(const nlohmann::json &j, QStringList &s) {
  std::vector<QString> v = j.get<std::vector<QString>>();
  s = QStringList(v.begin(), v.end());
}

inline void to_json(nlohmann::json &j, const QColor &color) {
  j = nlohmann::json{{"r", color.red()},
                     {"g", color.green()},
                     {"b", color.blue()},
                     {"a", color.alpha()}};
}

inline void from_json(const nlohmann::json &j, QColor &color) {
  color.setRgb(j["r"].get<int>(), j["g"].get<int>(), j["b"].get<int>(),
               j["a"].get<int>());
}

namespace nlohmann {
template <> struct adl_serializer<QByteArray> {
  static void from_json(const json &j, QByteArray &qba) {
    std::string str = j.get<std::string>();
    qba = QByteArray::fromBase64(QByteArray::fromStdString(str));
  }

  static void to_json(json &j, const QByteArray &qba) {
    j = qba.toBase64().toStdString();
  }
};
} // namespace nlohmann
