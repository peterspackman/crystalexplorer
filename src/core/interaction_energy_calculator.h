#pragma once
#include "computation_provider.h"
#include "wavefunction_provider.h"

class InteractionEnergyCalculator {
public:
    enum class CalculationMethod {
        Direct,        // E_AB directly from wavefunction
        Subtraction    // E_AB - E_A - E_B
    };
    
    struct Result {
        double interactionEnergy{0.0};
        CalculationMethod method;
        QString description;
        bool success{false};
    };
    
    static Result calculateInteraction(
        ComputationProvider* providerAB,
        ComputationProvider* providerA = nullptr,
        ComputationProvider* providerB = nullptr
    ) {
        Result result;
        
        // Try direct calculation first if we have a wavefunction
        if (auto wfnProvider = dynamic_cast<WavefunctionProvider*>(providerAB)) {
            if (wfnProvider->hasWavefunction()) {
                result.interactionEnergy = wfnProvider->totalEnergy();
                result.method = CalculationMethod::Direct;
                result.description = "Direct from wavefunction";
                result.success = true;
                return result;
            }
        }
        
        // Fall back to subtraction method
        if (providerA && providerB && 
            providerAB->hasValidData() && providerA->hasValidData() && providerB->hasValidData()) {
            
            auto energyAB = providerAB->getProperty("energy");
            auto energyA = providerA->getProperty("energy");
            auto energyB = providerB->getProperty("energy");
            
            if (energyAB.isValid() && energyA.isValid() && energyB.isValid()) {
                double eAB = energyAB.toDouble();
                double eA = energyA.toDouble();
                double eB = energyB.toDouble();
                
                result.interactionEnergy = eAB - eA - eB;
                result.method = CalculationMethod::Subtraction;
                result.description = "E_AB - E_A - E_B";
                result.success = true;
            }
        }
        
        return result;
    }
};