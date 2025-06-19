#include "crystalplane_unified.h"
#include <QJsonObject>
#include <QDebug>

CrystalPlaneUnified::CrystalPlaneUnified(CrystalStructure *parent)
    : Plane(nullptr), m_millerIndex{1, 0, 0} {
    // Set appropriate defaults for crystal planes
    PlaneSettings settings = this->settings();
    settings.boundsA = QVector2D(0.0, 1.0);  // Unit cell repetitions
    settings.boundsB = QVector2D(0.0, 1.0);
    settings.gridSpacing = 0.1;  // 0.1 unit cells for reasonable density
    Plane::updateSettings(settings);
    
    // Initialize the plane geometry
    if (parent) {
        setParent(parent);
        connectToCrystalStructure();
        updateCartesianFromMiller();
        updateName();
    }
}

CrystalPlaneUnified::CrystalPlaneUnified(const MillerIndex &hkl, CrystalStructure *parent)
    : Plane(nullptr), m_millerIndex(hkl) {
    // Set appropriate defaults for crystal planes
    PlaneSettings settings = this->settings();
    settings.boundsA = QVector2D(0.0, 1.0);  // Unit cell repetitions
    settings.boundsB = QVector2D(0.0, 1.0);
    settings.gridSpacing = 0.1;  // 0.1 unit cells for reasonable density
    Plane::updateSettings(settings);
    
    // Initialize the plane geometry
    if (parent) {
        setParent(parent);
        connectToCrystalStructure();
        updateCartesianFromMiller();
        updateName();
    }
}

CrystalPlaneUnified::CrystalPlaneUnified(int h, int k, int l, CrystalStructure *parent)
    : Plane(nullptr), m_millerIndex{h, k, l} {
    // Set appropriate defaults for crystal planes
    PlaneSettings settings = this->settings();
    settings.boundsA = QVector2D(0.0, 1.0);  // Unit cell repetitions
    settings.boundsB = QVector2D(0.0, 1.0);
    settings.gridSpacing = 0.1;  // 0.1 unit cells for reasonable density
    Plane::updateSettings(settings);
    
    // Initialize the plane geometry
    if (parent) {
        setParent(parent);
        connectToCrystalStructure();
        updateCartesianFromMiller();
        updateName();
    }
}

void CrystalPlaneUnified::setMillerH(int h) {
    if (m_millerIndex.h != h) {
        m_millerIndex.h = h;
        updateCartesianFromMiller();
        updateName();
        emit millerIndicesChanged(m_millerIndex.h, m_millerIndex.k, m_millerIndex.l);
    }
}

void CrystalPlaneUnified::setMillerK(int k) {
    if (m_millerIndex.k != k) {
        m_millerIndex.k = k;
        updateCartesianFromMiller();
        updateName();
        emit millerIndicesChanged(m_millerIndex.h, m_millerIndex.k, m_millerIndex.l);
    }
}

void CrystalPlaneUnified::setMillerL(int l) {
    if (m_millerIndex.l != l) {
        m_millerIndex.l = l;
        updateCartesianFromMiller();
        updateName();
        emit millerIndicesChanged(m_millerIndex.h, m_millerIndex.k, m_millerIndex.l);
    }
}

void CrystalPlaneUnified::setMillerIndex(const MillerIndex &hkl) {
    if (m_millerIndex != hkl) {
        m_millerIndex = hkl;
        updateCartesianFromMiller();
        updateName();
        emit millerIndicesChanged(m_millerIndex.h, m_millerIndex.k, m_millerIndex.l);
    }
}

void CrystalPlaneUnified::setMillerIndices(int h, int k, int l) {
    if (m_millerIndex.h != h || m_millerIndex.k != k || m_millerIndex.l != l) {
        m_millerIndex = {h, k, l};
        updateCartesianFromMiller();
        updateName();
        emit millerIndicesChanged(h, k, l);
    }
}

double CrystalPlaneUnified::interplanarSpacing() const {
    auto *crystal = parentCrystalStructure();
    if (!crystal) {
        return 1.0; // Default spacing
    }
    
    CrystalPlaneGenerator generator(crystal, m_millerIndex);
    return generator.interplanarSpacing();
}

CrystalStructure* CrystalPlaneUnified::parentCrystalStructure() const {
    return qobject_cast<CrystalStructure*>(parent());
}

void CrystalPlaneUnified::updateSettings(const PlaneSettings& settings) {
    PlaneSettings currentSettings = this->settings();
    
    // For crystal planes, manually update settings to avoid calculateAxesFromNormal()
    if (currentSettings != settings) {
        m_visible = settings.visible;
        m_color = settings.color;
        m_name = settings.name;
        setObjectName(m_name);
        m_showGrid = settings.showGrid;
        m_gridSpacing = settings.gridSpacing;
        m_repeatRangeA = settings.repeatRangeA;
        m_repeatRangeB = settings.repeatRangeB;
        m_showAxes = settings.showAxes;
        m_axisA = settings.axisA;  // Keep crystal axes as-is
        m_axisB = settings.axisB;  // Keep crystal axes as-is
        m_showBounds = settings.showBounds;
        m_boundsA = settings.boundsA;
        m_boundsB = settings.boundsB;
        m_origin = settings.origin;
        m_normal = settings.normal;
        
        // Check if origin or normal changed and we're not updating from Miller indices
        bool originChanged = currentSettings.origin != settings.origin;
        bool normalChanged = currentSettings.normal != settings.normal;
        
        if ((originChanged || normalChanged) && !m_updatingFromMiller) {
            // Update Cartesian coordinates and try to find equivalent Miller indices
            m_updatingFromCartesian = true;
            updateMillerFromCartesian();
            m_updatingFromCartesian = false;
        }
        
        emit settingsChanged();
    }
}

void CrystalPlaneUnified::calculateAxesFromNormal() {
    // Do nothing - crystal planes get their axes from the crystal structure,
    // not from orthonormal calculations based on the normal vector
}

void CrystalPlaneUnified::updateCartesianFromMiller() {
    if (m_updatingFromCartesian) {
        return; // Prevent infinite loops
    }
    
    auto *crystal = parentCrystalStructure();
    if (!crystal) {
        qWarning() << "CrystalPlane: No parent CrystalStructure found";
        return;
    }
    
    if (m_millerIndex.isZero()) {
        qWarning() << "CrystalPlane: Invalid Miller indices (0,0,0)";
        return;
    }
    
    m_updatingFromMiller = true;
    
    try {
        CrystalPlaneGenerator generator(crystal, m_millerIndex);
        
        // Update the Cartesian properties (convert occ::Vec3 to QVector3D)
        auto unitNormal = generator.normalVector();
        auto origin = generator.origin(0.0);
        auto aVec = generator.aVector();
        auto bVec = generator.bVector();
        
        // For crystal planes, scale the normal by d-spacing so offsets are in d units
        double dSpacing = generator.interplanarSpacing();
        QVector3D normal(unitNormal.x() * dSpacing, unitNormal.y() * dSpacing, unitNormal.z() * dSpacing);
        
        // Update settings directly using base class method
        PlaneSettings settings = this->settings();
        settings.normal = normal;
        settings.origin = QVector3D(origin.x(), origin.y(), origin.z());
        settings.axisA = QVector3D(aVec.x(), aVec.y(), aVec.z());
        settings.axisB = QVector3D(bVec.x(), bVec.y(), bVec.z());
        Plane::updateSettings(settings);
        
    } catch (const std::exception &e) {
        qWarning() << "CrystalPlane: Error updating from Miller indices:" << e.what();
    }
    
    m_updatingFromMiller = false;
}

void CrystalPlaneUnified::updateMillerFromCartesian() {
    if (m_updatingFromMiller) {
        return; // Prevent infinite loops
    }
    
    auto *crystal = parentCrystalStructure();
    if (!crystal) {
        return;
    }
    
    // This is more complex - we'd need to find Miller indices that best match
    // the current normal vector. For now, we'll just warn that manual Cartesian
    // changes might not maintain crystallographic meaning.
    qDebug() << "CrystalPlane: Cartesian coordinates changed manually - "
             << "Miller indices relationship may no longer be accurate";
}

CrystalPlaneUnified* CrystalPlaneUnified::fromCrystalPlaneStruct(const ::CrystalPlane &crystalPlane, 
                                                              CrystalStructure *parent) {
    auto *plane = new CrystalPlaneUnified(crystalPlane.hkl, parent);
    
    // Update color and offset using settings
    PlaneSettings settings = plane->settings();
    settings.color = crystalPlane.color;
    
    // Apply offset by moving origin along normal
    if (!qFuzzyIsNull(crystalPlane.offset)) {
        QVector3D offsetOrigin = plane->origin() + plane->normal() * crystalPlane.offset;
        settings.origin = offsetOrigin;
    }
    
    plane->Plane::updateSettings(settings); // Use base class to avoid Miller update
    
    return plane;
}

::CrystalPlane CrystalPlaneUnified::toCrystalPlaneStruct() const {
    ::CrystalPlane crystalPlane;
    crystalPlane.hkl = m_millerIndex;
    crystalPlane.color = color();
    
    // Calculate offset from the zero-offset plane
    CrystalPlaneGenerator generator(parentCrystalStructure(), m_millerIndex);
    auto originVec = generator.origin(0.0);
    QVector3D zeroOrigin(originVec.x(), originVec.y(), originVec.z());
    QVector3D currentOrigin = origin();
    QVector3D normal = this->normal().normalized();
    
    // Project offset onto normal direction
    crystalPlane.offset = QVector3D::dotProduct(currentOrigin - zeroOrigin, normal);
    
    return crystalPlane;
}

QJsonObject CrystalPlaneUnified::toJson() const {
    QJsonObject json = Plane::toJson();
    
    // Add Miller indices
    json["millerH"] = m_millerIndex.h;
    json["millerK"] = m_millerIndex.k;
    json["millerL"] = m_millerIndex.l;
    json["isCrystalPlane"] = true;
    
    return json;
}

bool CrystalPlaneUnified::fromJson(const QJsonObject &json) {
    if (!Plane::fromJson(json)) {
        return false;
    }
    
    // Load Miller indices
    if (json.contains("millerH") && json.contains("millerK") && json.contains("millerL")) {
        setMillerIndices(json["millerH"].toInt(1), 
                        json["millerK"].toInt(0), 
                        json["millerL"].toInt(0));
    }
    
    return true;
}

void CrystalPlaneUnified::onCrystalStructureChanged() {
    // Recalculate Cartesian coordinates when crystal structure changes
    updateCartesianFromMiller();
}

void CrystalPlaneUnified::connectToCrystalStructure() {
    auto *crystal = parentCrystalStructure();
    if (crystal) {
        // Connect to crystal structure changes that would affect plane geometry
        connect(crystal, &ChemicalStructure::atomsChanged, 
                this, &CrystalPlaneUnified::onCrystalStructureChanged);
    }
}

void CrystalPlaneUnified::disconnectFromCrystalStructure() {
    auto *crystal = parentCrystalStructure();
    if (crystal) {
        disconnect(crystal, nullptr, this, nullptr);
    }
}

void CrystalPlaneUnified::updateName() {
    // Generate name from Miller indices
    QString name = QString("(%1%2%3)")
                   .arg(m_millerIndex.h)
                   .arg(m_millerIndex.k)
                   .arg(m_millerIndex.l);
    
    if (this->name() != name) {
        PlaneSettings settings = this->settings();
        settings.name = name;
        Plane::updateSettings(settings); // Use base class to avoid potential signal loops
    }
}