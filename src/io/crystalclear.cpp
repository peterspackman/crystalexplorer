#include "crystalclear.h"
#include "pair_energy_results.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <occ/crystal/crystal.h>
#include <vector>

namespace io {

QJsonDocument loadJsonDocument(const QString &filename) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Could not open file";
    return QJsonDocument();
  }

  QByteArray data = file.readAll();
  return QJsonDocument::fromJson(data);
}

inline occ::crystal::Crystal loadOccCrystal(const QJsonObject &json) {
  occ::Mat3 directMatrix;
  // Parse lattice vectors
  QJsonArray latticeVectorsArray = json["unit cell"]["direct_matrix"].toArray();
  for (int i = 0; i < 3; ++i) {
    QJsonArray row = latticeVectorsArray[i].toArray();
    for (int j = 0; j < 3; ++j) {
      directMatrix(i, j) = row[j].toDouble();
    }
  }
  occ::crystal::UnitCell uc(directMatrix);
  occ::crystal::SpaceGroup sg(json["space group"]["symbol"].toString().toStdString());

  QJsonArray positionsArray = json["asymmetric unit"]["positions"].toArray();
  QJsonArray labelsArray = json["asymmetric unit"]["labels"].toArray();
  QJsonArray numbersArray = json["asymmetric unit"]["atomic numbers"].toArray();

  QJsonArray row = positionsArray[0].toArray();
  occ::Mat3N positions(3, row.size());
  qDebug() << positions.cols();
  for (int i = 0; i < 3; ++i) {
    QJsonArray row = positionsArray[i].toArray();
    for (int j = 0; j < row.size(); ++j) {
      positions(i, j) = row[j].toDouble();
    }
  }

  occ::IVec atomicNumbers(numbersArray.size());
  for(int i = 0; i < numbersArray.size(); i++) {
    atomicNumbers(i) = numbersArray[i].toInt();

  }
  std::vector<std::string> labels;
  for(int i = 0; i < labelsArray.size(); i++) {
    labels.push_back(labelsArray[i].toString().toStdString());
  }

  occ::crystal::AsymmetricUnit asym(positions, atomicNumbers, labels);

  return occ::crystal::Crystal(asym, sg, uc);
}

CrystalStructure *loadCrystalClearJson(const QString &filename) {
  auto doc = loadJsonDocument(filename);
  if (doc.isNull())
    return nullptr;
  QJsonObject json = doc.object();

  occ::crystal::Crystal crystal = loadOccCrystal(json["crystal"].toObject());

  QList<PairInteractions::PairInteractionList> interactions;
  QList<QList<DimerAtoms>> atomIndices;

  // Parse neighbor energies
  QJsonArray pairsArray = json["pairs"].toArray();
  interactions.resize(pairsArray.size());
  atomIndices.resize(pairsArray.size());
  for (int i = 0; i < pairsArray.size(); ++i) {
    QJsonArray siteEnergies = pairsArray[i].toArray();
    auto &neighbors = interactions[i];
    auto &offsets = atomIndices[i];
    for (int j = 0; j < siteEnergies.size(); ++j) {
      auto *pair = new PairInteraction("cg");
      QJsonObject dimerObj = siteEnergies[j].toObject();
      QJsonObject energiesObj = dimerObj["energies"].toObject();
      for (auto it = energiesObj.constBegin(); it != energiesObj.constEnd(); ++it) {
        pair->addComponent(it.key(), it.value().toDouble());
      }
      QJsonArray offsetsObj = dimerObj["uc_atom_offsets"].toArray();
      DimerAtoms d;
      QJsonArray a = offsetsObj[0].toArray();
      for(int i = 0; i < a.size(); i++) {
        QJsonArray idx = a[i].toArray();
        d.a.push_back(GenericAtomIndex{idx[0].toInt(), idx[1].toInt(), idx[2].toInt(), idx[3].toInt()});
      }
      QJsonArray b = offsetsObj[1].toArray();
      for(int i = 0; i < b.size(); i++) {
        QJsonArray idx = b[i].toArray();
        d.b.push_back(GenericAtomIndex{idx[0].toInt(), idx[1].toInt(), idx[2].toInt(), idx[3].toInt()});
      }
      neighbors.append(pair);
      offsets.append(d);
    }
  }

  qDebug() << interactions;

  QString title = json["title"].toString();

  CrystalStructure *result = new CrystalStructure();
  result->setOccCrystal(crystal);
  result->setPairInteractionsFromDimerAtoms(interactions, atomIndices);
  result->setName(title);
  return result;
}

} // namespace io
