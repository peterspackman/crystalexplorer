#pragma once
#include <QColor>
#include <QString>
#include <nlohmann/json.hpp>

inline void to_json(nlohmann::json &j, const QString &s) { j = s.toStdString(); }
inline void from_json(const nlohmann::json &j, QString &s) {
  s = QString::fromStdString(j.get<std::string>());
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
