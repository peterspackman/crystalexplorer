#pragma once

#include <QString>
#include <QByteArray>
#include <occ/crystal/crystal.h>

using OccCrystal = occ::crystal::Crystal;

class PdbFile {
public:
  PdbFile() = default;
  bool readFromFile(const QString &fileName);
  bool readFromString(const QString &content);

  int numberOfCrystals() const;

  const OccCrystal &getCrystalStructure(int index = 0) const;
  const QByteArray& getCrystalPdbContents(int index = 0) const;
  const QString& getCrystalName(int index = 0) const;

private:
  std::vector<OccCrystal> m_crystals;
  std::vector<QByteArray> m_crystalPdbContents;
  std::vector<QString> m_crystalNames;
};
