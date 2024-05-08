#pragma once

#include <QDebug>
#include <QStringList>
#include <QVector3D>
#include <occ/core/linear_algebra.h>

class XYZFile {
public:
  XYZFile() = default;
  XYZFile(const std::vector<QString> &atomSymbols, Eigen::Ref<const occ::Mat3N> atomPositions);

  bool readFromFile(const QString &fileName);
  bool readFromString(const QString &content);

  bool writeToFile(const QString &filename) const;
  QString toString() const;

  const QString &getComment() const;
  void setComment(const QString &);

  const std::vector<QString> &getAtomSymbols() const;
  void setAtomSymbols(const std::vector<QString> &);
  void setElements(Eigen::Ref<const occ::IVec>);

  const std::vector<occ::Vec3> &getAtomPositions() const;
  void setAtomPositions(const std::vector<occ::Vec3> &);
  void setAtomPositions(Eigen::Ref<const occ::Mat3N>);

private:
  std::vector<QString> m_atomSymbols;
  QString m_comment;
  std::vector<occ::Vec3> m_atomPositions;
};
