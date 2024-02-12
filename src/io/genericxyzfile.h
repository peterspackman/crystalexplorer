#pragma once
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <ankerl/unordered_dense.h>
#include <Eigen/Dense>

// TODO support variant of float, int etc.
class GenericXYZFile {
public:
  using VectorType = Eigen::VectorXf;
  GenericXYZFile() = default;
  bool readFromFile(const QString &fileName);
  bool readFromString(const QString &content);

  [[nodiscard]] const VectorType &column(const QString &) const;
  [[nodiscard]] inline const auto &columns() const { return m_columns; }
  [[nodiscard]] QStringList columnNames() const;

  [[nodiscard]] QString getErrorString() const;

private:
  QString m_error;
  VectorType m_emptyColumn;
  ankerl::unordered_dense::map<QString, VectorType> m_columns;
};
