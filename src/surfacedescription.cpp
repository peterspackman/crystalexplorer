#include "surfacedescription.h"

IsosurfacePropertyDetails::Attributes
IsosurfacePropertyDetails::getAttributes(IsosurfacePropertyDetails::Type t) {
  return IsosurfacePropertyDetails::availableTypes[t];
}

IsosurfacePropertyDetails::Type
IsosurfacePropertyDetails::typeFromTontoName(const QString &s) {
  const auto &props = IsosurfacePropertyDetails::availableTypes;
  for (auto kv = props.constKeyValueBegin(); kv != props.constKeyValueEnd();
       kv++) {
    if (kv->second.tontoName == s) {
      return kv->first;
    }
  }
  return IsosurfacePropertyDetails::Type::Unknown;
}

const QMap<IsosurfacePropertyDetails::Type,
           IsosurfacePropertyDetails::Attributes>
    IsosurfacePropertyDetails::availableTypes{
        {IsosurfacePropertyDetails::Type::None,
         IsosurfacePropertyDetails::Attributes{
             ColorScheme::noneColor, // color scheme
             "None",                 // property name
             "none",                 // tonto property type string
             "",                     // property unit of measure
             false,                  // needs a wavefunction?
             false,                  // needs an isovalue?
             false,                  // needs orbitals?
             "<p>No surface property. The surface has a "
             "solid color defined by the "
             "'None' color set in the CrystalExplorer "
             "preferences.</p>" // description
         }},

        {IsosurfacePropertyDetails::Type::DistanceInternal,
         IsosurfacePropertyDetails::Attributes{
             ColorScheme::RedGreenBlue, // color scheme
             "di",                      // property name
             "d_i",                     // tonto property type string
             ANGSTROM_SYMBOL,           // property unit of measure
             false,                     // needs a wavefunction?
             false,                     // needs an isovalue?
             false,                     // needs orbitals?
             "<p>d<sub>i</sub></p>"     // description
         }},

        {IsosurfacePropertyDetails::Type::DistanceExternal,
         IsosurfacePropertyDetails::Attributes{
             ColorScheme::RedGreenBlue, // color scheme
             "de",                      // property name
             "d_e",                     // tonto property type string
             ANGSTROM_SYMBOL,           // property unit of measure
             false,                     // needs a wavefunction?
             false,                     // needs an isovalue?
             false,                     // needs orbitals?
             "<p>d<sub>e</sub></p>"     // description
         }},

        {IsosurfacePropertyDetails::Type::DistanceNorm,
         IsosurfacePropertyDetails::Attributes{
             ColorScheme::RedWhiteBlue, // color scheme
             "dnorm",                   // property name
             "d_norm",                  // tonto property type string
             "",                        // property unit of measure
             false,                     // needs a wavefunction?
             false,                     // needs an isovalue?
             false,                     // needs orbitals?
             "<p>d<sub>norm</sub></p>"  // description
         }},

        {IsosurfacePropertyDetails::Type::ShapeIndex,
         IsosurfacePropertyDetails::Attributes{
             ColorScheme::RedGreenBlue, // color scheme
             "Shape Index",             // property name
             "shape_index",             // tonto property type string
             "",                        // property unit of measure
             false,                     // needs a wavefunction?
             false,                     // needs an isovalue?
             false,                     // needs orbitals?
             "<p>shape index</p>"       // description
         }},

        {IsosurfacePropertyDetails::Type::Curvedness,
         IsosurfacePropertyDetails::Attributes{
             ColorScheme::RedGreenBlue, // color scheme
             "Curvedness",              // property name
             "curvedness",              // tonto property type string
             "",                        // property unit of measure
             false,                     // needs a wavefunction?
             false,                     // needs an isovalue?
             false,                     // needs orbitals?
             "<p>shape index</p>"       // description
         }},

        {IsosurfacePropertyDetails::Type::PromoleculeDensity,
         IsosurfacePropertyDetails::Attributes{
             ColorScheme::RedWhiteBlue, // color scheme
             "Promolecule Density",     // property name
             "promolecule_density",     // tonto property
                                        // type string
             "au",                      // property unit of measure
             false,                     // needs a wavefunction?
             false,                     // needs an isovalue?
             false,                     // needs orbitals?
             "<p>The sum of spherical atoms electron "
             "density for the "
             "molecule.</p>" // description
         }},

        {IsosurfacePropertyDetails::Type::ElectronDensity,
         IsosurfacePropertyDetails::Attributes{
             ColorScheme::RedWhiteBlue, // color scheme
             "Electron Density",        // property name
             "electron_density",        // tonto property type
                                        // string
             "au",                      // property unit of measure
             true,                      // needs a wavefunction?
             false,                     // needs an isovalue?
             false,                     // needs orbitals?
             "<p>The electron density, calculated from "
             "the wavefunction in the "
             "previous energy calculation.</p>" // description
         }},

        {IsosurfacePropertyDetails::Type::DeformationDensity,
         IsosurfacePropertyDetails::Attributes{
             ColorScheme::RedWhiteBlue, // color scheme
             "Deformation Density",     // property name
             "deformation_density",     // tonto property
                                        // type string
             "au",                      // property unit of measure
             true,                      // needs a wavefunction?
             false,                     // needs an isovalue?
             false,                     // needs orbitals?
             "<p>The difference between the "
             "<i>ab-initio</i> electron density, and "
             "the sum of spherical atoms electron "
             "density, calculated from the "
             "wavefunction in the previous energy "
             "calculation.</p>" // description
         }},

        {IsosurfacePropertyDetails::Type::ElectricPotential,
         IsosurfacePropertyDetails::Attributes{
             ColorScheme::RedWhiteBlue, // color scheme
             "Electrostatic Potential", // property name
             "electric_potential",      // tonto property type
                                        // string
             "au",                      // property unit of measure
             true,                      // needs a wavefunction?
             false,                     // needs an isovalue?
             false,                     // needs orbitals?
             "<p>The <i>ab-initio</i> electrostatic "
             "potential from the electrons and "
             "the nuclei, calculated from the "
             "wavefunction in the previous energy "
             "calculation.</p>" // description
         }},

        {IsosurfacePropertyDetails::Type::Orbital,
         IsosurfacePropertyDetails::Attributes{
             ColorScheme::RedWhiteBlue, // color scheme
             "Orbital",                 // property name
             "orbital",                 // tonto property type string
             "au",                      // property unit of measure
             true,                      // needs a wavefunction?
             false,                     // needs an isovalue?
             true,                      // needs orbitals?
             "<p>The sign of the chosen molecular "
             "orbital in the region of the "
             "surface.</p>" // description
         }},

        {IsosurfacePropertyDetails::Type::SpinDensity,
         IsosurfacePropertyDetails::Attributes{
             ColorScheme::RedWhiteBlue, // color scheme
             "Spin Density",            // property name
             "spin_density",            // tonto property type
                                        // string
             "au",                      // property unit of measure
             true,                      // needs a wavefunction?
             false,                     // needs an isovalue?
             false,                     // needs orbitals?
             "<p>The sign of the chosen molecular "
             "orbital in the region of the "
             "surface.</p>" // description
         }},

        /*
        {localIonisationEnergyProp,
        IsosurfacePropertyDetails::Attributes
        {
         RedGreenBlue, // color scheme
         "Local Ionisation Energy", // property name
         "local_ionisation_energy", // tonto property
        type string "au", // property unit of measure
         true, // needs a wavefunction?
         false, // needs an isovalue?
         false, // needs orbitals?
         "<p>The local ionisation energy.</p>" //
        description
         }},

        {localElectronAffinityProp,
        IsosurfacePropertyDetails::Attributes
        {
         RedGreenBlue, // color scheme
         "Local Electron Affinity", // property name
         "local_electron_affinity", // tonto property
        type string "au", // property unit of measure
         true, // needs a wavefunction?
         false, // needs an isovalue?
         false, // needs orbitals?
         "<p>The local electron affinity.</p>" //
        description
         }},
         */

        {IsosurfacePropertyDetails::Type::FragmentPatch,
         IsosurfacePropertyDetails::Attributes{
             ColorScheme::Qualitative14Dark, // color
                                             // scheme
             "Fragment Patch",               // property name
             "fragment_patch",               // tonto property type
                                             // string
             "",                             // property unit of measure
             false,                          // needs a wavefunction?
             false,                          // needs an isovalue?
             false,                          // needs orbitals?
             "<p>Fragment Patches</p>"       // description
         }},

        {IsosurfacePropertyDetails::Type::Domain,
         IsosurfacePropertyDetails::Attributes{
             ColorScheme::Rainbow, // color scheme
             "Domain",             // property name
             "domain",             // tonto property type string
             "",                   // property unit of measure
             false,                // needs a wavefunction?
             false,                // needs an isovalue?
             false,                // needs orbitals?
             "<p>Domain</p>"       // description
         }}};

IsosurfaceDetails::Attributes
IsosurfaceDetails::getAttributes(IsosurfaceDetails::Type t) {
  return IsosurfaceDetails::availableTypes[t];
}

const QMap<IsosurfaceDetails::Type, IsosurfaceDetails::Attributes>
    IsosurfaceDetails::availableTypes{
        {IsosurfaceDetails::Type::Hirshfeld,
         IsosurfaceDetails::Attributes{
             "Hirshfeld",                           // label
             "Stockholder_weight",                  // tonto surface type
             false,                                 // needs a wavefunction?
             false,                                 // needs an isovalue?
             false,                                 // needs orbitals?
             false,                                 // needs a cluster option?
             0.5f,                                  // default isovalue
             "<p>Generate a Hirshfeld surface.</p>" // description
         }},
        {IsosurfaceDetails::Type::CrystalVoid,
         IsosurfaceDetails::Attributes{
             "Crystal Voids",       // label
             "Promolecule_density", // tonto surface type
             false,                 // needs a wavefunction?
             true,                  // needs an isovalue?
             false,                 // needs orbitals?
             true,                  // needs a cluster option?
             0.002f,
             "<p>Generate a promolecule surface including all "
             "the atoms in the "
             "cluster. The surface is capped withing the unit "
             "cell, and gives an "
             "idea "
             "of voids in the crystal. Choose a lower isovalue "
             "to investigate "
             "<i>channels</i> or <i>pores</i> in the "
             "crystal.</p>" // description
         }},

        {IsosurfaceDetails::Type::PromoleculeDensity,
         IsosurfaceDetails::Attributes{
             "Promolecule Density", // label
             "Promolecule_density", // tonto surface type
             false,                 // needs a wavefunction?
             true,                  // needs an isovalue?
             false,                 // needs orbitals?
             false,                 // needs a cluster option?
             0.002f,

             "<p>The sum of spherical atoms electron density "
             "for the molecule.</p>" // description
         }},

        {IsosurfaceDetails::Type::ElectronDensity,
         IsosurfaceDetails::Attributes{
             "Electron Density", // label
             "Electron_density", // tonto surface type
             true,               // needs a wavefunction?
             true,               // needs an isovalue?
             false,              // needs orbitals?
             false,              // needs a cluster option?
             0.008f,
             "<p>An isosurface of the electron density, "
             "calculated from the "
             "wavefunction in the previous energy "
             "calculation.</p>" // description
         }},

        {IsosurfaceDetails::Type::DeformationDensity,
         IsosurfaceDetails::Attributes{
             "Deformation Density", // label
             "Deformation_density", // tonto surface type
             true,                  // needs a wavefunction?
             true,                  // needs an isovalue?
             false,                 // needs orbitals?
             false,                 // needs a cluster option?
             0.008f,
             "<p>The difference between the <i>ab-initio</i> "
             "electron density, and "
             "the sum of spherical atoms electron density,as "
             "calculated from the "
             "wavefunction in the previous energy "
             "calculation.</p>" // description
         }},

        {IsosurfaceDetails::Type::ElectricPotential,
         IsosurfaceDetails::Attributes{
             "Electrostatic Potential", // label
             "Electric_potential",      // tonto surface type
             true,                      // needs a wavefunction?
             true,                      // needs an isovalue?
             false,                     // needs orbitals?
             false,                     // needs a cluster option?
             0.05f,
             "<p>The <i>ab-initio</i> electrostatic potential "
             "from the electrons "
             "and "
             "the nuclei, calculated from the wavefunction in "
             "the previous energy "
             "calculation.</p>" // description
         }},

        {IsosurfaceDetails::Type::Orbital,
         IsosurfaceDetails::Attributes{
             "Orbital", // label
             "Orbital", // tonto surface type
             true,      // needs a wavefunction?
             true,      // needs an isovalue?
             true,      // needs orbitals?
             false,     // needs a cluster option?
             0.02f,
             "<p>An isosurface of the molecular "
             "orbital, calculated from the "
             "wavefunction.</p>" // description
         }},
        /*
          {IsosurfaceDetails::Type::ADP,
          IsosurfaceDetails::Attributes{"ADP", // label "ADP", //
          tonto surface type false, // needs a wavefunction?
                                           true,  // needs an
          isovalue? false, // needs orbitals? false, // needs a
          cluster option?
                                           "<p>An isosurface of
          the anharmonic " "displacement parameters.</p>" //
          description
                                           }},
                                           */
        {IsosurfaceDetails::Type::SpinDensity,
         IsosurfaceDetails::Attributes{
             "Spin density", // label
             "spin_density", // tonto surface type
             true,           // needs a wavefunction?
             true,           // needs an isovalue?
             false,          // needs orbitals?
             false,          // needs a cluster option?
             0.02f,
             "<p>An isosurface of the spin"
             "density.</p>" // description
         }}

    };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Requestable Property Types
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const QMap<IsosurfaceDetails::Type, QList<IsosurfacePropertyDetails::Type>>
    IsosurfaceDetails::requestableProperties{
        {IsosurfaceDetails::Type::Hirshfeld,
         {IsosurfacePropertyDetails::Type::None,
          IsosurfacePropertyDetails::Type::PromoleculeDensity,
          IsosurfacePropertyDetails::Type::ElectronDensity,
          IsosurfacePropertyDetails::Type::DeformationDensity,
          IsosurfacePropertyDetails::Type::ElectricPotential,
          IsosurfacePropertyDetails::Type::Orbital}},
        {IsosurfaceDetails::Type::CrystalVoid,
         {IsosurfacePropertyDetails::Type::None}},
        {IsosurfaceDetails::Type::PromoleculeDensity,
         {IsosurfacePropertyDetails::Type::None,
          IsosurfacePropertyDetails::Type::ElectronDensity,
          IsosurfacePropertyDetails::Type::DeformationDensity,
          IsosurfacePropertyDetails::Type::ElectricPotential,
          IsosurfacePropertyDetails::Type::Orbital}},
        {IsosurfaceDetails::Type::ElectronDensity,
         {IsosurfacePropertyDetails::Type::None,
          IsosurfacePropertyDetails::Type::PromoleculeDensity,
          IsosurfacePropertyDetails::Type::DeformationDensity,
          IsosurfacePropertyDetails::Type::ElectricPotential,
          IsosurfacePropertyDetails::Type::Orbital}},
        {IsosurfaceDetails::Type::DeformationDensity,
         {IsosurfacePropertyDetails::Type::None}},
        {IsosurfaceDetails::Type::ElectricPotential,
         {IsosurfacePropertyDetails::Type::None}},
        {IsosurfaceDetails::Type::Orbital,
         {IsosurfacePropertyDetails::Type::None}},
        {IsosurfaceDetails::Type::ADP, {IsosurfacePropertyDetails::Type::None}},
        {IsosurfaceDetails::Type::SpinDensity,
         {IsosurfacePropertyDetails::Type::None}}};

const QList<IsosurfacePropertyDetails::Type> &
IsosurfaceDetails::getRequestableProperties(Type t) {
  auto it = requestableProperties.find(t);
  static const QList<IsosurfacePropertyDetails::Type> emptyList;
  if (it != requestableProperties.end())
    return *it;
  return emptyList;
}

float ResolutionDetails::value(ResolutionDetails::Level level) {
    using Level = ResolutionDetails::Level;
    switch(level) {
    case Level::VeryLow:
        return 1.5f;
    case Level::Low:
        return 0.8f;
    case Level::Medium:
        return 0.5f;
    case Level::High:
        return 0.2f;
    case Level::VeryHigh:
        return 0.15f;
    }
}

const char * ResolutionDetails::name(ResolutionDetails::Level level) {
    using Level = ResolutionDetails::Level;
    switch(level) {
    case Level::VeryLow:
        return "Very Low";
    case Level::Low:
        return "Low";
    case Level::Medium:
        return "Medium";
    case Level::High:
        return "High (Standard)";
    case Level::VeryHigh:
        return "Very High";
    }
}

const QList<ResolutionDetails::Level> ResolutionDetails::levels {
    ResolutionDetails::Level::VeryLow,
    ResolutionDetails::Level::Low,
    ResolutionDetails::Level::Medium,
    ResolutionDetails::Level::High,
    ResolutionDetails::Level::VeryHigh,
};

