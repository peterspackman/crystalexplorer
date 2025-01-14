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

void loadMetadata(PairInteraction *pair, const json &obj) {
  for (auto it = obj.begin(); it != obj.end(); ++it) {
    const auto &key = it.key();
    const auto &value = it.value();

    if (key == "energies" || key == "uc_atom_offsets")
      continue;

    QString qKey = QString::fromStdString(key);
    if (value.is_number_integer()) {
      pair->addMetadata(qKey, value.get<int>());
    } else if (value.is_number_float()) {
      pair->addMetadata(qKey, value.get<double>());
    } else if (value.is_boolean()) {
      pair->addMetadata(qKey, value.get<bool>());
    } else if (value.is_string()) {
      QString qValue = QString::fromStdString(value.get<std::string>());
      if (qKey.toLower().contains("id")) {
        pair->setLabel(qValue);
      } else {
        pair->addMetadata(qKey, qValue);
      }
    }
  }
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
    neighbors.reserve(siteEnergies.size());
    offsets.reserve(siteEnergies.size());

    for (int j = 0; j < siteEnergies.size(); ++j) {
      auto *pair = new PairInteraction(modelName);
      pair_energy::Parameters params;
      params.hasPermutationSymmetry = hasPermutationSymmetry;
      pair->setParameters(params);

      const auto &dimerObj = siteEnergies[j];
      pair->setLabel(QString::number(j + 1));
      loadMetadata(pair, dimerObj);

      const auto &energiesObj = dimerObj["energies"];
      for (auto it = energiesObj.begin(); it != energiesObj.end(); ++it) {
        QString key = QString::fromStdString(it.key());
        if (it->is_number()) {
          pair->addComponent(key, it->get<double>());
        } else {
          qWarning() << "Warning: Unsupported type for key " << key;
          continue;
        }
      }
      const auto &offsetsObj = dimerObj["uc_atom_offsets"];
      DimerAtoms d;
      const auto &a = offsetsObj[0];
      d.a.reserve(a.size()); // For DimerAtoms
      for (int i = 0; i < a.size(); i++) {
        auto idx = a[i];
        d.a.push_back(GenericAtomIndex{idx[0], idx[1], idx[2], idx[3]});
      }
      const auto &b = offsetsObj[1];
      d.b.reserve(b.size());
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

inline Mesh *getMeshFromJson(const json &j) {
  Mesh *result = nullptr;
  occ::Mat3N positions;
  Eigen::VectorXf areas, energies;

  qDebug() << "Json contains" << j.size();
  if (!j.contains("positions")) {
    qDebug() << "No positions";
    return result;
  }
  if (!j.contains("areas")) {
    qDebug() << "No areas";
    return result;
  }

  if (j.contains("electronic_energies")) {
    j.at("electronic_energies").get_to(energies);
  }

  j.at("positions").get_to(positions);
  j.at("areas").get_to(areas);

  positions.array() *= occ::units::BOHR_TO_ANGSTROM;
  areas.array() *= occ::units::BOHR_TO_ANGSTROM * occ::units::BOHR_TO_ANGSTROM;

  qDebug() << "Loaded " << positions.cols() << " points, " << areas.size()
           << "areas," << energies.size() << "energy values";

  if (positions.cols() < 1)
    return result;

  result = new Mesh(positions);
  result->setVertexProperty("None", Eigen::VectorXf::Zero(positions.cols()));

  if (energies.size() > 0) {
    result->setVertexProperty("Electronic Energy", energies);
  }

  result->setVertexProperty("Area", areas);
  return result;
}

void loadCrystalClearSurfaceJson(const QString &filename,
                                 CrystalStructure *structure) {
  auto j = loadJsonDocument(filename);
  if (j.is_null())
    return;

  for (auto it = j.begin(); it != j.end(); ++it) {
    auto key = QString::fromStdString(it.key());
    if (!it->is_object())
      continue;
    qDebug() << "Key" << key;
    Mesh *mesh = getMeshFromJson(it->get<json>());
    if (!mesh)
      continue;
    mesh->setObjectName(key);
    mesh->setParent(structure);
    MeshInstance *instance = new MeshInstance(mesh);
    instance->setObjectName("+ {x,y,z} [0,0,0]");
  }
}

} // namespace io
