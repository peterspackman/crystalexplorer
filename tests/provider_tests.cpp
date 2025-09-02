#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "computation_provider.h"
#include "energy_provider.h"
#include "wavefunction_provider.h"
#include "simple_energy_provider.h"
#include "molecular_wavefunction_provider.h"
#include "molecular_wavefunction.h"

using Catch::Approx;

TEST_CASE("SimpleEnergyProvider basic functionality", "[provider][energy]") {
    SimpleEnergyProvider provider(-100.5, "HF/6-31G*");
    
    SECTION("Energy access") {
        REQUIRE(provider.hasEnergy());
        REQUIRE(provider.totalEnergy() == Approx(-100.5));
        REQUIRE(provider.hasValidData());
    }
    
    SECTION("Property interface") {
        REQUIRE(provider.canProvideProperty("energy"));
        REQUIRE(provider.canProvideProperty("total_energy"));
        REQUIRE(!provider.canProvideProperty("wavefunction"));
        
        auto energyVar = provider.getProperty("energy");
        REQUIRE(energyVar.isValid());
        REQUIRE(energyVar.toDouble() == Approx(-100.5));
    }
    
    SECTION("Description") {
        REQUIRE(provider.description().contains("Energy"));
        REQUIRE(provider.description().contains("HF/6-31G*"));
    }
}

TEST_CASE("SimpleEnergyProvider state management", "[provider][energy]") {
    SimpleEnergyProvider provider(-75.0, "B3LYP");
    
    SECTION("Initial state") {
        REQUIRE(provider.hasEnergy());
        REQUIRE(provider.totalEnergy() == Approx(-75.0));
    }
    
    SECTION("Energy modification") {
        provider.setEnergy(-80.5);
        REQUIRE(provider.hasEnergy());
        REQUIRE(provider.totalEnergy() == Approx(-80.5));
    }
    
    SECTION("Clear energy") {
        provider.clearEnergy();
        REQUIRE(!provider.hasEnergy());
        REQUIRE(!provider.hasValidData());
        REQUIRE(!provider.getProperty("energy").isValid());
    }
}

TEST_CASE("MolecularWavefunctionProvider adapter", "[provider][wavefunction]") {
    MolecularWavefunction wfn;
    wfn.setTotalEnergy(-150.25);
    wfn.setRawContents(QByteArray("dummy wavefunction data"));
    wfn.setNumberOfBasisFunctions(10);
    wfn.setNumberOfOccupiedOrbitals(5);
    wfn.setNumberOfVirtualOrbitals(5);
    
    MolecularWavefunctionProvider provider(&wfn);
    
    SECTION("Energy interface") {
        REQUIRE(provider.hasEnergy());
        REQUIRE(provider.totalEnergy() == Approx(-150.25));
    }
    
    SECTION("Wavefunction interface") {
        REQUIRE(provider.hasWavefunction());
        REQUIRE(provider.wavefunctionData().size() > 0);
        REQUIRE(provider.numberOfOrbitals() == 10);
    }
    
    SECTION("Extended property support") {
        REQUIRE(provider.canProvideProperty("energy"));
        REQUIRE(provider.canProvideProperty("wavefunction"));
        REQUIRE(provider.canProvideProperty("orbitals"));
    }
    
    SECTION("Valid data check") {
        REQUIRE(provider.hasValidData());
    }
}

TEST_CASE("Empty provider edge cases", "[provider][edge_cases]") {
    SECTION("Energy provider without energy") {
        SimpleEnergyProvider provider(0.0, "test");
        provider.clearEnergy();
        
        REQUIRE(!provider.hasEnergy());
        REQUIRE(!provider.hasValidData());
        REQUIRE(!provider.getProperty("energy").isValid());
    }
    
    SECTION("MolecularWavefunction without data") {
        MolecularWavefunction wfn;
        MolecularWavefunctionProvider provider(&wfn);
        
        REQUIRE(!provider.hasEnergy());
        REQUIRE(!provider.hasWavefunction());
        REQUIRE(!provider.hasValidData());
    }
    
    SECTION("MolecularWavefunction with only energy") {
        MolecularWavefunction wfn;
        wfn.setTotalEnergy(-100.0);
        MolecularWavefunctionProvider provider(&wfn);
        
        REQUIRE(provider.hasEnergy());
        REQUIRE(!provider.hasWavefunction());
        REQUIRE(provider.hasValidData()); // Should be valid due to energy
    }
}

TEST_CASE("Property interface consistency", "[provider][properties]") {
    SECTION("Energy provider properties") {
        SimpleEnergyProvider provider(-50.0, "test");
        
        // Test all property variants
        REQUIRE(provider.canProvideProperty("energy"));
        REQUIRE(provider.canProvideProperty("total_energy"));
        REQUIRE(!provider.canProvideProperty("invalid_property"));
        
        auto energy1 = provider.getProperty("energy");
        auto energy2 = provider.getProperty("total_energy");
        auto invalid = provider.getProperty("invalid_property");
        
        REQUIRE(energy1.isValid());
        REQUIRE(energy2.isValid());
        REQUIRE(!invalid.isValid());
        REQUIRE(energy1.toDouble() == Approx(energy2.toDouble()));
    }
    
    SECTION("Wavefunction provider properties") {
        MolecularWavefunction wfn;
        wfn.setTotalEnergy(-75.0);
        wfn.setRawContents(QByteArray("test data"));
        MolecularWavefunctionProvider provider(&wfn);
        
        REQUIRE(provider.canProvideProperty("energy"));
        REQUIRE(provider.canProvideProperty("wavefunction"));
        REQUIRE(provider.canProvideProperty("orbitals"));
        REQUIRE(!provider.canProvideProperty("invalid_property"));
    }
}