#pragma once
#include "json.h"
#include "mesh.h"
#include "icosphere_mesh.h"
#include <QObject>

class ChemicalStructure;
#include <QVariant>
#include <occ/core/elastic_tensor.h>

class ElasticTensorResults : public QObject {
    Q_OBJECT

public:
    enum class PropertyType {
        YoungsModulus,
        ShearModulusMax,
        ShearModulusMin,
        LinearCompressibility,
        PoissonRatioMax,
        PoissonRatioMin
    };

    explicit ElasticTensorResults(QObject *parent = nullptr);
    ElasticTensorResults(const occ::Mat6 &elasticMatrix, 
                        const QString &name = "Elastic Tensor",
                        QObject *parent = nullptr);

    void setElasticMatrix(const occ::Mat6 &matrix);
    const occ::Mat6 &elasticMatrix() const;
    
    QString name() const;
    void setName(const QString &name);
    
    QString description() const;
    void setDescription(const QString &description);

    // Property calculations
    double youngsModulus(const occ::Vec3 &direction) const;
    double shearModulus(const occ::Vec3 &direction, double angle = 0.0) const;
    double linearCompressibility(const occ::Vec3 &direction) const;
    double poissonRatio(const occ::Vec3 &direction, double angle = 0.0) const;

    // Average properties
    double averageBulkModulus(occ::core::ElasticTensor::AveragingScheme scheme = 
                             occ::core::ElasticTensor::AveragingScheme::Hill) const;
    double averageShearModulus(occ::core::ElasticTensor::AveragingScheme scheme = 
                              occ::core::ElasticTensor::AveragingScheme::Hill) const;
    double averageYoungsModulus(occ::core::ElasticTensor::AveragingScheme scheme = 
                               occ::core::ElasticTensor::AveragingScheme::Hill) const;
    double averagePoissonRatio(occ::core::ElasticTensor::AveragingScheme scheme = 
                              occ::core::ElasticTensor::AveragingScheme::Hill) const;

    // Mesh generation for spatial visualization
    Mesh *createPropertyMesh(PropertyType property, 
                            int subdivisions = 3,
                            double radius = 10.0) const;

    // Eigenvalue analysis
    occ::Vec6 eigenvalues() const;
    bool isStable() const; // All eigenvalues positive

    // JSON serialization
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json &json);

    // Voigt notation access
    const occ::Mat6 &voigtStiffness() const;
    const occ::Mat6 &voigtCompliance() const;

signals:
    void propertyChanged();

private:
    QString m_name;
    QString m_description;
    occ::Mat6 m_elasticMatrix;
    mutable std::unique_ptr<occ::core::ElasticTensor> m_tensor;
    
    void updateTensor() const;
};

class ElasticTensorCollection : public QObject {
    Q_OBJECT

public:
    explicit ElasticTensorCollection(QObject *parent = nullptr);

    void add(ElasticTensorResults *tensor);
    void remove(ElasticTensorResults *tensor);
    void clear();

    int count() const;
    ElasticTensorResults *at(int index) const;
    QList<ElasticTensorResults *> tensors() const;

    ElasticTensorResults *findByName(const QString &name) const;

    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json &json);

signals:
    void tensorAdded(ElasticTensorResults *tensor);
    void tensorRemoved(ElasticTensorResults *tensor);

private:
    QList<ElasticTensorResults *> m_tensors;
};