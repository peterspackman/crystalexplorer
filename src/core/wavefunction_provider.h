#pragma once
#include "energy_provider.h"
#include <QByteArray>
#include <vector>

class WavefunctionProvider : public EnergyProvider {
public:
    virtual const QByteArray& wavefunctionData() const = 0;
    virtual bool hasWavefunction() const = 0;
    virtual int numberOfOrbitals() const = 0;
    virtual const std::vector<double>& orbitalEnergies() const = 0;
    
    // Extended property support
    bool canProvideProperty(const QString& property) const override {
        return EnergyProvider::canProvideProperty(property) ||
               property == "wavefunction" || property == "orbitals" || 
               property == "orbital_energies";
    }
    
    QVariant getProperty(const QString& property) const override {
        if (EnergyProvider::canProvideProperty(property)) {
            return EnergyProvider::getProperty(property);
        }
        
        if (property == "wavefunction") {
            return hasWavefunction() ? QVariant(wavefunctionData()) : QVariant();
        } else if (property == "orbitals") {
            return QVariant(numberOfOrbitals());
        } else if (property == "orbital_energies") {
            // Note: QVariant doesn't directly support std::vector<double>
            // This is a simplified implementation
            return QVariant();
        }
        return QVariant();
    }
    
    bool hasValidData() const override { 
        return hasWavefunction() || hasEnergy(); 
    }
};