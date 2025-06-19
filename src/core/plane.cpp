#include "plane.h"
#include "planeinstance.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QChildEvent>

Plane::Plane(QObject *parent)
    : QObject(nullptr), m_name("Plane") {
    setObjectName(m_name);
    calculateAxesFromNormal();
    if (parent) {
        setParent(parent);
    }
}

Plane::Plane(const QString &name, QObject *parent)
    : QObject(nullptr), m_name(name) {
    setObjectName(m_name);
    calculateAxesFromNormal();
    if (parent) {
        setParent(parent);
    }
}

void Plane::setVisible(bool visible) {
    if (m_visible != visible) {
        m_visible = visible;
        emit settingsChanged();
    }
}

double Plane::distanceToPoint(const QVector3D &point) const {
    QVector3D diff = point - m_origin;
    return std::abs(QVector3D::dotProduct(m_normal, diff));
}

QVector3D Plane::projectPointToPlane(const QVector3D &point) const {
    QVector3D diff = point - m_origin;
    double distance = QVector3D::dotProduct(m_normal, diff);
    return point - distance * m_normal;
}

QJsonObject Plane::toJson() const {
    QJsonObject json;
    json["name"] = m_name;
    json["visible"] = m_visible;
    json["color"] = m_color.name();
    json["showGrid"] = m_showGrid;
    json["gridSpacing"] = m_gridSpacing;
    json["repeatRangeA"] = QJsonArray{m_repeatRangeA.x(), m_repeatRangeA.y()};
    json["repeatRangeB"] = QJsonArray{m_repeatRangeB.x(), m_repeatRangeB.y()};
    json["showAxes"] = m_showAxes;
    json["axisA"] = QJsonArray{m_axisA.x(), m_axisA.y(), m_axisA.z()};
    json["axisB"] = QJsonArray{m_axisB.x(), m_axisB.y(), m_axisB.z()};
    json["showBounds"] = m_showBounds;
    json["boundsA"] = QJsonArray{m_boundsA.x(), m_boundsA.y()};
    json["boundsB"] = QJsonArray{m_boundsB.x(), m_boundsB.y()};
    json["origin"] = QJsonArray{m_origin.x(), m_origin.y(), m_origin.z()};
    json["normal"] = QJsonArray{m_normal.x(), m_normal.y(), m_normal.z()};
    return json;
}

bool Plane::fromJson(const QJsonObject &json) {
    try {
        PlaneSettings settings;
        
        settings.name = json.value("name").toString("Plane");
        settings.visible = json.value("visible").toBool(true);
        settings.color = QColor(json.value("color").toString("#ff0000"));
        settings.showGrid = json.value("showGrid").toBool(true);
        settings.gridSpacing = json.value("gridSpacing").toDouble(1.0);
        
        if (json.contains("repeatRangeA")) {
            auto rangeA = json["repeatRangeA"].toArray();
            settings.repeatRangeA = QVector2D(rangeA[0].toDouble(-2), rangeA[1].toDouble(2));
        }
        
        if (json.contains("repeatRangeB")) {
            auto rangeB = json["repeatRangeB"].toArray();
            settings.repeatRangeB = QVector2D(rangeB[0].toDouble(-2), rangeB[1].toDouble(2));
        }
        
        settings.showAxes = json.value("showAxes").toBool(false);
        
        if (json.contains("axisA")) {
            auto axisA = json["axisA"].toArray();
            settings.axisA = QVector3D(axisA[0].toDouble(1), axisA[1].toDouble(0), axisA[2].toDouble(0));
        }
        
        if (json.contains("axisB")) {
            auto axisB = json["axisB"].toArray();
            settings.axisB = QVector3D(axisB[0].toDouble(0), axisB[1].toDouble(1), axisB[2].toDouble(0));
        }
        
        settings.showBounds = json.value("showBounds").toBool(false);
        
        if (json.contains("boundsA")) {
            auto boundsA = json["boundsA"].toArray();
            settings.boundsA = QVector2D(boundsA[0].toDouble(-5), boundsA[1].toDouble(5));
        }
        
        if (json.contains("boundsB")) {
            auto boundsB = json["boundsB"].toArray();
            settings.boundsB = QVector2D(boundsB[0].toDouble(-5), boundsB[1].toDouble(5));
        }
        
        if (json.contains("origin")) {
            auto origin = json["origin"].toArray();
            settings.origin = QVector3D(origin[0].toDouble(0), origin[1].toDouble(0), origin[2].toDouble(0));
        }
        
        if (json.contains("normal")) {
            auto normal = json["normal"].toArray();
            settings.normal = QVector3D(normal[0].toDouble(0), normal[1].toDouble(0), normal[2].toDouble(1));
        }
        
        updateSettings(settings);
        return true;
    } catch (...) {
        return false;
    }
}

void Plane::onInstanceChanged() {
    // Called directly by PlaneInstance children when they change
    emit settingsChanged();
}

PlaneInstance* Plane::createInstance(double offset) {
    // Create a new PlaneInstance as a child of this plane
    auto *instance = new PlaneInstance(this, offset);
    instance->setObjectName(instance->name());
    return instance;
}

void Plane::calculateAxesFromNormal() {
    // Create an orthonormal basis from the normal vector
    QVector3D normal = m_normal.normalized();
    
    // Choose a vector that's not parallel to the normal
    QVector3D up = (qAbs(normal.z()) < 0.9f) ? QVector3D(0, 0, 1) : QVector3D(1, 0, 0);
    
    // Calculate the first axis (axisA) perpendicular to normal
    m_axisA = QVector3D::crossProduct(normal, up).normalized();
    
    // Calculate the second axis (axisB) perpendicular to both normal and axisA
    m_axisB = QVector3D::crossProduct(normal, m_axisA).normalized();
}

PlaneSettings Plane::settings() const {
    PlaneSettings settings;
    settings.visible = m_visible;
    settings.color = m_color;
    settings.name = m_name;
    settings.showGrid = m_showGrid;
    settings.gridSpacing = m_gridSpacing;
    settings.repeatRangeA = m_repeatRangeA;
    settings.repeatRangeB = m_repeatRangeB;
    settings.showAxes = m_showAxes;
    settings.axisA = m_axisA;
    settings.axisB = m_axisB;
    settings.showBounds = m_showBounds;
    settings.boundsA = m_boundsA;
    settings.boundsB = m_boundsB;
    settings.origin = m_origin;
    settings.normal = m_normal;
    return settings;
}

void Plane::updateSettings(const PlaneSettings& settings) {
    PlaneSettings current = this->settings();
    if (current != settings) {
        m_visible = settings.visible;
        m_color = settings.color;
        m_name = settings.name;
        setObjectName(m_name);
        m_showGrid = settings.showGrid;
        m_gridSpacing = settings.gridSpacing;
        m_repeatRangeA = settings.repeatRangeA;
        m_repeatRangeB = settings.repeatRangeB;
        m_showAxes = settings.showAxes;
        m_axisA = settings.axisA;
        m_axisB = settings.axisB;
        m_showBounds = settings.showBounds;
        m_boundsA = settings.boundsA;
        m_boundsB = settings.boundsB;
        m_origin = settings.origin;
        m_normal = settings.normal;
        
        // Recalculate axes if normal changed
        if (current.normal != settings.normal) {
            calculateAxesFromNormal();
        }
        
        emit settingsChanged();
    }
}