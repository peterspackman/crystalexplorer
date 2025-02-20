#pragma once
#include <QDebug>
#include <chrono>
#include <string>

class ScopedTimer {
public:
  explicit ScopedTimer(const QString &name)
      : m_name(name), m_start(std::chrono::high_resolution_clock::now()) {}

  ~ScopedTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start);
    qDebug() << m_name << "took" << duration.count() << "ms";
  }

private:
  QString m_name;
  std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};
