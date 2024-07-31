#include "load_wavefunction.h"
#include <QByteArray>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

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

void setXTBJsonProperties(MolecularWavefunction *wfn, const QJsonDocument &doc) {
    QJsonObject obj = doc.object();
    const std::vector<const char *> keys{
        "electronic energy",
        "HOMO-LUMO gap / eV",
    };

    wfn->setTotalEnergy(obj.value("total energy").toDouble());

    wfn->setProperty("method", obj.value("method").toString("Unknown"));

    for(const auto &k: keys) {
        wfn->setProperty(k, obj.value(k).toDouble());
    }

}

void setJsonProperties(MolecularWavefunction *wfn, const QJsonDocument &doc) {
  QJsonObject obj = doc.object();

  if(detectXTB(doc)) {
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

bool populateWavefunctionFromJsonContents(MolecularWavefunction *wfn, const QByteArray &contents) {
    QJsonDocument doc = QJsonDocument::fromJson(contents);
    if (doc.isNull())  return false;
    qDebug() << "Found JSON format, setting additional data";
    setJsonProperties(wfn, doc);
    return true;
}

bool populateWavefunctionFromMoldenContents(MolecularWavefunction *wfn, const QByteArray &contents) {
    // TODO
    return true;
}

} // namespace io
