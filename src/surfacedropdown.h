#pragma once
#include "surfacedescription.h"
#include "isosurface_parameters.h"
#include <QComboBox>


using SurfaceDescriptionMap = QMap<QString, isosurface::SurfaceDescription>;
using SurfacePropertyDescriptionMap = QMap<QString, isosurface::SurfacePropertyDescription>;

class SurfaceTypeDropdown : public QComboBox {
    Q_OBJECT
public:
    SurfaceTypeDropdown(QWidget *parent = nullptr);
    QString current() const;
    void setCurrent(QString);

    void setDescriptions(const SurfaceDescriptionMap &surfaceDescriptions);
    isosurface::SurfaceDescription currentSurfaceDescription() const;
signals:
    void selectionChanged(QString);
    void descriptionChanged(QString);
private slots:
    void onCurrentIndexChanged(int);
private:
    SurfaceDescriptionMap m_surfaceDescriptions;
};


class SurfacePropertyTypeDropdown : public QComboBox {
    Q_OBJECT
public:
    SurfacePropertyTypeDropdown(QWidget *parent = nullptr);
    QString current() const;
    void setDescriptions(const SurfaceDescriptionMap &surfaceDescriptions,
                         const SurfacePropertyDescriptionMap &surfacePropertyDescriptions);
    isosurface::SurfacePropertyDescription currentSurfacePropertyDescription() const;
signals:
    void selectionChanged(QString);
private slots:
    void onCurrentIndexChanged(int);
public slots:
    void onSurfaceSelectionChanged(QString);
private:
    SurfaceDescriptionMap m_surfaceDescriptions;
    SurfacePropertyDescriptionMap m_surfacePropertyDescriptions;
    isosurface::SurfaceDescription getSurfaceDescription(const QString &surfaceKey) const;
    isosurface::SurfacePropertyDescription getSurfacePropertyDescription(const QString &propertyKey) const;
};


class ResolutionDropdown : public QComboBox {
    Q_OBJECT
  public:
    ResolutionDropdown(QWidget *parent = nullptr);
    ResolutionDetails::Level currentLevel() const;
    float currentResolutionValue() const;

  private slots:
    void onCurrentIndexChanged(int);

  private:
    void populateDropdown();
    ResolutionDetails::Level m_level;
};
