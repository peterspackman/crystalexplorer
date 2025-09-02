#pragma once
#include "energy_provider.h"

class SimpleEnergyProvider : public EnergyProvider {
private:
    double _energy{0.0};
    bool _hasEnergy{false};
    QString _method;
    
public:
    SimpleEnergyProvider(double energy, const QString& method) 
        : _energy(energy), _hasEnergy(true), _method(method) {}
    
    void setEnergy(double energy) { 
        _energy = energy; 
        _hasEnergy = true; 
    }
    
    void clearEnergy() { 
        _hasEnergy = false; 
    }
    
    double totalEnergy() const override { 
        return _energy; 
    }
    
    bool hasEnergy() const override { 
        return _hasEnergy; 
    }
    
    QString description() const override { 
        return QString("Energy: %1 (%2)").arg(_energy).arg(_method); 
    }
};