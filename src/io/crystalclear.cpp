#include "crystalclear.h"
#include "meshinstance.h"
#include "pair_energy_results.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <occ/crystal/crystal.h>
#include <occ/core/units.h>
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
  occ::crystal::SpaceGroup sg(
      json["space group"]["symbol"].toString().toStdString());

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
  for (int i = 0; i < numbersArray.size(); i++) {
    atomicNumbers(i) = numbersArray[i].toInt();
  }
  std::vector<std::string> labels;
  for (int i = 0; i < labelsArray.size(); i++) {
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
      for (auto it = energiesObj.constBegin(); it != energiesObj.constEnd();
           ++it) {
        pair->addComponent(it.key(), it.value().toDouble());
      }
      QJsonArray offsetsObj = dimerObj["uc_atom_offsets"].toArray();
      DimerAtoms d;
      QJsonArray a = offsetsObj[0].toArray();
      for (int i = 0; i < a.size(); i++) {
        QJsonArray idx = a[i].toArray();
        d.a.push_back(GenericAtomIndex{idx[0].toInt(), idx[1].toInt(),
                                       idx[2].toInt(), idx[3].toInt()});
      }
      QJsonArray b = offsetsObj[1].toArray();
      for (int i = 0; i < b.size(); i++) {
        QJsonArray idx = b[i].toArray();
        d.b.push_back(GenericAtomIndex{idx[0].toInt(), idx[1].toInt(),
                                       idx[2].toInt(), idx[3].toInt()});
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

void loadCrystalClearSurfaceJson(const QString &filename,
                                 CrystalStructure *structure) {
  auto doc = loadJsonDocument(filename);
  if (doc.isNull())
    return;
  QJsonObject json = doc.object();

  QJsonArray cdsPositionsArray = json["cds_pos"].toArray();
  QJsonArray row = cdsPositionsArray[0].toArray();
  occ::Mat3N cdsPositions(3, row.size());
  qDebug() << cdsPositions.cols();
  for (int i = 0; i < 3; ++i) {
    QJsonArray row = cdsPositionsArray[i].toArray();
    for (int j = 0; j < row.size(); ++j) {
      cdsPositions(i, j) = row[j].toDouble() * occ::units::BOHR_TO_ANGSTROM;
    }
  }

  QJsonArray cdsAreasArray = json["a_cds"].toArray();
  Eigen::VectorXf cdsAreas(cdsAreasArray.size());
  for (int i = 0; i < cdsAreas.size(); i++) {
      cdsAreas(i) = cdsAreasArray[i].toDouble() * occ::units::BOHR_TO_ANGSTROM * occ::units::BOHR_TO_ANGSTROM;
  }

  QJsonArray cdsEnergiesArray = json["e_cds"].toArray();
  Eigen::VectorXf cdsEnergies(cdsEnergiesArray.size());
  for (int i = 0; i < cdsEnergies.size(); i++) {
      cdsEnergies(i) = cdsEnergiesArray[i].toDouble();
  }
  qDebug() << "Loaded " << cdsEnergies.size() << "energy values";

  Mesh *cdsMesh = new Mesh(cdsPositions);
  cdsMesh->setVertexProperty("None",
                          Eigen::VectorXf::Zero(cdsPositions.cols()));
  cdsMesh->setVertexProperty("Energy", cdsEnergies);
  cdsMesh->setVertexProperty("Area", cdsAreas);

  cdsMesh->setObjectName("cds");
  cdsMesh->setParent(structure);
  MeshInstance *cdsInstance = new MeshInstance(cdsMesh);
  cdsInstance->setObjectName("+ {x,y,z} [0,0,0]");


  QJsonArray coulombPositionsArray = json["coulomb_pos"].toArray();
  QJsonArray coulombRow = coulombPositionsArray[0].toArray();
  occ::Mat3N coulombPositions(3, coulombRow.size());
  qDebug() << coulombPositions.cols();
  for (int i = 0; i < 3; ++i) {
    QJsonArray row = coulombPositionsArray[i].toArray();
    for (int j = 0; j < row.size(); ++j) {
      coulombPositions(i, j) = row[j].toDouble() * occ::units::BOHR_TO_ANGSTROM;
    }
  }

  QJsonArray coulombAreasArray = json["a_coulomb"].toArray();
  Eigen::VectorXf coulombAreas(coulombAreasArray.size());
  for (int i = 0; i < coulombAreas.size(); i++) {
      coulombAreas(i) = coulombAreasArray[i].toDouble() * occ::units::BOHR_TO_ANGSTROM * occ::units::BOHR_TO_ANGSTROM;
  }

  QJsonArray coulombEnergiesArray = json["e_coulomb"].toArray();
  Eigen::VectorXf coulombEnergies(coulombEnergiesArray.size());
  for (int i = 0; i < coulombEnergies.size(); i++) {
      coulombEnergies(i) = coulombEnergiesArray[i].toDouble();
  }
  qDebug() << "Loaded " << coulombEnergies.size() << "energy values";

  Mesh *mesh = new Mesh(coulombPositions);
  mesh->setVertexProperty("None",
                          Eigen::VectorXf::Zero(coulombPositions.cols()));
  mesh->setVertexProperty("Energy", coulombEnergies);
  mesh->setVertexProperty("Area", coulombAreas);

  mesh->setObjectName("Coulomb");
  mesh->setParent(structure);
  MeshInstance *instance = new MeshInstance(mesh);
  instance->setObjectName("+ {x,y,z} [0,0,0]");
}

} // namespace io
