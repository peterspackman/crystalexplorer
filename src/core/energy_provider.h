#pragma once
#include "computation_provider.h"

class EnergyProvider : public ComputationProvider {
public:
    virtual double totalEnergy() const = 0;
    virtual bool hasEnergy() const = 0;
    
    // ComputationProvider implementation
    bool canProvideProperty(const QString& property) const override {
        return property == "energy" || property == "total_energy";
    }
    
    QVariant getProperty(const QString& property) const override {
        if (property == "energy" || property == "total_energy") {
            return hasEnergy() ? QVariant(totalEnergy()) : QVariant();
        }
        return QVariant();
    }
    
    bool hasValidData() const override { 
        return hasEnergy(); 
    }
};