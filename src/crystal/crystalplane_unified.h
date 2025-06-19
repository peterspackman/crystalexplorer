#pragma once

#include "plane.h"
#include "crystalplane.h"
#include "crystalstructure.h"
#include "crystalplanegenerator.h"

/**
 * CrystalPlaneUnified extends the base Plane class to handle Miller indices and 
 * crystal coordinate systems. It automatically converts between Miller indices 
 * and Cartesian coordinates using the parent CrystalStructure.
 */
class CrystalPlaneUnified : public Plane {
    Q_OBJECT
    
    Q_PROPERTY(int millerH READ millerH WRITE setMillerH NOTIFY millerIndicesChanged)
    Q_PROPERTY(int millerK READ millerK WRITE setMillerK NOTIFY millerIndicesChanged)
    Q_PROPERTY(int millerL READ millerL WRITE setMillerL NOTIFY millerIndicesChanged)
    Q_PROPERTY(double interplanarSpacing READ interplanarSpacing NOTIFY millerIndicesChanged)

public:
    explicit CrystalPlaneUnified(CrystalStructure *parent = nullptr);
    explicit CrystalPlaneUnified(const MillerIndex &hkl, CrystalStructure *parent = nullptr);
    explicit CrystalPlaneUnified(int h, int k, int l, CrystalStructure *parent = nullptr);
    
    // Miller indices properties
    int millerH() const { return m_millerIndex.h; }
    int millerK() const { return m_millerIndex.k; }
    int millerL() const { return m_millerIndex.l; }
    MillerIndex millerIndex() const { return m_millerIndex; }
    
    void setMillerH(int h);
    void setMillerK(int k);
    void setMillerL(int l);
    void setMillerIndex(const MillerIndex &hkl);
    void setMillerIndices(int h, int k, int l);
    
    // Crystal-specific properties
    double interplanarSpacing() const;
    CrystalStructure* parentCrystalStructure() const;
    
    // Override offset units for crystal planes
    QString offsetUnit() const override { return "d"; }
    
    // Override grid units for crystal planes
    QString gridUnit() const override { return "uc"; }
    
    // Override settings update to maintain Miller ↔ Cartesian consistency
    void updateSettings(const PlaneSettings& settings) override;
    
    // Override to prevent orthonormal axis calculation for crystal planes
    void calculateAxesFromNormal() override;
    
    // Conversion between coordinate systems
    void updateCartesianFromMiller();
    void updateMillerFromCartesian();
    
    // Create a CrystalPlaneUnified from existing CrystalPlane struct
    static CrystalPlaneUnified* fromCrystalPlaneStruct(const ::CrystalPlane &crystalPlane, 
                                                       CrystalStructure *parent);
    
    // Convert to CrystalPlane struct for compatibility
    ::CrystalPlane toCrystalPlaneStruct() const;
    
    // Serialization with Miller indices
    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject &json) override;

signals:
    void millerIndicesChanged(int h, int k, int l);

private slots:
    void onCrystalStructureChanged();

private:
    void connectToCrystalStructure();
    void disconnectFromCrystalStructure();
    void updateName();
    
    MillerIndex m_millerIndex{1, 0, 0};  // Default to (100) plane
    bool m_updatingFromMiller{false};    // Prevent conversion loops
    bool m_updatingFromCartesian{false}; // Prevent conversion loops
};