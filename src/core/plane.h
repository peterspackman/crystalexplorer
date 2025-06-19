#pragma once
#include <QObject>
#include <QColor>
#include <QVector2D>
#include <QVector3D>
#include <Eigen/Dense>

class PlaneInstance;  // Forward declaration

/**
 * Settings structure for plane properties
 */
struct PlaneSettings {
    // Visual properties
    bool visible{true};
    QColor color{Qt::red};
    QString name{"Plane"};
    
    // Grid properties
    bool showGrid{true};
    double gridSpacing{1.0};
    QVector2D repeatRangeA{-2, 2};
    QVector2D repeatRangeB{-2, 2};
    
    // Axes properties
    bool showAxes{false};
    QVector3D axisA{1, 0, 0};  // Default to X axis
    QVector3D axisB{0, 1, 0};  // Default to Y axis
    
    // Bounds properties
    bool showBounds{false};
    QVector2D boundsA{-5, 5};
    QVector2D boundsB{-5, 5};
    
    // Geometry properties
    QVector3D origin{0, 0, 0};
    QVector3D normal{0, 0, 1};  // Default to Z axis
    
    bool operator==(const PlaneSettings& other) const {
        return visible == other.visible &&
               color == other.color &&
               name == other.name &&
               showGrid == other.showGrid &&
               qFuzzyCompare(gridSpacing, other.gridSpacing) &&
               repeatRangeA == other.repeatRangeA &&
               repeatRangeB == other.repeatRangeB &&
               showAxes == other.showAxes &&
               axisA == other.axisA &&
               axisB == other.axisB &&
               showBounds == other.showBounds &&
               boundsA == other.boundsA &&
               boundsB == other.boundsB &&
               origin == other.origin &&
               normal == other.normal;
    }
    
    bool operator!=(const PlaneSettings& other) const {
        return !(*this == other);
    }
};

/**
 * Base class for visualization planes with axes, bounds, and visual properties.
 * This serves as a foundation for different types of planes (crystal planes, 
 * cutting planes, measurement planes, etc.)
 */
class Plane : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY settingsChanged)
    Q_PROPERTY(QColor color READ color NOTIFY settingsChanged)
    Q_PROPERTY(QString name READ name NOTIFY settingsChanged)
    Q_PROPERTY(bool showGrid READ showGrid NOTIFY settingsChanged)
    Q_PROPERTY(double gridSpacing READ gridSpacing NOTIFY settingsChanged)
    Q_PROPERTY(bool showAxes READ showAxes NOTIFY settingsChanged)
    Q_PROPERTY(bool showBounds READ showBounds NOTIFY settingsChanged)

public:
    explicit Plane(QObject *parent = nullptr);
    explicit Plane(const QString &name, QObject *parent = nullptr);
    virtual ~Plane() = default;

    // Visual properties
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible);
    QColor color() const { return m_color; }
    QString name() const { return m_name; }

    // Grid properties
    bool showGrid() const { return m_showGrid; }
    double gridSpacing() const { return m_gridSpacing; }
    QVector2D repeatRangeA() const { return m_repeatRangeA; }
    QVector2D repeatRangeB() const { return m_repeatRangeB; }

    // Axes properties
    bool showAxes() const { return m_showAxes; }
    QVector3D axisA() const { return m_axisA; }
    QVector3D axisB() const { return m_axisB; }

    // Bounds properties
    bool showBounds() const { return m_showBounds; }
    QVector2D boundsA() const { return m_boundsA; }
    QVector2D boundsB() const { return m_boundsB; }

    // Geometry properties
    QVector3D origin() const { return m_origin; }
    QVector3D normal() const { return m_normal; }
    
    // Offset units (e.g., "Å" for Cartesian, "d" for crystal d-spacing)
    virtual QString offsetUnit() const { return "Å"; }
    
    // Grid units (e.g., "Å" for Cartesian, "uc" for crystal unit cells)
    virtual QString gridUnit() const { return "Å"; }

    // Plane equation: normal · (point - origin) = 0
    double distanceToPoint(const QVector3D &point) const;
    QVector3D projectPointToPlane(const QVector3D &point) const;

    // Serialization
    virtual QJsonObject toJson() const;
    virtual bool fromJson(const QJsonObject &json);

    // Called by PlaneInstance children when they change
    void onInstanceChanged();
    
    // Create a new PlaneInstance at the specified offset
    PlaneInstance* createInstance(double offset);
    
    // Settings management
    PlaneSettings settings() const;
    virtual void updateSettings(const PlaneSettings& settings);

protected:
    // Calculate orthogonal axes from the normal vector (virtual for crystal planes)
    virtual void calculateAxesFromNormal();

signals:
    void settingsChanged();

protected:
    // Visual properties
    bool m_visible{true};
    QColor m_color{Qt::red};
    QString m_name{"Plane"};
    
    // Grid properties
    bool m_showGrid{true};
    double m_gridSpacing{1.0};
    QVector2D m_repeatRangeA{-2, 2};
    QVector2D m_repeatRangeB{-2, 2};
    
    // Axes properties
    bool m_showAxes{false};
    QVector3D m_axisA{1, 0, 0};  // Default to X axis
    QVector3D m_axisB{0, 1, 0};  // Default to Y axis
    
    // Bounds properties
    bool m_showBounds{false};
    QVector2D m_boundsA{-5, 5};
    QVector2D m_boundsB{-5, 5};
    
    // Geometry properties
    QVector3D m_origin{0, 0, 0};
    QVector3D m_normal{0, 0, 1};  // Default to Z axis
};
