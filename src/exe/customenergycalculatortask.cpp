#include "customenergycalculatortask.h"
#include "settings.h"
#include "filedependency.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <occ/core/element.h>

CustomEnergyCalculatorTask::CustomEnergyCalculatorTask(QObject *parent) 
    : ExternalProgramTask(parent) {
}

void CustomEnergyCalculatorTask::setParameters(const pair_energy::Parameters &params) {
    m_parameters = params;
    setProperty("basename", params.deriveName());
}

void CustomEnergyCalculatorTask::setCalculatorName(const QString &calculatorName) {
    m_calculatorName = calculatorName;
    
    // Load calculator configuration from settings
    QStringList calculators = settings::readSetting(settings::keys::CUSTOM_CALCULATORS).toStringList();
    QStringList commands = settings::readSetting(settings::keys::CUSTOM_COMMANDS).toStringList();
    
    int index = calculators.indexOf(calculatorName);
    if (index >= 0 && index < commands.size()) {
        m_calculatorCommand = commands[index];
        
        // Parse command to get executable and arguments
        QStringList commandParts = m_calculatorCommand.split(' ', Qt::SkipEmptyParts);
        if (!commandParts.isEmpty()) {
            setExecutable(commandParts.first());
            if (commandParts.size() > 1) {
                setArguments(commandParts.mid(1));
            }
        }
    }
}

QString CustomEnergyCalculatorTask::jsonFilename() const {
    return hashedBaseName() + "_custom_result.json";
}

nlohmann::json CustomEnergyCalculatorTask::prepareInputJson(const pair_energy::Parameters &params) const {
    nlohmann::json input;
    
    // Add task information
    input["task"] = "interaction_energy";
    input["method"] = m_calculatorName.toStdString();
    
    // Add molecule A
    input["molecule_a"] = prepareMoleculeJson(params.atomsA, params.transformA);
    
    // Add molecule B  
    input["molecule_b"] = prepareMoleculeJson(params.atomsB, params.transformB);
    
    // Add metadata
    input["metadata"] = {
        {"source", "CrystalExplorer CustomEnergyCalculator"},
        {"calculator", m_calculatorName.toStdString()},
        {"pair_name", params.deriveName().toStdString()}
    };
    
    return input;
}

nlohmann::json CustomEnergyCalculatorTask::prepareMoleculeJson(const std::vector<GenericAtomIndex> &atoms,
                                                              const Eigen::Isometry3d &transform) const {
    nlohmann::json molecule;
    
    if (!m_parameters.structure) {
        return molecule;
    }
    
    const auto &pos = m_parameters.structure->atomicPositions();
    const auto &nums = m_parameters.structure->atomicNumbers();
    
    nlohmann::json atomsJson = nlohmann::json::array();
    
    for (const auto &atomIdx : atoms) {
        int idx = atomIdx.unique;
        if (idx >= 0 && idx < pos.cols()) {
            // Get position and apply transformation
            auto position = pos.col(idx);
            Eigen::Vector3d pos3d(position(0), position(1), position(2));
            Eigen::Vector3d transformedPos = transform * pos3d;
            
            nlohmann::json atom;
            atom["element"] = occ::core::Element(nums[idx]).symbol();
            atom["position"] = {transformedPos.x(), transformedPos.y(), transformedPos.z()};
            atomsJson.push_back(atom);
        }
    }
    
    molecule["atoms"] = atomsJson;
    molecule["charge"] = 0; // Default, could be made configurable
    molecule["multiplicity"] = 1; // Default, could be made configurable
    
    return molecule;
}

void CustomEnergyCalculatorTask::start() {
    if (!m_parameters.structure) {
        qWarning() << "No chemical structure specified for custom energy calculator";
        return;
    }
    
    if (m_calculatorCommand.isEmpty()) {
        qWarning() << "No command configured for custom calculator:" << m_calculatorName;
        return;
    }
    
    QString name = hashedBaseName();
    QString inputJsonName = name + "_input.json";
    QString outputJsonName = jsonFilename();
    
    // Prepare input JSON
    nlohmann::json inputJson = prepareInputJson(m_parameters);
    
    // Write input JSON to file
    emit progressText("Writing input JSON for custom calculator");
    QFile inputFile(inputJsonName);
    if (inputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&inputFile);
        stream << QString::fromStdString(inputJson.dump(2));
        inputFile.close();
    } else {
        qWarning() << "Failed to write input JSON file:" << inputJsonName;
        return;
    }
    
    // Set up arguments to include input and output files
    QStringList args = arguments();
    args << inputJsonName << outputJsonName;
    setArguments(args);
    
    // Set up file dependencies
    QList<FileDependency> reqs{FileDependency(inputJsonName)};
    setRequirements(reqs);
    setOutputs({FileDependency(outputJsonName, outputJsonName)});
    
    emit progressText("Starting custom energy calculator");
    ExternalProgramTask::start();
}

void CustomEnergyCalculatorTask::postProcess() {
    ExternalProgramTask::postProcess();
    
    QString outputJsonName = jsonFilename();
    parseResultJson(outputJsonName);
    
    emit calculationComplete(m_parameters, this);
}

void CustomEnergyCalculatorTask::parseResultJson(const QString &jsonPath) {
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open result JSON file:" << jsonPath;
        return;
    }
    
    QTextStream stream(&file);
    QString jsonString = stream.readAll();
    file.close();
    
    try {
        nlohmann::json result = nlohmann::json::parse(jsonString.toStdString());
        m_interactionEnergy = parseEnergyFromResult(result);
        qDebug() << "Parsed interaction energy:" << m_interactionEnergy << "from custom calculator";
    } catch (const std::exception &e) {
        qWarning() << "Failed to parse result JSON:" << e.what();
        m_interactionEnergy = 0.0;
    }
}

double CustomEnergyCalculatorTask::parseEnergyFromResult(const nlohmann::json &result) const {
    // Check for error first
    if (result.contains("error")) {
        QString error = QString::fromStdString(result["error"].get<std::string>());
        qWarning() << "Custom calculator reported error:" << error;
        return 0.0;
    }
    
    // Look for energy field (following occ external energy model convention)
    if (result.contains("energy") && result["energy"].is_number()) {
        return result["energy"].get<double>();
    }
    
    // Alternative: look for interaction_energy field
    if (result.contains("interaction_energy") && result["interaction_energy"].is_number()) {
        return result["interaction_energy"].get<double>();
    }
    
    // If neither found, check for total energy and reference energies to compute interaction
    if (result.contains("total_energy") && result.contains("energy_a") && result.contains("energy_b")) {
        double total = result["total_energy"].get<double>();
        double ea = result["energy_a"].get<double>();
        double eb = result["energy_b"].get<double>();
        return total - ea - eb;
    }
    
    qWarning() << "Custom calculator result does not contain recognizable energy field";
    return 0.0;
}