#pragma once
#include "plane.h"
#include <QObject>

/**
 * Represents a specific instance of a plane at a particular offset along its normal.
 * This allows a single Plane definition to have multiple instances at different positions.
 * Similar to how MeshInstance works with Mesh.
 */
class PlaneInstance : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibilityChanged)
    Q_PROPERTY(double offset READ offset WRITE setOffset NOTIFY offsetChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)

public:
    explicit PlaneInstance(Plane *parent, double offset = 0.0);

    // Parent plane access
    const Plane *plane() const { return m_plane; }
    Plane *plane() { return m_plane; }

    // Instance properties
    bool isVisible() const;
    void setVisible(bool visible);

    double offset() const { return m_offset; }
    void setOffset(double offset);

    QString name() const;

    // Derived properties from parent plane (with offset applied)
    QColor color() const { return m_plane ? m_plane->color() : QColor(Qt::red); }
    bool showGrid() const { return m_plane ? m_plane->showGrid() : false; }
    double gridSpacing() const { return m_plane ? m_plane->gridSpacing() : 1.0; }
    bool showAxes() const { return m_plane ? m_plane->showAxes() : false; }
    bool showBounds() const { return m_plane ? m_plane->showBounds() : false; }

    // Geometry calculations (with offset applied)
    QVector3D origin() const;
    QVector3D normal() const { return m_plane ? m_plane->normal() : QVector3D(0, 0, 1); }
    QVector3D axisA() const { return m_plane ? m_plane->axisA() : QVector3D(1, 0, 0); }
    QVector3D axisB() const { return m_plane ? m_plane->axisB() : QVector3D(0, 1, 0); }

    // Distance calculations
    double distanceFromOriginalPlane() const { return std::abs(m_offset); }
    double distanceToPoint(const QVector3D &point) const;
    QVector3D projectPointToPlane(const QVector3D &point) const;

    // Serialization
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject &json);

signals:
    void visibilityChanged(bool visible);
    void offsetChanged(double offset);
    void nameChanged();

private slots:
    void onPlaneChanged();

private:
    Plane *m_plane;
    bool m_visible{true};
    double m_offset{0.0};
};