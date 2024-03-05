#pragma once
#include "billboardrenderer.h"
#include "cylinderrenderer.h"
#include "graphics.h"
#include "linerenderer.h"
#include "settings.h"
#include "sphereimpostorrenderer.h"

enum class FragmentPairStyle { RoundedLine, SegmentedLine };

class FragmentPairInfo {
public:
  FragmentPairInfo(QVector3D p1, QVector3D p2, QColor col, QString s)
      : m_point1(p1), m_point2(p2), m_color(col), m_valueString(s) {
    float energy = m_valueString.toFloat();
    if (energy > 0) {
      m_color =
          settings::readSetting(settings::keys::ENERGY_FRAMEWORK_POSITIVE_COLOR)
              .value<QColor>();
    }
  }

  inline QVector3D labelPosition() const { return (m_point1 + m_point2) / 2.0; }
  inline const QString &label() const { return m_valueString; }

  inline void draw(FragmentPairStyle style, EllipsoidRenderer *spheres,
                   CylinderRenderer *cylinders, LineRenderer *lines,
                   BillboardRenderer *text) const {
    float currentScale =
        settings::readSetting(settings::keys::ENERGY_FRAMEWORK_SCALE).toFloat();
    switch (style) {
    case FragmentPairStyle::RoundedLine: {
      std::vector<SphereImpostorVertex> vertices;
      float radius = lineRadius(currentScale);
      cx::graphics::addSphereToEllipsoidRenderer(spheres, m_point1, m_color,
                                             radius);
      cx::graphics::addSphereToEllipsoidRenderer(spheres, m_point2, m_color,
                                             radius);

      cx::graphics::addCylinderToCylinderRenderer(cylinders, m_point1, m_point2,
                                              m_color, m_color, radius);
      break;
    }
    case FragmentPairStyle::SegmentedLine: {
      cx::graphics::addLineToLineRenderer(*lines, m_point1, m_point2, 1.0f,
                                      m_color);
      cx::graphics::addTextToBillboardRenderer(*text, labelPosition(), label());
      break;
    }
    default:
      break;
    }
  }

private:
  inline float lineRadius(float scale) const {
    return fabs(m_valueString.toFloat()) * scale;
  }

  QVector3D m_point1;
  QVector3D m_point2;
  QColor m_color;
  QString m_valueString;
};
