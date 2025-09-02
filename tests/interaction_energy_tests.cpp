#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <QMap>

#include "interaction_energy_calculator.h"
#include "simple_energy_provider.h"
#include "molecular_wavefunction_provider.h"
#include "molecular_wavefunction.h"

using Catch::Approx;

class MockEnergyProvider : public EnergyProvider {
private:
    double _energy{0.0};
    bool _hasEnergy{false};
    QString _method;

public:
    MockEnergyProvider(double energy, const QString& method) 
        : _energy(energy), _hasEnergy(true), _method(method) {}
    
    void setEnergy(double energy) { _energy = energy; _hasEnergy = true; }
    void clearEnergy() { _hasEnergy = false; }
    
    double totalEnergy() const override { return _energy; }
    bool hasEnergy() const override { return _hasEnergy; }
    QString description() const override { 
        return QString("Mock: %1 (%2)").arg(_energy).arg(_method); 
    }
};

class MockWavefunctionProvider : public WavefunctionProvider {
private:
    double _energy{0.0};
    bool _hasEnergy{false};
    bool _hasWavefunction{false};
    QByteArray _wfnData;
    QString _method;
    std::vector<double> _orbitalEnergies;

public:
    MockWavefunctionProvider(double energy, const QString& method, bool hasWfn = true) 
        : _energy(energy), _hasEnergy(true), _hasWavefunction(hasWfn), _method(method) {
        if (hasWfn) {
            _wfnData = QByteArray("mock wavefunction data");
            _orbitalEnergies = {-1.0, -0.5, 0.2, 0.8};
        }
    }
    
    double totalEnergy() const override { return _energy; }
    bool hasEnergy() const override { return _hasEnergy; }
    bool hasWavefunction() const override { return _hasWavefunction; }
    const QByteArray& wavefunctionData() const override { return _wfnData; }
    int numberOfOrbitals() const override { return static_cast<int>(_orbitalEnergies.size()); }
    const std::vector<double>& orbitalEnergies() const override { return _orbitalEnergies; }
    QString description() const override { 
        return QString("MockWfn: %1 (%2)").arg(_energy).arg(_method); 
    }
};

TEST_CASE("Interaction energy subtraction method", "[interaction][subtraction]") {
    MockEnergyProvider providerAB(-200.0, "dimer");
    MockEnergyProvider providerA(-80.0, "monomer_A");
    MockEnergyProvider providerB(-90.0, "monomer_B");
    
    auto result = InteractionEnergyCalculator::calculateInteraction(
        &providerAB, &providerA, &providerB);
    
    REQUIRE(result.success);
    REQUIRE(result.interactionEnergy == Approx(-30.0)); // -200 - (-80) - (-90)
    REQUIRE(result.method == InteractionEnergyCalculator::CalculationMethod::Subtraction);
    REQUIRE(result.description.contains("E_AB - E_A - E_B"));
}

TEST_CASE("Interaction energy direct method", "[interaction][direct]") {
    // Mock a wavefunction provider that calculates interaction directly
    MockWavefunctionProvider directProvider(-15.5, "direct_interaction");
    
    auto result = InteractionEnergyCalculator::calculateInteraction(&directProvider);
    
    REQUIRE(result.success);
    REQUIRE(result.interactionEnergy == Approx(-15.5));
    REQUIRE(result.method == InteractionEnergyCalculator::CalculationMethod::Direct);
    REQUIRE(result.description.contains("Direct from wavefunction"));
}

TEST_CASE("Method preference: direct over subtraction", "[interaction][method_preference]") {
    // When we have both wavefunction and monomer data, direct should be preferred
    MockWavefunctionProvider wfnProvider(-12.5, "direct");
    MockEnergyProvider providerA(-80.0, "monomer_A");
    MockEnergyProvider providerB(-90.0, "monomer_B");
    
    auto result = InteractionEnergyCalculator::calculateInteraction(
        &wfnProvider, &providerA, &providerB);
    
    REQUIRE(result.success);
    REQUIRE(result.interactionEnergy == Approx(-12.5)); // Direct energy, not subtraction
    REQUIRE(result.method == InteractionEnergyCalculator::CalculationMethod::Direct);
}

TEST_CASE("Fallback to subtraction when no wavefunction", "[interaction][fallback]") {
    // Energy provider without wavefunction should fall back to subtraction
    MockWavefunctionProvider energyProvider(-200.0, "energy_only", false);
    MockEnergyProvider providerA(-80.0, "monomer_A");
    MockEnergyProvider providerB(-90.0, "monomer_B");
    
    auto result = InteractionEnergyCalculator::calculateInteraction(
        &energyProvider, &providerA, &providerB);
    
    REQUIRE(result.success);
    REQUIRE(result.interactionEnergy == Approx(-30.0)); // -200 - (-80) - (-90)
    REQUIRE(result.method == InteractionEnergyCalculator::CalculationMethod::Subtraction);
}

TEST_CASE("Trimer interaction energies", "[interaction][trimer]") {
    // Test 3-body interaction: E_ABC - E_AB - E_AC - E_BC + E_A + E_B + E_C
    // Create providers - in a real app these would be parented to ensure cleanup
    MockEnergyProvider providerABC(-300.0, "trimer");
    MockEnergyProvider providerAB(-190.0, "dimer_AB");
    MockEnergyProvider providerAC(-185.0, "dimer_AC");
    MockEnergyProvider providerBC(-195.0, "dimer_BC");
    MockEnergyProvider providerA(-80.0, "monomer_A");
    MockEnergyProvider providerB(-90.0, "monomer_B");
    MockEnergyProvider providerC(-85.0, "monomer_C");
    
    double expectedTrimer = -300.0 - (-190.0) - (-185.0) - (-195.0) + (-80.0) + (-90.0) + (-85.0);
    
    SECTION("Manual calculation") {
        REQUIRE(expectedTrimer == Approx(15.0));
    }
    
    SECTION("Using calculator for pairs") {
        auto result_AB = InteractionEnergyCalculator::calculateInteraction(
            &providerAB, &providerA, &providerB);
        auto result_AC = InteractionEnergyCalculator::calculateInteraction(
            &providerAC, &providerA, &providerC);
        auto result_BC = InteractionEnergyCalculator::calculateInteraction(
            &providerBC, &providerB, &providerC);
            
        REQUIRE(result_AB.interactionEnergy == Approx(-20.0)); // -190 + 80 + 90
        REQUIRE(result_AC.interactionEnergy == Approx(-20.0)); // -185 + 80 + 85  
        REQUIRE(result_BC.interactionEnergy == Approx(-20.0)); // -195 + 90 + 85
        
        // All should use subtraction method
        REQUIRE(result_AB.method == InteractionEnergyCalculator::CalculationMethod::Subtraction);
        REQUIRE(result_AC.method == InteractionEnergyCalculator::CalculationMethod::Subtraction);
        REQUIRE(result_BC.method == InteractionEnergyCalculator::CalculationMethod::Subtraction);
    }
}

TEST_CASE("Real MolecularWavefunction integration", "[interaction][integration]") {
    SECTION("Direct calculation with real wavefunction") {
        MolecularWavefunction wfn;
        wfn.setTotalEnergy(-25.75);
        wfn.setRawContents(QByteArray("real wavefunction data"));
        
        MolecularWavefunctionProvider provider(&wfn);
        auto result = InteractionEnergyCalculator::calculateInteraction(&provider);
        
        REQUIRE(result.success);
        REQUIRE(result.interactionEnergy == Approx(-25.75));
        REQUIRE(result.method == InteractionEnergyCalculator::CalculationMethod::Direct);
    }
    
    SECTION("Subtraction with SimpleEnergyProvider") {
        SimpleEnergyProvider providerAB(-180.0, "B3LYP");
        SimpleEnergyProvider providerA(-70.0, "B3LYP");  
        SimpleEnergyProvider providerB(-85.0, "B3LYP");
        
        auto result = InteractionEnergyCalculator::calculateInteraction(
            &providerAB, &providerA, &providerB);
        
        REQUIRE(result.success);
        REQUIRE(result.interactionEnergy == Approx(-25.0)); // -180 - (-70) - (-85)
        REQUIRE(result.method == InteractionEnergyCalculator::CalculationMethod::Subtraction);
    }
}

TEST_CASE("Error handling", "[interaction][error]") {
    SECTION("Missing providers") {
        MockEnergyProvider providerAB(-200.0, "dimer");
        
        auto result = InteractionEnergyCalculator::calculateInteraction(
            &providerAB, nullptr, nullptr);
            
        REQUIRE(!result.success); // Should fail without A and B for subtraction
    }
    
    SECTION("Invalid energy data") {
        MockEnergyProvider providerAB(-200.0, "dimer");
        MockEnergyProvider providerA(-80.0, "monomer_A");
        MockEnergyProvider providerB(-90.0, "monomer_B");
        
        providerA.clearEnergy(); // Invalidate one provider
        
        auto result = InteractionEnergyCalculator::calculateInteraction(
            &providerAB, &providerA, &providerB);
            
        REQUIRE(!result.success);
    }
    
    SECTION("All providers invalid") {
        MockEnergyProvider providerAB(0.0, "invalid");
        MockEnergyProvider providerA(0.0, "invalid");
        MockEnergyProvider providerB(0.0, "invalid");
        
        providerAB.clearEnergy();
        providerA.clearEnergy();
        providerB.clearEnergy();
        
        auto result = InteractionEnergyCalculator::calculateInteraction(
            &providerAB, &providerA, &providerB);
            
        REQUIRE(!result.success);
    }
    
    SECTION("Only dimer provider valid") {
        MockWavefunctionProvider providerAB(-15.0, "direct", false); // No wavefunction
        MockEnergyProvider providerA(0.0, "invalid");
        MockEnergyProvider providerB(0.0, "invalid");
        
        providerA.clearEnergy();
        providerB.clearEnergy();
        
        auto result = InteractionEnergyCalculator::calculateInteraction(
            &providerAB, &providerA, &providerB);
            
        REQUIRE(!result.success); // Can't do subtraction with invalid A, B
    }
}

