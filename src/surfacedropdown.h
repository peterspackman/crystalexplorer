#pragma once
#include "surfacedescription.h"
#include "isosurface_parameters.h"
#include <QComboBox>

class SurfaceTypeDropdown : public QComboBox {
  Q_OBJECT
public:
  SurfaceTypeDropdown(QWidget *parent = nullptr);
  IsosurfaceDetails::Attributes currentSurfaceAttributes() const;

  inline IsosurfaceDetails::Type currentType() const { return m_selectedType; }
  isosurface::Kind currentKind() const;

signals:
  void surfaceTypeChanged(IsosurfaceDetails::Type);
  void descriptionChanged(QString);

private slots:
  void onCurrentIndexChanged(int);

private:
  void populateDropdown();
  IsosurfaceDetails::Type m_selectedType;
};

class SurfacePropertyTypeDropdown : public QComboBox {
  Q_OBJECT
public:
  SurfacePropertyTypeDropdown(QWidget *parent = nullptr);

  IsosurfacePropertyDetails::Attributes
  currentSurfacePropertyAttributes() const;

  inline IsosurfacePropertyDetails::Type currentType() const { return m_selectedType; }

private slots:
  void onCurrentIndexChanged(int);

public slots:
  void onSurfaceTypeChanged(IsosurfaceDetails::Type);

private:
  IsosurfacePropertyDetails::Type m_selectedType;
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
