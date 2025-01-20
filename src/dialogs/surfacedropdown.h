#pragma once
#include "isosurface_parameters.h"
#include <QComboBox>

class SurfaceTypeDropdown : public QComboBox {
  Q_OBJECT
public:
  SurfaceTypeDropdown(QWidget *parent = nullptr);
  QString current() const;
  void setCurrent(QString);

  void
  setDescriptions(const isosurface::SurfaceDescriptions &surfaceDescriptions);
  isosurface::SurfaceDescription currentSurfaceDescription() const;
signals:
  void selectionChanged(QString);
  void descriptionChanged(QString);
private slots:
  void onCurrentIndexChanged(int);

private:
  isosurface::SurfaceDescriptions m_surfaceDescriptions;
};

class SurfacePropertyTypeDropdown : public QComboBox {
  Q_OBJECT
public:
  SurfacePropertyTypeDropdown(QWidget *parent = nullptr);
  QString current() const;
  void
  setDescriptions(const isosurface::SurfaceDescriptions &surfaceDescriptions,
                  const isosurface::SurfacePropertyDescriptions
                      &surfacePropertyDescriptions);
  isosurface::SurfacePropertyDescription
  currentSurfacePropertyDescription() const;
signals:
  void selectionChanged(QString);
private slots:
  void onCurrentIndexChanged(int);
public slots:
  void onSurfaceSelectionChanged(QString);

private:
  isosurface::SurfaceDescriptions m_surfaceDescriptions;
  isosurface::SurfacePropertyDescriptions m_surfacePropertyDescriptions;
  isosurface::SurfaceDescription
  getSurfaceDescription(const QString &surfaceKey) const;
  isosurface::SurfacePropertyDescription
  getSurfacePropertyDescription(const QString &propertyKey) const;
};

class ResolutionDropdown : public QComboBox {
  Q_OBJECT
public:
  ResolutionDropdown(QWidget *parent = nullptr);
  isosurface::Resolution currentLevel() const;
  float currentResolutionValue() const;

private slots:
  void onCurrentIndexChanged(int);

private:
  void populateDropdown();
  isosurface::Resolution m_resolutionLevel{isosurface::Resolution::High};
};
