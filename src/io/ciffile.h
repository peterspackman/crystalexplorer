#pragma once

#include <QDebug>
#include <QStringList>
#include <QVector3D>
#include <occ/crystal/crystal.h>

using OccCrystal = occ::crystal::Crystal;

class CifFile {
public:
  CifFile() = default;
  bool readFromFile(const QString &fileName);
  bool readFromString(const QString &content);

  int numberOfCrystals() const;

  const OccCrystal &getCrystalStructure(int index = 0) const;
  const QByteArray& getCrystalCifContents(int index = 0) const;
  const QString& getCrystalName(int index = 0) const;

private:
  std::vector<OccCrystal> m_crystals;
  std::vector<QByteArray> m_crystalCifContents;
  std::vector<QString> m_crystalNames;
};
