#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "simple_energy_provider.h"
#include "molecular_wavefunction_provider.h"
#include "interaction_energy_calculator.h"
#include "molecular_wavefunction.h"

TEST_CASE("PairEnergyCalculator integration with provider system", "[pair_energy][integration]") {
    
    SECTION("Provider calculation matches original arithmetic") {
        // Create mock wavefunctions with known energies
        auto wfnA = new MolecularWavefunction();
        auto wfnB = new MolecularWavefunction();
        auto wfnAB = new MolecularWavefunction();
        
        // Set up test energies (in atomic units)
        double energyA = -10.5;
        double energyB = -8.3;
        double energyAB = -19.2; // Combined energy
        
        wfnA->setTotalEnergy(energyA);
        wfnB->setTotalEnergy(energyB);
        wfnAB->setTotalEnergy(energyAB);
        
        // Original calculation method
        double referenceEnergy = energyA + energyB;
        double originalResult = energyAB - referenceEnergy;
        
        // Provider-based calculation
        SimpleEnergyProvider combinedProvider(energyAB, "test-method");
        MolecularWavefunctionProvider providerA(wfnA);
        MolecularWavefunctionProvider providerB(wfnB);
        
        auto calcResult = InteractionEnergyCalculator::calculateInteraction(
            &combinedProvider, &providerA, &providerB);
        
        REQUIRE(calcResult.success);
        REQUIRE_THAT(calcResult.interactionEnergy, 
                     Catch::Matchers::WithinAbs(originalResult, 1e-10));
        
        // Clean up
        delete wfnA;
        delete wfnB;
        delete wfnAB;
    }
    
    SECTION("Provider calculation handles edge cases") {
        auto wfnA = new MolecularWavefunction();
        auto wfnB = new MolecularWavefunction();
        auto wfnAB = new MolecularWavefunction();
        
        // Test with zero interaction energy
        double energyA = -5.0;
        double energyB = -3.0;
        double energyAB = -8.0; // Exactly sum of monomers
        
        wfnA->setTotalEnergy(energyA);
        wfnB->setTotalEnergy(energyB);
        wfnAB->setTotalEnergy(energyAB);
        
        SimpleEnergyProvider combinedProvider(energyAB, "test-method");
        MolecularWavefunctionProvider providerA(wfnA);
        MolecularWavefunctionProvider providerB(wfnB);
        
        auto calcResult = InteractionEnergyCalculator::calculateInteraction(
            &combinedProvider, &providerA, &providerB);
        
        REQUIRE(calcResult.success);
        REQUIRE_THAT(calcResult.interactionEnergy, 
                     Catch::Matchers::WithinAbs(0.0, 1e-10));
        
        delete wfnA;
        delete wfnB;
        delete wfnAB;
    }
    
    SECTION("Provider calculation with repulsive interaction") {
        auto wfnA = new MolecularWavefunction();
        auto wfnB = new MolecularWavefunction();
        auto wfnAB = new MolecularWavefunction();
        
        // Test with positive (repulsive) interaction energy
        double energyA = -10.0;
        double energyB = -5.0;
        double energyAB = -14.5; // Less stable than sum of monomers
        
        wfnA->setTotalEnergy(energyA);
        wfnB->setTotalEnergy(energyB);
        wfnAB->setTotalEnergy(energyAB);
        
        double originalResult = energyAB - (energyA + energyB); // Should be +0.5
        
        SimpleEnergyProvider combinedProvider(energyAB, "test-method");
        MolecularWavefunctionProvider providerA(wfnA);
        MolecularWavefunctionProvider providerB(wfnB);
        
        auto calcResult = InteractionEnergyCalculator::calculateInteraction(
            &combinedProvider, &providerA, &providerB);
        
        REQUIRE(calcResult.success);
        REQUIRE_THAT(calcResult.interactionEnergy, 
                     Catch::Matchers::WithinAbs(originalResult, 1e-10));
        REQUIRE(calcResult.interactionEnergy > 0.0); // Repulsive
        
        delete wfnA;
        delete wfnB;
        delete wfnAB;
    }
    
    SECTION("Unit conversion consistency") {
        auto wfnA = new MolecularWavefunction();
        auto wfnB = new MolecularWavefunction();
        auto wfnAB = new MolecularWavefunction();
        
        double energyA = -100.5;
        double energyB = -80.3;
        double energyAB = -182.1;
        
        wfnA->setTotalEnergy(energyA);
        wfnB->setTotalEnergy(energyB);
        wfnAB->setTotalEnergy(energyAB);
        
        SimpleEnergyProvider combinedProvider(energyAB, "test-method");
        MolecularWavefunctionProvider providerA(wfnA);
        MolecularWavefunctionProvider providerB(wfnB);
        
        auto calcResult = InteractionEnergyCalculator::calculateInteraction(
            &combinedProvider, &providerA, &providerB);
        
        REQUIRE(calcResult.success);
        
        // Original conversion factor used in PairEnergyCalculator
        double conversionFactor = 2625.5; // a.u. to kJ/mol
        double energyInKjMol = calcResult.interactionEnergy * conversionFactor;
        
        // Verify the conversion makes sense
        REQUIRE(std::abs(energyInKjMol) > 0.1); // Should be meaningful magnitude
        
        delete wfnA;
        delete wfnB;
        delete wfnAB;
    }
}