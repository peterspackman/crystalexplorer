#pragma once
#include "externalprogram.h"
#include "pair_energy_parameters.h"
#include <nlohmann/json.hpp>
#include <Eigen/Geometry>

class CustomEnergyCalculatorTask: public ExternalProgramTask {
    Q_OBJECT

public:
    explicit CustomEnergyCalculatorTask(QObject * parent = nullptr);

    void setParameters(const pair_energy::Parameters &);
    void setCalculatorName(const QString &calculatorName);
    virtual void start() override;

    QString jsonFilename() const;

signals:
    void calculationComplete(pair_energy::Parameters params, CustomEnergyCalculatorTask *task);

protected:
    virtual void postProcess() override;

private:
    nlohmann::json prepareInputJson(const pair_energy::Parameters &params) const;
    nlohmann::json prepareMoleculeJson(const std::vector<GenericAtomIndex> &atoms, 
                                      const Eigen::Isometry3d &transform = Eigen::Isometry3d::Identity()) const;
    void parseResultJson(const QString &jsonPath);
    double parseEnergyFromResult(const nlohmann::json &result) const;

    pair_energy::Parameters m_parameters;
    QString m_calculatorName;
    QString m_calculatorCommand;
    double m_interactionEnergy{0.0};
};