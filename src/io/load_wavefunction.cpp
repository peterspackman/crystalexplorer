#include "load_wavefunction.h"
#include "json.h"
#include "eigen_json.h"
#include <QByteArray>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

QByteArray readFileContents(const QString &filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Could not open file for reading:" << filePath;
    return {};
  }
  QByteArray result = file.readAll();
  file.close();
  return result;
}

inline bool detectXTB(const nlohmann::json &doc) {
  return doc.contains("xtb version");
}

void setXTBJsonProperties(MolecularWavefunction *wfn,
                          const nlohmann::json &doc) {
  const std::vector<const char *> keys{
      "electronic energy",
      "HOMO-LUMO gap / eV",
  };

  if (doc.contains("total energy")) {
    wfn->setTotalEnergy(doc.at("total energy").get<double>());
  }

  QString method{"Unknown"};
  if (doc.contains("method")) {
    doc.at("method").get_to(method);
  }
  wfn->setProperty("method", method);

  for (const auto &k : keys) {
    if (doc.contains(k)) {
      double value = doc.at(k).get<double>();
      wfn->setProperty(k, value);
    }
  }
}

void setJsonProperties(MolecularWavefunction *wfn, const nlohmann::json &doc) {
  if (detectXTB(doc)) {
    setXTBJsonProperties(wfn, doc);
    return;
  }
  int nbf = 0;
  if (doc.contains("basis functions")) {
    nbf = doc.at("basis functions").get<int>();
    wfn->setNumberOfBasisFunctions(nbf);
  } else {
    qWarning() << "Expected a numeric 'basis functions' value";
  }

  if (doc.contains("molecular orbitals")) {
    const auto &mo = doc.at("molecular orbitals");
    bool isRestricted{true};
    if(mo.contains("orbital energies")) {
      Eigen::VectorXd energies = mo.at("orbital energies");
      wfn->setOrbitalEnergies(std::vector<double>(energies.data(), energies.data() + energies.size()));
    }
    if(mo.contains("alpha electrons")) {
      wfn->setNumberOfOccupiedOrbitals(mo.at("alpha electrons").get<int>());
    }
    if(mo.contains("atomic orbitals")) {
      wfn->setNumberOfVirtualOrbitals(mo.at("atomic orbitals").get<int>() - wfn->numberOfOccupiedOrbitals());
    }

  } else {
    qWarning() << "No molecular orbitals information found";
  }

  if (doc.contains("energy")) {
    for (const auto &item : doc.at("energy").items()) {
      if (item.key() == "total") {
        double totalEnergy = item.value().get<double>();
        qDebug() << "Total Energy:" << totalEnergy;
        wfn->setTotalEnergy(totalEnergy);
      }
    }
  }
}

namespace io {
MolecularWavefunction *loadWavefunction(const QString &filename) {
  MolecularWavefunction *wfn = new MolecularWavefunction();

  // new scope to make it easy to avoid use after move
  {
    QByteArray contents = readFileContents(filename);
    qDebug() << "Read" << contents.size() << "bytes from wavefunction at"
             << filename;
    populateWavefunctionFromJsonContents(wfn, contents);
    wfn->setRawContents(contents);
  }
  wfn->setFileFormat(wfn::fileFormatFromFilename(filename));

  return wfn;
}

bool populateWavefunctionFromJsonContents(MolecularWavefunction *wfn,
                                          const QByteArray &contents) {
  try {
    nlohmann::json doc = nlohmann::json::parse(contents.constData());
    qDebug() << "Found JSON format, setting additional data";
    setJsonProperties(wfn, doc);
    return true;
  } catch (nlohmann::json::parse_error &e) {
    qWarning() << "JSON parse error:" << e.what();
    return false;
  }
  return false;
}

bool populateWavefunctionFromXtbStdoutContents(MolecularWavefunction *wfn,
                                               const QByteArray &contents) {
  qDebug() << "Found XTB stdout format, setting additional data";

  QTextStream stream(contents);
  QString line;

  QRegularExpression versionRegex("\\* xtb version (\\S+)");
  QRegularExpression totalEnergyRegex("TOTAL ENERGY\\s+(-?\\d+\\.\\d+)\\s+Eh");
  QRegularExpression gradientNormRegex(
      "GRADIENT NORM\\s+(-?\\d+\\.\\d+)\\s+Eh/α");
  QRegularExpression homoLumoGapRegex("HOMO-LUMO GAP\\s+(-?\\d+\\.\\d+)\\s+eV");
  QRegularExpression sccEnergyRegex("SCC energy\\s+(-?\\d+\\.\\d+)\\s+Eh");
  QRegularExpression dispersionEnergyRegex(
      "-> dispersion\\s+(-?\\d+\\.\\d+)\\s+Eh");
  QRegularExpression repulsionEnergyRegex(
      "repulsion energy\\s+(-?\\d+\\.\\d+)\\s+Eh");
  QRegularExpression wallTimeRegex("\\* "
                                   "wall-time:\\s+(\\d+)\\s+d,\\s+(\\d+)\\s+h,"
                                   "\\s+(\\d+)\\s+min,\\s+([\\d.]+)\\s+sec");

  while (stream.readLineInto(&line)) {
    QRegularExpressionMatch match;

    if ((match = versionRegex.match(line)).hasMatch()) {
      wfn->setProperty("xtb version", match.captured(1));
    } else if ((match = totalEnergyRegex.match(line)).hasMatch()) {
      double totalEnergy = match.captured(1).toDouble();
      wfn->setTotalEnergy(totalEnergy);
      wfn->setProperty("total energy", totalEnergy);
    } else if ((match = gradientNormRegex.match(line)).hasMatch()) {
      wfn->setProperty("gradient norm", match.captured(1).toDouble());
    } else if ((match = homoLumoGapRegex.match(line)).hasMatch()) {
      wfn->setProperty("HOMO-LUMO gap / eV", match.captured(1).toDouble());
    } else if ((match = sccEnergyRegex.match(line)).hasMatch()) {
      wfn->setProperty("SCC energy", match.captured(1).toDouble());
    } else if ((match = dispersionEnergyRegex.match(line)).hasMatch()) {
      wfn->setProperty("dispersion energy", match.captured(1).toDouble());
    } else if ((match = repulsionEnergyRegex.match(line)).hasMatch()) {
      wfn->setProperty("repulsion energy", match.captured(1).toDouble());
    } else if ((match = wallTimeRegex.match(line)).hasMatch()) {
      int days = match.captured(1).toInt();
      int hours = match.captured(2).toInt();
      int minutes = match.captured(3).toInt();
      double seconds = match.captured(4).toDouble();
      double totalSeconds =
          days * 86400 + hours * 3600 + minutes * 60 + seconds;
      wfn->setProperty("wall time / s", totalSeconds);
    }
  }

  wfn->setProperty("method", "xtb");
  return true;
}

bool populateWavefunctionFromXtbPropertiesContents(MolecularWavefunction *wfn,
                                                   const QByteArray &contents) {
  qDebug() << "Found XTB properties format, setting additional data";

  QTextStream stream(contents);
  QString line;

  // Define regular expressions for splitting and matching
  QRegularExpression whitespaceRegex("\\s+");
  QRegularExpression totalEnergyRegex("TOTAL ENERGY");
  QRegularExpression hlGapRegex("HL-Gap");
  QRegularExpression dipoleRegex("full:");

  while (stream.readLineInto(&line)) {
    if (totalEnergyRegex.match(line).hasMatch()) {
      QStringList parts = line.split(whitespaceRegex, Qt::SkipEmptyParts);
      if (parts.size() >= 3) {
        double totalEnergy = parts[2].toDouble();
        wfn->setTotalEnergy(totalEnergy);
        wfn->setProperty("total energy", totalEnergy);
      }
    } else if (hlGapRegex.match(line).hasMatch()) {
      QStringList parts = line.split(whitespaceRegex, Qt::SkipEmptyParts);
      if (parts.size() >= 5) {
        double homoLumoGapEv = parts[4].toDouble();
        wfn->setProperty("HOMO-LUMO gap / eV", homoLumoGapEv);
      }
    } else if (dipoleRegex.match(line).hasMatch()) {
      QStringList parts = line.split(whitespaceRegex, Qt::SkipEmptyParts);
      if (parts.size() >= 5) {
        double dipoleMoment = parts[4].toDouble();
        wfn->setProperty("dipole moment / Debye", dipoleMoment);
        break; // Assuming this is the last property we need
      }
    }
  }
  wfn->setProperty("method", "xtb");
  return true;
}

bool populateWavefunctionFromMoldenContents(MolecularWavefunction *wfn,
                                            const QByteArray &contents) {
  // TODO
  qDebug() << "Found Molden format, setting additional data";
  return true;
}

} // namespace io
