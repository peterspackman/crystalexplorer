#pragma once
#include <occ/crystal/unitcell.h>
#include "crystalplane.h"
#include "chemicalstructure.h"
#include "qeigen.h"
#include <QVector>

class CrystalPlaneGenerator {
public:
  CrystalPlaneGenerator(ChemicalStructure *, const MillerIndex &hkl);
  occ::Vec3 normalVector() const;

  inline double a() const { return m_aVector.norm(); }
  inline double b() const { return m_bVector.norm(); }
  inline double area() const { return m_aVector.cross(m_bVector).norm(); }
  inline double depth() const { return m_depthVector.norm(); }

  double interplanarSpacing() const;

  Matrix3q basisMatrix(double depth_scale = 1.0) const;

  inline const auto &aVector() const { return m_aVector; }
  inline const auto &bVector() const { return m_bVector; }
  inline const auto &depthVector() const { return m_depthVector; }

  inline const auto &alpha() const { return m_angle; }
  inline const auto &hkl() const { return m_hkl; }

  occ::Vec3 origin(double offset = 0.0) const;

private:
  void calculateVectors();

  occ::crystal::UnitCell m_uc;
  MillerIndex m_hkl;
  occ::Vec3 m_aVector;
  occ::Vec3 m_bVector;
  occ::Vec3 m_depthVector;
  float m_angle{0.0};
};
