#pragma once
#include "crystalstructure.h"
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVector3D>
#include <array>
#include <vector>

namespace io {

struct GulpAtomPosition {
  QString element;
  QString core_shell; // "core" or "shel", empty if not specified
  double x{0.0}, y{0.0}, z{0.0};
  double charge{0.0};
  double occupancy{1.0};
  bool has_charge{false};

  static double parseFractional(const QString &value);
  QVector3D position() const { return QVector3D(x, y, z); }
};

class GulpInputFile {
public:
  GulpInputFile() = default;
  explicit GulpInputFile(const QString &filename);
  ~GulpInputFile() = default;
  GulpInputFile(const GulpInputFile &other);
  GulpInputFile &operator=(const GulpInputFile &other);

  // File operations
  bool load(const QString &filename);
  bool save(const QString &filename) const;

  // Getters
  inline const QStringList &keywords() const { return m_keywords; }
  inline const std::array<double, 6> &cellParameters() const {
    return m_cellParams;
  }
  inline const std::vector<GulpAtomPosition> &atoms() const { return m_atoms; }
  inline const QString &spaceGroup() const { return m_spaceGroup; }
  inline const QString &fileContents() const { return m_fileContents; }

  // Setters
  void setKeywords(const QStringList &keywords) { m_keywords = keywords; }
  inline void setCellParameters(const std::array<double, 6> &params) {
    m_cellParams = params;
  }
  inline void addAtom(const GulpAtomPosition &atom) { m_atoms.push_back(atom); }
  inline int periodicity() const { return m_periodicity; }
  void setSpaceGroup(const QString &sg) { m_spaceGroup = sg; }

  inline bool success() const { return m_success; }
  inline const QString &errorMessage() const { return m_errorMessage; }

  // Conversion methods for your existing structures
  ChemicalStructure *toChemicalStructure() const;
  CrystalStructure *toCrystalStructure() const;

  // Factory methods from your existing structures
  static GulpInputFile *
  fromChemicalStructure(const ChemicalStructure *structure);
  static GulpInputFile *fromCrystalStructure(const CrystalStructure *structure);

private:
  QStringList m_keywords;
  std::array<double, 6> m_cellParams; // a, b, c, alpha, beta, gamma
  std::vector<GulpAtomPosition> m_atoms;
  QString m_spaceGroup{"P 1"}; // Default to P1
  QString m_fileContents;      // Store original file contents
  int m_periodicity{false};
  bool m_fractional{false};
  bool m_success{false};
  QString m_errorMessage{"Unknown Error"};

  bool isKeywordLine(const QString &line);
  bool parseCoords(const QString &line);
  void parseCell(const QString &line);
  static QStringList tokenize(const QString &line);
};

} // namespace io
