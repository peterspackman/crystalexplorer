#pragma once
#include "wavefunction_provider.h"
#include "molecular_wavefunction.h"

class MolecularWavefunctionProvider : public WavefunctionProvider {
private:
    MolecularWavefunction* _wfn;
    
public:
    MolecularWavefunctionProvider(MolecularWavefunction* wfn) : _wfn(wfn) {}
    
    double totalEnergy() const override { 
        return _wfn->totalEnergy(); 
    }
    
    bool hasEnergy() const override { 
        return _wfn->totalEnergy() != 0.0; 
    }
    
    bool hasWavefunction() const override { 
        return _wfn->haveContents(); 
    }
    
    const QByteArray& wavefunctionData() const override { 
        return _wfn->rawContents(); 
    }
    
    int numberOfOrbitals() const override { 
        return _wfn->numberOfOrbitals(); 
    }
    
    const std::vector<double>& orbitalEnergies() const override { 
        return _wfn->orbitalEnergies(); 
    }
    
    QString description() const override { 
        return _wfn->description(); 
    }
};