#include "crystalclear.h"
#include "meshinstance.h"
#include "pair_energy_results.h"
#include <QDebug>
#include <fstream>
#include <occ/core/units.h>
#include <occ/crystal/crystal.h>
#include <string>
#include <vector>

namespace io {

using json = nlohmann::json;

json loadJsonDocument(const QString &filename) {
  std::ifstream file(filename.toStdString());
  if (!file.is_open()) {
    qWarning() << "Could not open file" << filename;
    return json();
  }
  return json::parse(file);
}

inline occ::crystal::Crystal loadOccCrystal(const json &json) {
  occ::Mat3 directMatrix;
  // Parse lattice vectors
  auto latticeVectorsArray = json["unit cell"]["direct_matrix"];
  latticeVectorsArray.get_to(directMatrix);

  occ::crystal::UnitCell uc(directMatrix);
  occ::crystal::SpaceGroup sg(json["space group"]["symbol"].get<std::string>());

  auto positionsArray = json["asymmetric unit"]["positions"];
  auto labelsArray = json["asymmetric unit"]["labels"];
  auto numbersArray = json["asymmetric unit"]["atomic numbers"];

  auto row = positionsArray[0];
  occ::Mat3N positions;
  positionsArray.get_to(positions);

  occ::IVec atomicNumbers;
  numbersArray.get_to(atomicNumbers);

  std::vector<std::string> labels;
  labelsArray.get_to(labels);

  occ::crystal::AsymmetricUnit asym(positions, atomicNumbers, labels);

  return occ::crystal::Crystal(asym, sg, uc);
}

CrystalStructure *loadCrystalClearJson(const QString &filename) {
  auto json = loadJsonDocument(filename);
  if (json.is_null())
    return nullptr;

  occ::crystal::Crystal crystal = loadOccCrystal(json["crystal"]);

  QList<PairInteractions::PairInteractionList> interactions;
  QList<QList<DimerAtoms>> atomIndices;

  QString modelName = "cg";
  if (json.contains("model")) {
    modelName = QString::fromStdString(json["model"]);
  }
  bool hasPermutationSymmetry{true};
  if (json.contains("has_permutation_symmetry")) {
    hasPermutationSymmetry = json["has_permutation_symmetry"];
  }

  // Parse neighbor energies
  auto pairsArray = json["pairs"];
  interactions.resize(pairsArray.size());
  atomIndices.resize(pairsArray.size());
  for (int i = 0; i < pairsArray.size(); ++i) {
    auto siteEnergies = pairsArray[i];
    auto &neighbors = interactions[i];
    auto &offsets = atomIndices[i];
    for (int j = 0; j < siteEnergies.size(); ++j) {
      auto *pair = new PairInteraction(modelName);
      pair_energy::Parameters params;
      params.hasPermutationSymmetry = hasPermutationSymmetry;
      pair->setParameters(params);

      auto dimerObj = siteEnergies[j];
      pair->setLabel(QString::number(j + 1));
      for (auto it = dimerObj.begin(); it != dimerObj.end(); ++it) {
        QString key = QString::fromStdString(it.key());
        if (key == "energies")
          continue;
        if (it->is_number_integer()) {
          pair->addMetadata(key, it->get<int>());
        } else if (it->is_number_float()) {
          pair->addMetadata(key, it->get<double>());
        } else if (it->is_boolean()) {
          pair->addMetadata(key, it->get<bool>());
        } else if (it->is_string()) {
          QString value = QString::fromStdString(it->get<std::string>());
          if (key.toLower().contains("id")) {
            pair->setLabel(value);
          } else {
            pair->addMetadata(key, value);
          }
        }
      }
      auto energiesObj = dimerObj["energies"];
      for (auto it = energiesObj.begin(); it != energiesObj.end(); ++it) {
        QString key = QString::fromStdString(it.key());
        if (it->is_number()) {
          pair->addComponent(key, it->get<double>());
        } else {
          qWarning() << "Warning: Unsupported type for key " << key;
          continue;
        }
      }
      auto offsetsObj = dimerObj["uc_atom_offsets"];
      DimerAtoms d;
      auto a = offsetsObj[0];
      for (int i = 0; i < a.size(); i++) {
        auto idx = a[i];
        d.a.push_back(GenericAtomIndex{idx[0], idx[1], idx[2], idx[3]});
      }
      auto b = offsetsObj[1];
      for (int i = 0; i < b.size(); i++) {
        auto idx = b[i];
        d.b.push_back(GenericAtomIndex{idx[0], idx[1], idx[2], idx[3]});
      }
      neighbors.push_back(pair);
      offsets.push_back(d);
    }
  }

  std::string title = json["title"];
  qDebug() << "Interactions loaded" << title;

  CrystalStructure *result = new CrystalStructure();
  result->setOccCrystal(crystal);
  result->setPairInteractionsFromDimerAtoms(interactions, atomIndices,
                                            hasPermutationSymmetry);
  result->setName(QString::fromStdString(title));
  return result;
}

void loadCrystalClearSurfaceJson(const QString &filename,
                                 CrystalStructure *structure) {
  auto json = loadJsonDocument(filename);
  if (json.is_null())
    return;

  qDebug() << "Pos CDS";
  occ::Mat3N cdsPositions;
  json["cds_pos"].get_to(cdsPositions);
  cdsPositions.array() *= occ::units::BOHR_TO_ANGSTROM;

  qDebug() << "A CDS";
  Eigen::VectorXf cdsAreas;
  json["a_cds"].get_to(cdsAreas);
  cdsAreas.array() *=
      occ::units::BOHR_TO_ANGSTROM * occ::units::BOHR_TO_ANGSTROM;

  qDebug() << "E CDS";
  Eigen::VectorXf cdsEnergies;
  json["e_cds"].get_to(cdsEnergies);
  qDebug() << "Loaded " << cdsEnergies.size() << " energy values";

  Mesh *cdsMesh = new Mesh(cdsPositions);
  cdsMesh->setVertexProperty("None",
                             Eigen::VectorXf::Zero(cdsPositions.cols()));
  cdsMesh->setVertexProperty("Energy", cdsEnergies);
  cdsMesh->setVertexProperty("Area", cdsAreas);

  cdsMesh->setObjectName("cds");
  cdsMesh->setParent(structure);
  MeshInstance *cdsInstance = new MeshInstance(cdsMesh);
  cdsInstance->setObjectName("+ {x,y,z} [0,0,0]");

  qDebug() << "Pos Coulomb";
  occ::Mat3N coulombPositions;
  json["coulomb_pos"].get_to(coulombPositions);
  coulombPositions.array() *= occ::units::BOHR_TO_ANGSTROM;

  qDebug() << "A Coulomb";
  Eigen::VectorXf coulombAreas;
  json["a_coulomb"].get_to(coulombAreas);
  coulombAreas.array() *=
      occ::units::BOHR_TO_ANGSTROM * occ::units::BOHR_TO_ANGSTROM;

  qDebug() << "E coulomb";
  Eigen::VectorXf coulombEnergies;
  json["e_coulomb"].get_to(coulombEnergies);
  qDebug() << "Loaded " << coulombEnergies.size() << " energy values";

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
