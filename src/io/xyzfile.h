#pragma once

#include <QDebug>
#include <QStringList>
#include <QVector3D>
#include <occ/core/linear_algebra.h>

class XYZFile {
public:
  XYZFile() = default;
  bool readFromFile(const QString &fileName);
  bool readFromString(const QString &content);
  const std::vector<QString> &getAtomSymbols() const;
  const std::vector<occ::Vec3> &getAtomPositions() const;

private:
  std::vector<QString> m_atomSymbols;
  std::vector<occ::Vec3> m_atomPositions;
};
