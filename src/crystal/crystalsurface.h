#include "chemicalstructure.h"
#include <occ/crystal/hkl.h>
#include <occ/crystal/crystal.h>

using OccCrystal = occ::crystal::Crystal;

struct CrystalSurfaceSettings {
  occ::crystal::HKL hkl{1, 0, 0};
  double offset{0.0};
  double depthFactor{1.0};
};

class CrystalSurface: public ChemicalStructure {
public:
  explicit CrystalSurface(QObject *parent = nullptr);

  void setOccCrystalCut(const OccCrystal &, const CrystalSurfaceSettings  &settings = {});
  void setCut(const CrystalSurfaceSettings &settings);

  inline StructureType structureType() const override {
    return StructureType::Surface;
  }

  inline const auto &occCrystal() const { return m_crystal; }
  inline const auto &cutSettings() const { return m_settings; }

  inline const auto &spaceGroup() const { return m_crystal.space_group(); }
  occ::Mat3N convertCoordinates(const occ::Mat3N &pos, ChemicalStructure::CoordinateConversion) const override;

  occ::Mat3 cellVectors() const override;
  occ::Vec3 cellAngles() const override;
  occ::Vec3 cellLengths() const override;

  occ::Vec3 normalVector() const;
  double interplanarSpacing() const;

private:
  void determineVectors();

  OccCrystal m_crystal;
  CrystalSurfaceSettings m_settings;
  occ::Vec3 m_aVector;
  occ::Vec3 m_bVector;
  occ::Vec3 m_depthVector;
  occ::Vec3 m_dipole{0.0, 0.0, 0.0};
  double m_depth{0.0};
  double m_angle{0.0};
};
