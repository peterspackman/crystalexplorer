#include "planeinstance.h"
#include <QJsonObject>

PlaneInstance::PlaneInstance(Plane *parent, double offset)
    : QObject(nullptr), m_plane(parent), m_offset(offset) {
    
    // Connect to parent plane changes first (before setting parent)
    if (m_plane) {
        connect(m_plane, &Plane::settingsChanged, this, &PlaneInstance::onPlaneChanged);
        connect(m_plane, &Plane::settingsChanged, this, &PlaneInstance::nameChanged);
    }
    
    // Set parent after object is fully constructed to avoid race condition
    if (m_plane) {
        setParent(m_plane);
    }
}

bool PlaneInstance::isVisible() const {
    // Instance is visible if both the instance and parent plane are visible
    return m_visible && (m_plane ? m_plane->isVisible() : false);
}

void PlaneInstance::setVisible(bool visible) {
    if (m_visible != visible) {
        m_visible = visible;
        emit visibilityChanged(isVisible());
        
        // Notify parent plane that geometry has changed
        if (m_plane) {
            m_plane->onInstanceChanged();
        }
    }
}

void PlaneInstance::setOffset(double offset) {
    if (!qFuzzyCompare(m_offset, offset)) {
        m_offset = offset;
        emit offsetChanged(offset);
        emit nameChanged();
        
        // Notify parent plane that geometry has changed
        if (m_plane) {
            m_plane->onInstanceChanged();
        }
    }
}

QString PlaneInstance::name() const {
    if (qFuzzyIsNull(m_offset)) {
        return "origin";
    } else {
        // Format offset with appropriate precision
        QString offsetStr;
        if (std::abs(m_offset) >= 10.0) {
            offsetStr = QString::number(m_offset, 'f', 1); // 1 decimal for large offsets
        } else if (std::abs(m_offset) >= 1.0) {
            offsetStr = QString::number(m_offset, 'f', 2); // 2 decimals for medium offsets
        } else {
            offsetStr = QString::number(m_offset, 'f', 3); // 3 decimals for small offsets
        }
        
        // Add sign explicitly for clarity
        QString sign = (m_offset >= 0) ? "+" : "";
        return QString("%1%2").arg(sign, offsetStr);
    }
}

QVector3D PlaneInstance::origin() const {
    if (!m_plane) {
        return QVector3D(0, 0, 0);
    }
    
    // Calculate the origin point for this plane instance
    // Origin is plane origin + offset * normal_vector
    return m_plane->origin() + m_offset * m_plane->normal();
}

double PlaneInstance::distanceToPoint(const QVector3D &point) const {
    if (!m_plane) {
        return 0.0;
    }
    
    // Distance from point to this offset plane
    QVector3D instanceOrigin = origin();
    QVector3D normal = m_plane->normal();
    
    return std::abs(QVector3D::dotProduct(normal, point - instanceOrigin));
}

QVector3D PlaneInstance::projectPointToPlane(const QVector3D &point) const {
    if (!m_plane) {
        return point;
    }
    
    QVector3D instanceOrigin = origin();
    QVector3D normal = m_plane->normal();
    QVector3D diff = point - instanceOrigin;
    double distance = QVector3D::dotProduct(normal, diff);
    
    return point - distance * normal;
}

void PlaneInstance::onPlaneChanged() {
    // Emit signals when parent plane properties change
    emit visibilityChanged(isVisible());
}

QJsonObject PlaneInstance::toJson() const {
    QJsonObject json;
    json["offset"] = m_offset;
    json["visible"] = m_visible;
    return json;
}

bool PlaneInstance::fromJson(const QJsonObject &json) {
    try {
        setOffset(json.value("offset").toDouble(0.0));
        setVisible(json.value("visible").toBool(true));
        return true;
    } catch (...) {
        return false;
    }
}