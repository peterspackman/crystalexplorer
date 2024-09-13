#include "load_wavefunction.h"
#include <QByteArray>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
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

inline bool detectXTB(const QJsonDocument &doc) {
  QJsonObject obj = doc.object();
  QJsonValue versionValue = obj.value("xtb version");
  return !versionValue.isUndefined();
}

void setXTBJsonProperties(MolecularWavefunction *wfn,
                          const QJsonDocument &doc) {
  QJsonObject obj = doc.object();
  const std::vector<const char *> keys{
      "electronic energy",
      "HOMO-LUMO gap / eV",
  };

  wfn->setTotalEnergy(obj.value("total energy").toDouble());

  wfn->setProperty("method", obj.value("method").toString("Unknown"));

  for (const auto &k : keys) {
    wfn->setProperty(k, obj.value(k).toDouble());
  }
}

void setJsonProperties(MolecularWavefunction *wfn, const QJsonDocument &doc) {
  QJsonObject obj = doc.object();

  if (detectXTB(doc)) {
    setXTBJsonProperties(wfn, doc);
    return;
  }

  QJsonValue basisValue = obj.value("basis functions");
  if (basisValue.isUndefined() || !basisValue.isDouble()) {
    qWarning() << "Expected a numeric 'basis functions' value";
  } else {
    wfn->setNumberOfBasisFunctions(basisValue.toInt());
  }

  // Check for the existence of energies
  QJsonValue energyValue = obj.value("energy");

  if (energyValue.isUndefined() || !energyValue.isObject()) {
    qWarning() << "Expected an 'energy' object";
  } else {
    QJsonObject energyObj = energyValue.toObject();
    QJsonValue totalEnergyValue = energyObj.value("total");
    if (totalEnergyValue.isUndefined() || !totalEnergyValue.isDouble()) {
      qWarning() << "Expected a numeric 'total' energy value";
    } else {
      double totalEnergy = totalEnergyValue.toDouble();
      qDebug() << "Total Energy:" << totalEnergy;
      wfn->setTotalEnergy(totalEnergy);
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
  QJsonDocument doc = QJsonDocument::fromJson(contents);
  if (doc.isNull())
    return false;
  qDebug() << "Found JSON format, setting additional data";
  setJsonProperties(wfn, doc);
  return true;
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
