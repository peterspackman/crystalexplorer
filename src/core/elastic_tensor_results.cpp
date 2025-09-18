#include "elastic_tensor_results.h"
#include "chemicalstructure.h"
#include <QDebug>
#include <algorithm>
#include <cmath>

ElasticTensorResults::ElasticTensorResults(QObject *parent)
    : QObject(parent), m_name("Elastic Tensor"),
      m_elasticMatrix(occ::Mat6::Zero()) {
  setObjectName(m_name);
}

ElasticTensorResults::ElasticTensorResults(const occ::Mat6 &elasticMatrix,
                                           const QString &name, QObject *parent)
    : QObject(parent), m_name(name), m_elasticMatrix(elasticMatrix) {
  setObjectName(name);
}

void ElasticTensorResults::setElasticMatrix(const occ::Mat6 &matrix) {
  m_elasticMatrix = matrix;
  m_tensor.reset();
  emit propertyChanged();
}

const occ::Mat6 &ElasticTensorResults::elasticMatrix() const {
  return m_elasticMatrix;
}

QString ElasticTensorResults::name() const { return m_name; }

void ElasticTensorResults::setName(const QString &name) {
  if (m_name != name) {
    m_name = name;
    setObjectName(name);
    emit propertyChanged();
  }
}

QString ElasticTensorResults::description() const { return m_description; }

void ElasticTensorResults::setDescription(const QString &description) {
  if (m_description != description) {
    m_description = description;
    emit propertyChanged();
  }
}

void ElasticTensorResults::updateTensor() const {
  if (!m_tensor) {
    m_tensor = std::make_unique<occ::core::ElasticTensor>(m_elasticMatrix);
  }
}

double ElasticTensorResults::youngsModulus(const occ::Vec3 &direction) const {
  updateTensor();
  if (!m_tensor) {
    qDebug() << "Error: Elastic tensor not initialized";
    return 0.0;
  }
  try {
    return m_tensor->youngs_modulus(direction);
  } catch (const std::exception &e) {
    qDebug() << "Error calculating Young's modulus:" << e.what();
    return 0.0;
  }
}

double ElasticTensorResults::shearModulus(const occ::Vec3 &direction,
                                          double angle) const {
  updateTensor();
  if (!m_tensor) {
    qDebug() << "Error: Elastic tensor not initialized";
    return 0.0;
  }
  try {
    return m_tensor->shear_modulus(direction, angle);
  } catch (const std::exception &e) {
    qDebug() << "Error calculating shear modulus:" << e.what();
    return 0.0;
  }
}

double
ElasticTensorResults::linearCompressibility(const occ::Vec3 &direction) const {
  updateTensor();
  if (!m_tensor) {
    qDebug() << "Error: Elastic tensor not initialized";
    return 0.0;
  }
  try {
    return m_tensor->linear_compressibility(direction);
  } catch (const std::exception &e) {
    qDebug() << "Error calculating linear compressibility:" << e.what();
    return 0.0;
  }
}

double ElasticTensorResults::poissonRatio(const occ::Vec3 &direction,
                                          double angle) const {
  updateTensor();
  if (!m_tensor) {
    qDebug() << "Error: Elastic tensor not initialized";
    return 0.0;
  }
  try {
    return m_tensor->poisson_ratio(direction, angle);
  } catch (const std::exception &e) {
    qDebug() << "Error calculating Poisson ratio:" << e.what();
    return 0.0;
  }
}

double ElasticTensorResults::averageBulkModulus(
    occ::core::ElasticTensor::AveragingScheme scheme) const {
  updateTensor();
  if (!m_tensor) {
    qDebug() << "Error: Elastic tensor not initialized";
    return 0.0;
  }
  try {
    return m_tensor->average_bulk_modulus(scheme);
  } catch (const std::exception &e) {
    qDebug() << "Error calculating average bulk modulus:" << e.what();
    return 0.0;
  }
}

double ElasticTensorResults::averageShearModulus(
    occ::core::ElasticTensor::AveragingScheme scheme) const {
  updateTensor();
  if (!m_tensor) {
    qDebug() << "Error: Elastic tensor not initialized";
    return 0.0;
  }
  try {
    return m_tensor->average_shear_modulus(scheme);
  } catch (const std::exception &e) {
    qDebug() << "Error calculating average shear modulus:" << e.what();
    return 0.0;
  }
}

double ElasticTensorResults::averageYoungsModulus(
    occ::core::ElasticTensor::AveragingScheme scheme) const {
  updateTensor();
  if (!m_tensor) {
    qDebug() << "Error: Elastic tensor not initialized";
    return 0.0;
  }
  try {
    return m_tensor->average_youngs_modulus(scheme);
  } catch (const std::exception &e) {
    qDebug() << "Error calculating average Young's modulus:" << e.what();
    return 0.0;
  }
}

double ElasticTensorResults::averagePoissonRatio(
    occ::core::ElasticTensor::AveragingScheme scheme) const {
  updateTensor();
  if (!m_tensor) {
    qDebug() << "Error: Elastic tensor not initialized";
    return 0.0;
  }
  try {
    return m_tensor->average_poisson_ratio(scheme);
  } catch (const std::exception &e) {
    qDebug() << "Error calculating average Poisson ratio:" << e.what();
    return 0.0;
  }
}

occ::Vec6 ElasticTensorResults::eigenvalues() const {
  updateTensor();
  return m_tensor->eigenvalues();
}

bool ElasticTensorResults::isStable() const {
  auto evals = eigenvalues();
  for (int i = 0; i < evals.size(); ++i) {
    if (evals(i) <= 0.0) {
      return false;
    }
  }
  return true;
}

const occ::Mat6 &ElasticTensorResults::voigtStiffness() const {
  updateTensor();
  return m_tensor->voigt_c();
}

const occ::Mat6 &ElasticTensorResults::voigtCompliance() const {
  updateTensor();
  return m_tensor->voigt_s();
}

Mesh *ElasticTensorResults::createPropertyMesh(PropertyType property,
                                               int subdivisions,
                                               double radius,
                                               const Eigen::Vector3d& centerOffset) const {
  // Validate inputs
  if (subdivisions < 0 || subdivisions > 7) {
    qDebug() << "Invalid subdivisions:" << subdivisions;
    return nullptr;
  }
  if (radius <= 0.0 || radius > 100.0) {
    qDebug() << "Invalid radius:" << radius;
    return nullptr;
  }

  updateTensor();
  if (!m_tensor) {
    qDebug() << "Failed to create elastic tensor";
    return nullptr;
  }

  // Generate unit icosphere using IcosphereMesh class
  auto unitVertices = IcosphereMesh::generateVertices(subdivisions);
  auto faces = IcosphereMesh::generateFaces(subdivisions);

  // Validate geometry
  if (unitVertices.cols() == 0 || faces.cols() == 0) {
    qDebug() << "Failed to generate icosphere geometry";
    return nullptr;
  }

  // Calculate ALL property values for each vertex direction
  Mesh::ScalarPropertyValues youngsValues(unitVertices.cols());
  Mesh::ScalarPropertyValues shearMaxValues(unitVertices.cols());
  Mesh::ScalarPropertyValues shearMinValues(unitVertices.cols());
  Mesh::ScalarPropertyValues compressValues(unitVertices.cols());
  Mesh::ScalarPropertyValues poissonMaxValues(unitVertices.cols());
  Mesh::ScalarPropertyValues poissonMinValues(unitVertices.cols());

  QString propertyName;
  double maxValue = 0.0;

  try {
    for (int i = 0; i < unitVertices.cols(); ++i) {
      occ::Vec3 direction = unitVertices.col(i).normalized();

      // Validate direction vector
      if (direction.norm() < 1e-10) {
        qDebug() << "Warning: Invalid direction vector at vertex" << i;
        direction = occ::Vec3(1, 0, 0); // Default to x-direction
      }

      // Calculate all properties for this direction
      double young = youngsModulus(direction);

      // Find min/max shear modulus by sampling angles
      double shearMax = 0.0;
      double shearMin = std::numeric_limits<double>::max();
      const int numSamples = 36; // Sample every 5 degrees
      for (int j = 0; j < numSamples; ++j) {
        double angle = (j * M_PI) / numSamples;
        double shear = shearModulus(direction, angle);
        if (std::isfinite(shear)) {
          shearMax = std::max(shearMax, shear);
          shearMin = std::min(shearMin, shear);
        }
      }
      if (shearMin == std::numeric_limits<double>::max()) {
        shearMin = 0.0; // Fallback if all values were non-finite
      }

      double compress = linearCompressibility(direction);

      // Find min/max Poisson's ratio by sampling angles
      double poissonMax = -std::numeric_limits<double>::max();
      double poissonMin = std::numeric_limits<double>::max();
      for (int j = 0; j < numSamples; ++j) {
        double angle = (j * M_PI) / numSamples;
        double poisson = poissonRatio(direction, angle);
        if (std::isfinite(poisson)) {
          poissonMax = std::max(poissonMax, poisson);
          poissonMin = std::min(poissonMin, poisson);
        }
      }
      if (poissonMin == std::numeric_limits<double>::max()) {
        poissonMin = 0.0; // Fallback if all values were non-finite
        poissonMax = 0.0;
      }

      // Validate computed values
      if (!std::isfinite(young))
        young = 0.0;
      if (!std::isfinite(compress))
        compress = 0.0;

      youngsValues(i) = static_cast<float>(young);
      shearMaxValues(i) = static_cast<float>(shearMax);
      shearMinValues(i) = static_cast<float>(shearMin);
      compressValues(i) = static_cast<float>(compress);
      poissonMaxValues(i) = static_cast<float>(poissonMax);
      poissonMinValues(i) = static_cast<float>(poissonMin);

      // Find max value for the primary property being visualized (for scaling)
      double value = 0.0;
      switch (property) {
      case PropertyType::YoungsModulus:
        value = young;
        propertyName = "Young's Modulus (GPa)";
        break;
      case PropertyType::ShearModulusMax:
        value = shearMax;
        propertyName = "Shear Modulus Max (GPa)";
        break;
      case PropertyType::ShearModulusMin:
        value = shearMin;
        propertyName = "Shear Modulus Min (GPa)";
        break;
      case PropertyType::LinearCompressibility:
        value = compress;
        propertyName = "Linear Compressibility (TPa⁻¹)";
        break;
      case PropertyType::PoissonRatioMax:
        value = poissonMax;
        propertyName = "Poisson Ratio Max";
        break;
      case PropertyType::PoissonRatioMin:
        value = poissonMin;
        propertyName = "Poisson Ratio Min";
        break;
      }

      maxValue = std::max(maxValue, std::abs(value));
    }
  } catch (const std::exception &e) {
    qDebug() << "Error calculating property values:" << e.what();
    return nullptr;
  }

  // Select the appropriate property values for scaling
  Mesh::ScalarPropertyValues scalingValues(unitVertices.cols());
  switch (property) {
  case PropertyType::YoungsModulus:
    scalingValues = youngsValues;
    break;
  case PropertyType::ShearModulusMax:
    scalingValues = shearMaxValues;
    break;
  case PropertyType::ShearModulusMin:
    scalingValues = shearMinValues;
    break;
  case PropertyType::LinearCompressibility:
    scalingValues = compressValues;
    break;
  case PropertyType::PoissonRatioMax:
    scalingValues = poissonMaxValues;
    break;
  case PropertyType::PoissonRatioMin:
    scalingValues = poissonMinValues;
    break;
  }

  // Scale vertices based on property values (normalize to max value)
  Mesh::VertexList scaledVertices(3, unitVertices.cols());

  if (maxValue > 1e-10) {
    for (int i = 0; i < unitVertices.cols(); ++i) {
      occ::Vec3 direction = unitVertices.col(i);
      double scaleFactor = radius * std::abs(scalingValues(i)) / maxValue;
      // Apply scaling and center offset
      scaledVertices.col(i) = direction * scaleFactor + centerOffset;
    }
  } else {
    for (int i = 0; i < unitVertices.cols(); ++i) {
      scaledVertices.col(i) = unitVertices.col(i) * radius + centerOffset;
    }
  }

  // Create mesh with error handling
  Mesh *mesh = nullptr;
  try {
    mesh = new Mesh(scaledVertices, faces,
                    const_cast<ElasticTensorResults *>(this));
  } catch (const std::exception &e) {
    qDebug() << "Error creating mesh:" << e.what();
    return nullptr;
  }

  if (!mesh) {
    qDebug() << "Failed to create mesh object";
    return nullptr;
  }

  // Add ALL properties to mesh with error handling
  try {
    // Add all calculated properties with their ranges
    mesh->setVertexProperty("Young's Modulus (GPa)", youngsValues);
    mesh->setVertexPropertyRange(
        "Young's Modulus (GPa)",
        {youngsValues.minCoeff(), youngsValues.maxCoeff(), 0.0f});

    mesh->setVertexProperty("Shear Modulus Max (GPa)", shearMaxValues);
    mesh->setVertexPropertyRange(
        "Shear Modulus Max (GPa)",
        {shearMaxValues.minCoeff(), shearMaxValues.maxCoeff(), 0.0f});

    mesh->setVertexProperty("Shear Modulus Min (GPa)", shearMinValues);
    mesh->setVertexPropertyRange(
        "Shear Modulus Min (GPa)",
        {shearMinValues.minCoeff(), shearMinValues.maxCoeff(), 0.0f});

    mesh->setVertexProperty("Linear Compressibility (TPa⁻¹)", compressValues);
    mesh->setVertexPropertyRange(
        "Linear Compressibility (TPa⁻¹)",
        {compressValues.minCoeff(), compressValues.maxCoeff(), 0.0f});

    mesh->setVertexProperty("Poisson Ratio Max", poissonMaxValues);
    mesh->setVertexPropertyRange(
        "Poisson Ratio Max",
        {poissonMaxValues.minCoeff(), poissonMaxValues.maxCoeff(), 0.0f});

    mesh->setVertexProperty("Poisson Ratio Min", poissonMinValues);
    mesh->setVertexPropertyRange(
        "Poisson Ratio Min",
        {poissonMinValues.minCoeff(), poissonMinValues.maxCoeff(), 0.0f});

    // Set the primary property as selected
    mesh->setSelectedProperty(propertyName);

    // Set mesh name and description
    QString meshName = QString("%1 - %2").arg(propertyName, m_name);
    mesh->setObjectName(meshName);

    QString meshDesc = meshName;
    if (centerOffset.norm() > 1e-6) {
      meshDesc += QString(" (centered at %.3f, %.3f, %.3f)")
                     .arg(centerOffset.x()).arg(centerOffset.y()).arg(centerOffset.z());
    }
    mesh->setDescription(meshDesc);

    // Add default "None" property (required by renderer)
    mesh->setVertexProperty("None",
                            Eigen::VectorXf::Zero(scaledVertices.cols()));

    // Compute proper surface normals from the mesh geometry
    auto normals = mesh->computeVertexNormals(Mesh::NormalSetting::Average);
    mesh->setVertexNormals(normals);

    // Calculate atoms inside the mesh surface using parent structure
    if (auto *structure = qobject_cast<ChemicalStructure *>(this->parent())) {
      auto atomsInside = mesh->findAtomsInside(structure);
      auto atomsOutside = structure->atomIndices();

      // Remove atoms inside from the outside list
      atomsOutside.erase(
          std::remove_if(atomsOutside.begin(), atomsOutside.end(),
                         [&atomsInside](const GenericAtomIndex &idx) {
                           return std::find(atomsInside.begin(),
                                            atomsInside.end(),
                                            idx) != atomsInside.end();
                         }),
          atomsOutside.end());

      mesh->setAtomsInside(atomsInside);
      mesh->setAtomsOutside(atomsOutside);

      qDebug() << "Calculated atoms inside elastic tensor mesh:"
               << atomsInside.size() << "inside," << atomsOutside.size()
               << "outside";
    }

    // Validate mesh data integrity before returning
    if (mesh->numberOfVertices() == 0 || mesh->numberOfFaces() == 0) {
      qDebug() << "Error: Mesh has no vertices or faces";
      delete mesh;
      return nullptr;
    }

    // Validate that vertex normals were computed
    if (!mesh->haveVertexNormals()) {
      qDebug() << "Error: Mesh missing vertex normals";
      delete mesh;
      return nullptr;
    }

    // Validate that vertex properties match vertex count
    auto propNames = mesh->availableVertexProperties();
    for (const QString &propName : propNames) {
      auto propValues = mesh->vertexProperty(propName);
      if (propValues.size() != mesh->numberOfVertices()) {
        qDebug() << "Error: Property" << propName
                 << "size mismatch. Expected:" << mesh->numberOfVertices()
                 << "Got:" << propValues.size();
        delete mesh;
        return nullptr;
      }
    }

    qDebug() << "Created scaled property mesh - vertices:"
             << mesh->numberOfVertices() << "faces:" << mesh->numberOfFaces()
             << "max value:" << maxValue;

  } catch (const std::exception &e) {
    qDebug() << "Error setting mesh properties:" << e.what();
    delete mesh;
    return nullptr;
  }

  return mesh;
}

nlohmann::json ElasticTensorResults::toJson() const {
  nlohmann::json j;
  j["name"] = m_name.toStdString();
  j["description"] = m_description.toStdString();

  // Store elastic matrix
  std::vector<std::vector<double>> matrix(6, std::vector<double>(6));
  for (int i = 0; i < 6; ++i) {
    for (int k = 0; k < 6; ++k) {
      matrix[i][k] = m_elasticMatrix(i, k);
    }
  }
  j["elasticMatrix"] = matrix;

  // Store average properties
  j["averageProperties"] = {{"bulkModulus", averageBulkModulus()},
                            {"shearModulus", averageShearModulus()},
                            {"youngsModulus", averageYoungsModulus()},
                            {"poissonRatio", averagePoissonRatio()},
                            {"isStable", isStable()}};

  return j;
}

bool ElasticTensorResults::fromJson(const nlohmann::json &json) {
  try {
    if (json.contains("name")) {
      m_name = QString::fromStdString(json["name"]);
    }
    if (json.contains("description")) {
      m_description = QString::fromStdString(json["description"]);
    }

    if (json.contains("elasticMatrix")) {
      auto matrix = json["elasticMatrix"];
      for (int i = 0; i < 6; ++i) {
        for (int k = 0; k < 6; ++k) {
          m_elasticMatrix(i, k) = matrix[i][k];
        }
      }
    }

    m_tensor.reset();
    emit propertyChanged();
    return true;
  } catch (const std::exception &e) {
    qDebug() << "Error loading elastic tensor from JSON:" << e.what();
    return false;
  }
}

// ElasticTensorCollection implementation
ElasticTensorCollection::ElasticTensorCollection(QObject *parent)
    : QObject(parent) {}

void ElasticTensorCollection::add(ElasticTensorResults *tensor) {
  if (tensor && !m_tensors.contains(tensor)) {
    tensor->setParent(this);
    m_tensors.append(tensor);
    emit tensorAdded(tensor);
  }
}

void ElasticTensorCollection::remove(ElasticTensorResults *tensor) {
  if (m_tensors.removeOne(tensor)) {
    tensor->setParent(nullptr);
    emit tensorRemoved(tensor);
  }
}

void ElasticTensorCollection::clear() {
  while (!m_tensors.isEmpty()) {
    remove(m_tensors.last());
  }
}

int ElasticTensorCollection::count() const { return m_tensors.size(); }

ElasticTensorResults *ElasticTensorCollection::at(int index) const {
  if (index >= 0 && index < m_tensors.size()) {
    return m_tensors.at(index);
  }
  return nullptr;
}

QList<ElasticTensorResults *> ElasticTensorCollection::tensors() const {
  return m_tensors;
}

ElasticTensorResults *
ElasticTensorCollection::findByName(const QString &name) const {
  for (auto *tensor : m_tensors) {
    if (tensor->name() == name) {
      return tensor;
    }
  }
  return nullptr;
}

nlohmann::json ElasticTensorCollection::toJson() const {
  nlohmann::json j;
  j["tensors"] = nlohmann::json::array();

  for (const auto *tensor : m_tensors) {
    j["tensors"].push_back(tensor->toJson());
  }

  return j;
}

bool ElasticTensorCollection::fromJson(const nlohmann::json &json) {
  try {
    clear();

    if (json.contains("tensors")) {
      for (const auto &tensorJson : json["tensors"]) {
        auto *tensor = new ElasticTensorResults(this);
        if (tensor->fromJson(tensorJson)) {
          add(tensor);
        } else {
          delete tensor;
        }
      }
    }

    return true;
  } catch (const std::exception &e) {
    qDebug() << "Error loading elastic tensor collection from JSON:"
             << e.what();
    return false;
  }
}
